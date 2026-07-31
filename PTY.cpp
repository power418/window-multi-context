#include "PTY.hpp"

#include <pty.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <signal.h>

PTY::PTY() : m_master_fd(-1), m_child_pid(-1) {
}

PTY::~PTY() {
    close();
}

bool PTY::spawn(const std::string& shell, const TerminalSize& size) {
    struct winsize ws;
    ws.ws_col = size.cols;
    ws.ws_row = size.rows;
    ws.ws_xpixel = size.width_px;
    ws.ws_ypixel = size.height_px;

    m_child_pid = forkpty(&m_master_fd, nullptr, nullptr, &ws);
    
    if (m_child_pid < 0) {
        return false;
    }

    if (m_child_pid == 0) {
        // We are the child process
        const char* sh = shell.c_str();
        const char* argv[] = {sh, "-i", nullptr};
        const char* env[] = {"TERM=xterm-256color", "PATH=/usr/bin:/bin", nullptr};
        execve(sh, (char* const*)argv, (char* const*)env);
        _exit(1);
    }

    // We are the parent process
    // Set non-blocking mode for the master fd
    fcntl(m_master_fd, F_SETFL, O_NONBLOCK);
    return true;
}

void PTY::close() {
    if (m_child_pid > 0) {
        ::kill(m_child_pid, SIGTERM);
        usleep(50000);
        ::kill(m_child_pid, SIGKILL);
        waitpid(m_child_pid, nullptr, WNOHANG);
        m_child_pid = -1;
    }

    if (m_master_fd >= 0) {
        ::close(m_master_fd);
        m_master_fd = -1;
    }
}

int PTY::getMasterFD() const {
    return m_master_fd;
}

pid_t PTY::getChildPID() const {
    return m_child_pid;
}

void PTY::resize(const TerminalSize& size) {
    if (m_master_fd >= 0) {
        struct winsize ws;
        ws.ws_col = size.cols;
        ws.ws_row = size.rows;
        ws.ws_xpixel = size.width_px;
        ws.ws_ypixel = size.height_px;
        ioctl(m_master_fd, TIOCSWINSZ, &ws);
    }
}

int PTY::read(char* buffer, int max_len) {
    if (m_master_fd < 0) return -1;
    return ::read(m_master_fd, buffer, max_len);
}

void PTY::write(const std::string& data) {
    if (m_master_fd >= 0 && !data.empty()) {
        ::write(m_master_fd, data.c_str(), data.size());
    }
}
