#ifndef __PTY_HPP__
#define __PTY_HPP__

#include <string>
#include <sys/types.h>
#include <termios.h>

struct TerminalSize {
    int cols;
    int rows;
    int width_px;
    int height_px;
};

class PTY {
public:
    PTY();
    ~PTY();

    bool spawn(const std::string& shell, const TerminalSize& size);
    void close();
    
    int getMasterFD() const;
    pid_t getChildPID() const;
    
    void resize(const TerminalSize& size);
    
    int read(char* buffer, int max_len);
    void write(const std::string& data);

private:
    int m_master_fd;
    pid_t m_child_pid;
};

#endif // __PTY_HPP__
