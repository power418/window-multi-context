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
#include <sys/ioctl.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include "Fonts.hpp"
#include "Terminal.hpp"

class WindowConsole
{
public:
    WindowConsole() : m_terminal(80, 24) {}

    WindowConsole(const std::string& title, int w, int h,
                  const std::string& shell = "/bin/bash") : m_terminal(80, 24)
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
        swa.background_pixmap = None;
        swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                         ButtonPressMask | StructureNotifyMask;

        m_window = XCreateWindow(m_display, RootWindow(m_display, m_screen),
                                  0, 0, w, h, 0, DefaultDepth(m_display, m_screen),
                                  InputOutput, DefaultVisual(m_display, m_screen),
                                  CWColormap | CWBackPixmap | CWEventMask, &swa);

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

        updateSize();
        
        if (!shell.empty())
        {
            if (!m_terminal.spawn(shell))
            {
                close();
                return false;
            }
        }

        m_open = true;
        return true;
    }

    void close()
    {
        m_open = false;
        m_terminal.close();

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
        m_terminal.update();
    }

    void selectLoop(int fps = 60)
    {
        int pty_fd = m_terminal.getPTY().getMasterFD();
        if (!m_display) return;

        int x11_fd = ConnectionNumber(m_display);
        useconds_t frame_us = 1000000 / fps;
        auto last_fps_time = std::chrono::steady_clock::now();
        int frame_count = 0;

        while (m_open)
        {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(x11_fd, &fds);
            int max_fd = x11_fd + 1;

            if (pty_fd >= 0) {
                FD_SET(pty_fd, &fds);
                if (pty_fd >= max_fd) max_fd = pty_fd + 1;
            }

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

            if (pty_fd >= 0 && FD_ISSET(pty_fd, &fds))
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

        Pixmap pixmap = XCreatePixmap(m_display, m_window, m_width, m_height, DefaultDepth(m_display, m_screen));
        GC gc = XCreateGC(m_display, pixmap, 0, nullptr);

        XSetForeground(m_display, gc, 0x000000);
        XFillRectangle(m_display, pixmap, gc, 0, 0, m_width, m_height);

        Grid& grid = m_terminal.getState().getGrid();
        
#if defined(DEBUG)
        std::string dbg = "DEBUG: cols=" + std::to_string(grid.getCols())
                        + " rows=" + std::to_string(grid.getRows())
                        + " fps=~" + std::to_string(m_fps_counter);
        XSetForeground(m_display, gc, 0x111111);
        XFillRectangle(m_display, pixmap, gc, 0, 0, m_width, m_font_h + 4);
        m_font->drawString(pixmap, "#555555", 6, 4 + m_font->getAscent(), dbg);
        int start_y = 4 + m_font_h + 2;
#else
        int start_y = 0;
#endif

        // Draw the grid
        for (int row = 0; row < grid.getRows(); ++row)
        {
            int col = 0;
            while (col < grid.getCols()) {
                const Cell& first_cell = grid.getCell(col, row);
                uint32_t fg = first_cell.fg_color;
                uint32_t bg = first_cell.bg_color;
                
                std::string chunk;
                int start_col = col;

                // Group characters by same foreground & background color
                while (col < grid.getCols() && 
                       grid.getCell(col, row).fg_color == fg && 
                       grid.getCell(col, row).bg_color == bg) 
                {
                    char32_t c = grid.getCell(col, row).character;
                    if (c == 0) c = ' ';
                    
                    // Convert char32_t (UTF-32) back to UTF-8 string for Xft rendering
                    if (c < 0x80) {
                        chunk += (char)c;
                    } else if (c < 0x800) {
                        chunk += (char)(0xC0 | (c >> 6));
                        chunk += (char)(0x80 | (c & 0x3F));
                    } else if (c < 0x10000) {
                        chunk += (char)(0xE0 | (c >> 12));
                        chunk += (char)(0x80 | ((c >> 6) & 0x3F));
                        chunk += (char)(0x80 | (c & 0x3F));
                    } else if (c <= 0x10FFFF) {
                        chunk += (char)(0xF0 | (c >> 18));
                        chunk += (char)(0x80 | ((c >> 12) & 0x3F));
                        chunk += (char)(0x80 | ((c >> 6) & 0x3F));
                        chunk += (char)(0x80 | (c & 0x3F));
                    } else {
                        chunk += '?';
                    }
                    col++;
                }
                
                int px_x = 6 + (start_col * m_font_w);
                int px_y = 6 + start_y + (row * m_font_h);
                
                // Draw background quad if it's not the default terminal background (0x000000)
                if (bg != 0x000000) {
                    XSetForeground(m_display, gc, bg);
                    XFillRectangle(m_display, pixmap, gc, px_x, px_y, (col - start_col) * m_font_w, m_font_h);
                }
                
                // Draw text chunk
                bool is_empty_spaces = (chunk.find_first_not_of(' ') == std::string::npos);
                if (!is_empty_spaces || bg != 0x000000) {
                    char hex[16];
                    snprintf(hex, sizeof(hex), "#%06X", fg);
                    m_font->drawString(pixmap, hex, px_x, px_y + m_font->getAscent(), chunk);
                }
            }
        }

        // Draw cursor
        int cx = m_terminal.getState().getCursorX();
        int cy = m_terminal.getState().getCursorY();
        bool blink_on = (std::time(nullptr) % 1 == 0) ? true : false;
        
        if (m_cursor_visible && blink_on)
        {
            int cursor_px_x = 6 + (cx * m_font_w);
            int cursor_px_y = 6 + start_y + (cy * m_font_h);
            
            XSetFunction(m_display, gc, GXxor);
            XSetForeground(m_display, gc, 0xE5E5E5); // White XOR
            XFillRectangle(m_display, pixmap, gc, cursor_px_x, cursor_px_y, m_font_w, m_font_h);
            XSetFunction(m_display, gc, GXcopy); // Restore normal drawing
        }

        XCopyArea(m_display, pixmap, m_window, gc, 0, 0, m_width, m_height, 0, 0);
        XFreeGC(m_display, gc);
        XFreePixmap(m_display, pixmap);
        XFlush(m_display);
    }

    void writeOutput(const std::string& text)
    {
        m_terminal.writeOutput(text);
    }

    void sendInput(const std::string& text)
    {
        m_terminal.sendInput(text);
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
            case XK_Up:
                sendInput("\x1b[A");
                break;
            case XK_Down:
                sendInput("\x1b[B");
                break;
            case XK_Right:
                sendInput("\x1b[C");
                break;
            case XK_Left:
                sendInput("\x1b[D");
                break;
            case XK_Home:
                sendInput("\x1b[H");
                break;
            case XK_End:
                sendInput("\x1b[F");
                break;
            case XK_Page_Up:
                sendInput("\x1b[5~");
                break;
            case XK_Page_Down:
                sendInput("\x1b[6~");
                break;
            case XK_Return:
                buf[0] = '\n'; len = 1;
                break;
            case XK_BackSpace:
                buf[0] = 0x7f; len = 1; // DEL to PTY
                break;
            case XK_Escape:
                buf[0] = '\x1b'; len = 1;
                break;
            default:
                len = XLookupString(const_cast<XKeyEvent*>(&ev.xkey),
                                    buf, sizeof(buf) - 1, &ks, nullptr);
                break;
            }

            if (len > 0)
                sendInput(std::string(buf, len));

            break;
        }

        case ConfigureNotify:
            if (ev.xconfigure.width != m_width || ev.xconfigure.height != m_height)
            {
                m_width = ev.xconfigure.width;
                m_height = ev.xconfigure.height;
                updateSize();
                m_terminal.resize(m_cols, m_rows, m_width, m_height);
            }
            break;

        case ClientMessage:
            if ((Atom)ev.xclient.data.l[0] == m_wmDelete)
                m_open = false;
            break;

        case Expose:
            // nothing
            break;

        default:
            break;
        }
        return m_open;
    }

private:
    void updateSize()
    {
#if defined(DEBUG)
        int header_h = 4 + m_font_h + 2;
#else
        int header_h = 0;
#endif
        m_cols = (m_width - 8) / m_font_w;
        m_rows = (m_height - 8 - header_h) / m_font_h;
        if (m_cols < 10) m_cols = 10;
        if (m_rows < 3) m_rows = 3;
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

    Terminal m_terminal;

    AntialiasedFont* m_font = nullptr;

    bool m_cursor_visible = true;
    int m_fps_counter = 0;
};

#endif // __WINDOW_CONSOLE_HPP__
