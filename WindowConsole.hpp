#ifndef __WINDOW_CONSOLE_HPP__
#define __WINDOW_CONSOLE_HPP__

#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <chrono>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <pty.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include "Fonts.hpp"

class WindowConsole
{
public:
    WindowConsole() = default;

    WindowConsole(const std::string& title, int w, int h,
                  const std::string& shell = "/bin/bash")
    {
        create(title, w, h, shell);
    }

    ~WindowConsole()
    {
        close();
    }

    bool create(const std::string& title, int w, int h, const std::string& shell = "/bin/bash")
    {
        m_display = XOpenDisplay(nullptr);
        if (!m_display)
            return false;

        m_screen = DefaultScreen(m_display);
        m_width = w;
        m_height = h;
        m_title = title;

        Colormap cmap = XCreateColormap(m_display, RootWindow(m_display, m_screen),
                                         DefaultVisual(m_display, m_screen), AllocNone);
        XSetWindowAttributes swa = {};
        swa.colormap = cmap;
        swa.background_pixel = 0x000000;
        swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                         ButtonPressMask | StructureNotifyMask;

        m_window = XCreateWindow(m_display, RootWindow(m_display, m_screen),
                                  0, 0, w, h, 0, DefaultDepth(m_display, m_screen),
                                  InputOutput, DefaultVisual(m_display, m_screen),
                                  CWColormap | CWBackPixel | CWEventMask, &swa);

        if (!m_window)
        {
            XCloseDisplay(m_display);
            m_display = nullptr;
            return false;
        }

        XStoreName(m_display, m_window, title.c_str());
        m_wmDelete = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(m_display, m_window, &m_wmDelete, 1);
        XMapWindow(m_display, m_window);

        m_font = new AntialiasedFont(m_display, m_screen, "monospace-11");
        m_font_w = 8;
        m_font_h = m_font->getHeight() + 2;

        m_cols = (m_width - 8) / m_font_w;
        m_rows = (m_height - 8) / m_font_h;
        if (m_cols < 10) m_cols = 10;
        if (m_rows < 3) m_rows = 3;

        if (!openPty(shell))
        {
            close();
            return false;
        }

        m_open = true;
        return true;
    }

    void close()
    {
        m_open = false;

        if (m_child_pid > 0)
        {
            kill(m_child_pid, SIGTERM);
            usleep(50000);
            kill(m_child_pid, SIGKILL);
            waitpid(m_child_pid, nullptr, WNOHANG);
            m_child_pid = 0;
        }

        if (m_master_fd >= 0)
        {
            ::close(m_master_fd);
            m_master_fd = -1;
        }

        if (m_font)
        {
            delete m_font;
            m_font = nullptr;
        }

        if (m_window)
        {
            XDestroyWindow(m_display, m_window);
            m_window = 0;
        }

        if (m_display)
        {
            XCloseDisplay(m_display);
            m_display = nullptr;
        }
    }

    bool pollEvents()
    {
        if (!m_display || !m_open)
            return false;

        while (XPending(m_display))
        {
            XEvent ev;
            XNextEvent(m_display, &ev);
            handleXEvent(ev);
        }

        return m_open;
    }

    void readPty()
    {
        if (m_master_fd < 0) return;

        char buf[4096];
        int n = ::read(m_master_fd, buf, sizeof(buf) - 1);
        if (n > 0)
        {
            buf[n] = '\0';
            std::cout.write(buf, n);
            std::cout.flush();
            for (int i = 0; i < n; i++)
            {
                char c = buf[i];
                if (c == '\n')
                {
                    m_lines.push_back(m_current_line);
                    m_current_line.clear();
                    if ((int)m_lines.size() > m_max_lines)
                        m_lines.pop_front();
                }
                else if (c == '\r')
                {
                    // ignore
                }
                else
                {
                    m_current_line += c;
                }
            }
        }
        else if (n == 0)
        {
            m_open = false;
        }
    }

    void selectLoop(int fps = 60)
    {
        if (!m_display || m_master_fd < 0) return;

        int x11_fd = ConnectionNumber(m_display);
        useconds_t frame_us = 1000000 / fps;
        auto last_fps_time = std::chrono::steady_clock::now();
        int frame_count = 0;

        while (m_open)
        {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(x11_fd, &fds);
            FD_SET(m_master_fd, &fds);
            int max_fd = (x11_fd > m_master_fd ? x11_fd : m_master_fd) + 1;

            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = frame_us;

            select(max_fd, &fds, nullptr, nullptr, &tv);

            if (FD_ISSET(x11_fd, &fds))
                while (XPending(m_display)) {
                    XEvent ev;
                    XNextEvent(m_display, &ev);
                    handleXEvent(ev);
                }

            if (FD_ISSET(m_master_fd, &fds))
                readPty();

            render();

            frame_count++;
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_fps_time).count();
            if (elapsed >= 500)
            {
                m_fps_counter = (frame_count * 1000) / (int)elapsed;
                frame_count = 0;
                last_fps_time = now;
            }
        }
    }

    void render()
    {
        if (!m_display || !m_window || !m_open)
            return;

        XEvent ev;
        while (XCheckTypedEvent(m_display, Expose, &ev))
            XNextEvent(m_display, &ev);

        GC gc = XCreateGC(m_display, m_window, 0, nullptr);

        XSetForeground(m_display, gc, 0x000000);
        XFillRectangle(m_display, m_window, gc, 0, 0, m_width, m_height);

#if defined(DEBUG)
        std::string dbg = "DEBUG: cols=" + std::to_string(m_cols)
                        + " rows=" + std::to_string(m_rows)
                        + " lines=" + std::to_string(m_lines.size())
                        + " fps=~" + std::to_string(m_fps_counter);
        XSetForeground(m_display, gc, 0x111111);
        XFillRectangle(m_display, m_window, gc, 0, 0, m_width, m_font_h + 4);
        m_font->drawString(m_window, "#555555", 6, 4 + m_font->getAscent(), dbg);
        int start_y = 4 + m_font_h + 2;
#else
        int start_y = 0;
#endif

        int y = 6 + m_font->getAscent() + start_y;
        int line_count = (int)m_lines.size();
        int avail_rows = m_rows - (start_y > 0 ? 1 : 0);

        // show completed lines, then the current line (shell prompt / echo)
        int start = line_count > avail_rows - 1 ? line_count - (avail_rows - 1) : 0;
        for (int i = start; i < line_count; i++)
        {
            m_font->drawString(m_window, "lime green", 6, y, m_lines[i]);
            y += m_font_h;
        }

        // show current line (shell prompt + typed chars, echoed by PTY)
        if (!m_current_line.empty())
        {
            m_font->drawString(m_window, "lime green", 6, y, m_current_line);
        }

        // blinking cursor at end of current line (or input line if no echo yet)
        auto& last = m_current_line.empty() ? m_input_line : m_current_line;
        bool blink_on = (std::time(nullptr) % 1 == 0) ? true : false;
        if (m_cursor_visible && blink_on)
        {
            int cx = 6 + m_font->getTextWidth(last);
            XSetForeground(m_display, gc, 0x00ff00);
            XFillRectangle(m_display, m_window, gc, cx, y - m_font->getAscent() + m_font->getDescent() + 1,
                            m_font_h / 2, 2);
        }

        XFreeGC(m_display, gc);
        XFlush(m_display);
    }

    void writeOutput(const std::string& text)
    {
        m_lines.push_back(text);
        if ((int)m_lines.size() > m_max_lines)
            m_lines.pop_front();
    }

    void sendInput(const std::string& text)
    {
        if (m_master_fd >= 0 && !text.empty())
            ::write(m_master_fd, text.c_str(), text.size());
    }

    bool isOpen() const { return m_open; }

    void* getDisplay() const { return (void*)m_display; }
    uintptr_t getWindow() const { return (uintptr_t)m_window; }

    bool handleXEvent(const XEvent& ev)
    {
        switch (ev.type)
        {
        case DestroyNotify:
            m_window = 0;
            m_open = false;
            break;

        case KeyPress:
        {
            char buf[32] = {};
            int len = 0;
            KeySym ks = XLookupKeysym(const_cast<XKeyEvent*>(&ev.xkey), 0);

            if (ks >= XK_F1 && ks <= XK_F12)
                return true;

            switch (ks)
            {
            case XK_Return:
                buf[0] = '\n'; len = 1;
                m_input_line.clear();
                break;
            case XK_BackSpace:
                if (!m_input_line.empty())
                {
                    m_input_line.pop_back();
                    buf[0] = 0x7f; len = 1; // DEL to PTY
                }
                break;
            case XK_Escape:
                m_input_line.clear();
                buf[0] = '\x1b'; len = 1;
                break;
            default:
                len = XLookupString(const_cast<XKeyEvent*>(&ev.xkey),
                                    buf, sizeof(buf) - 1, &ks, nullptr);
                if (len > 0 && buf[0] >= 32 && buf[0] <= 126)
                    m_input_line += buf[0];
                break;
            }

            if (len > 0)
                sendInput(std::string(buf, len));

            break;
        }

        case ClientMessage:
            if ((Atom)ev.xclient.data.l[0] == m_wmDelete)
                m_open = false;
            break;

        default:
            break;
        }
        return m_open;
    }

private:
    bool openPty(const std::string& shell)
    {
        m_child_pid = forkpty(&m_master_fd, nullptr, nullptr, nullptr);
        if (m_child_pid < 0)
            return false;

        if (m_child_pid == 0)
        {
            const char* sh = shell.c_str();
            const char* argv[] = {sh, "-i", nullptr};
            const char* env[] = {"TERM=xterm-256color", "PATH=/usr/bin:/bin", nullptr};
            execve(sh, (char* const*)argv, (char* const*)env);
            _exit(1);
        }

        fcntl(m_master_fd, F_SETFL, O_NONBLOCK);
        return true;
    }

    Display* m_display = nullptr;
    Window m_window = 0;
    int m_screen = 0;
    int m_width = 0;
    int m_height = 0;
    int m_cols = 80;
    int m_rows = 24;
    int m_font_w = 8;
    int m_font_h = 14;
    Atom m_wmDelete = 0;
    std::string m_title;
    bool m_open = false;

    AntialiasedFont* m_font = nullptr;

    int m_master_fd = -1;
    pid_t m_child_pid = 0;

    std::deque<std::string> m_lines;
    std::string m_current_line;
    std::string m_input_line;
    int m_max_lines = 1000;
    bool m_cursor_visible = true;
    int m_fps_counter = 0;
};

#endif // __WINDOW_CONSOLE_HPP__
