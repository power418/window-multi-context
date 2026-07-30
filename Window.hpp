#ifndef __WINDOW_HPP__
#define __WINDOW_HPP__

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

class WindowBuffer
{
public:
    WindowBuffer() = default;

    WindowBuffer(const std::string& title, int width, int height)
    {
        create(title, width, height);
    }

    ~WindowBuffer()
    {
        close();
    }

    bool create(const std::string& title, int width, int height, XVisualInfo* vi = nullptr)
    {
        m_display = XOpenDisplay(nullptr);
        if (!m_display)
            return false;

        m_screen = DefaultScreen(m_display);
        m_width = width;
        m_height = height;
        m_title = title;

        if (vi)
        {
            Colormap cmap = XCreateColormap(m_display, RootWindow(m_display, m_screen), vi->visual, AllocNone);
            XSetWindowAttributes swa = {};
            swa.colormap = cmap;
            swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                             ButtonPressMask | StructureNotifyMask;

            m_window = XCreateWindow(
                m_display, RootWindow(m_display, m_screen),
                0, 0, width, height, 0, vi->depth, InputOutput, vi->visual,
                CWColormap | CWEventMask, &swa
            );

            XStoreName(m_display, m_window, title.c_str());

            m_wmDelete = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
            XSetWMProtocols(m_display, m_window, &m_wmDelete, 1);

            XMapWindow(m_display, m_window);

            m_gc = XCreateGC(m_display, m_window, 0, nullptr);
            m_pixels.resize(width * height, 0);

            m_image = XCreateImage(
                m_display, CopyFromParent,
                DefaultDepth(m_display, m_screen),
                ZPixmap, 0, nullptr, width, height, 32, 0
            );

            if (!m_image)
            {
                XFreeGC(m_display, m_gc); m_gc = nullptr;
                XDestroyWindow(m_display, m_window); m_window = 0;
                XCloseDisplay(m_display); m_display = nullptr;
                return false;
            }

            m_image->data = (char*)m_pixels.data();

            XEvent ev;
            while (XCheckTypedEvent(m_display, Expose, &ev))
                XNextEvent(m_display, &ev);
        }

        // Single-window software rendering path (alternative to multi-context GL):
        // - Creates window with default visual via XCreateSimpleWindow
        // - Uses XImage + GC for software pixel rendering (setPixel/clear/drawRect)
        // - render() calls XPutImage to blit framebuffer to screen
        // This approach does NOT use OpenGL/GLX and is simpler for 2D rendering.
        // Keep it as reference for non-GL single-window mode.
        //
        // else
        // {
        //     m_window = XCreateSimpleWindow(
        //         m_display, RootWindow(m_display, m_screen),
        //         0, 0, width, height, 0,
        //         BlackPixel(m_display, m_screen),
        //         WhitePixel(m_display, m_screen)
        //     );
        //
        //     XSelectInput(m_display, m_window,
        //         ExposureMask | KeyPressMask | KeyReleaseMask |
        //         ButtonPressMask | StructureNotifyMask);
        // }
        //
        // XStoreName(m_display, m_window, title.c_str());
        //
        // m_wmDelete = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
        // XSetWMProtocols(m_display, m_window, &m_wmDelete, 1);
        //
        // XMapWindow(m_display, m_window);
        //
        // m_gc = XCreateGC(m_display, m_window, 0, nullptr);
        // m_pixels.resize(width * height, 0);
        //
        // m_image = XCreateImage(
        //     m_display, CopyFromParent,
        //     DefaultDepth(m_display, m_screen),
        //     ZPixmap, 0, nullptr, width, height, 32, 0
        // );
        //
        // if (!m_image)
        // {
        //     XFreeGC(m_display, m_gc); m_gc = nullptr;
        //     XDestroyWindow(m_display, m_window); m_window = 0;
        //     XCloseDisplay(m_display); m_display = nullptr;
        //     return false;
        // }
        //
        // m_image->data = (char*)m_pixels.data();
        //
        // XEvent ev;
        // while (XCheckTypedEvent(m_display, Expose, &ev))
        //     XNextEvent(m_display, &ev);

        return true;
    }

    void close()
    {
        if (m_image)
        {
            m_image->data = nullptr;
            XDestroyImage(m_image);
            m_image = nullptr;
        }
        if (m_gc)
        {
            XFreeGC(m_display, m_gc);
            m_gc = nullptr;
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

    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
    {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height)
            return;
        m_pixels[y * m_width + x] = (255 << 24) | (r << 16) | (g << 8) | b;
    }

    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height)
            return;
        m_pixels[y * m_width + x] = (a << 24) | (r << 16) | (g << 8) | b;
    }

    uint32_t getPixel(int x, int y) const
    {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height)
            return 0;
        return m_pixels[y * m_width + x];
    }

    void clear(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0)
    {
        uint32_t color = (255 << 24) | (r << 16) | (g << 8) | b;
        std::fill(m_pixels.begin(), m_pixels.end(), color);
    }

    void drawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b)
    {
        for (int py = y; py < y + h && py < m_height; py++)
            for (int px = x; px < x + w && px < m_width; px++)
                setPixel(px, py, r, g, b);
    }

    void render()
    {
        if (!m_display || !m_window || !m_image || !m_gc)
            return;
        XPutImage(m_display, m_window, m_gc, m_image, 0, 0, 0, 0, m_width, m_height);
        XFlush(m_display);
    }

    bool handleXEvent(const XEvent& ev)
    {
        switch (ev.type)
        {
        case DestroyNotify:
            m_window = 0;
            return false;
        case KeyPress:
        {
            auto ks = XLookupKeysym(const_cast<XKeyEvent*>(&ev.xkey), 0);
            if (ks == XK_Escape || ks == XK_q)
                return false;
            break;
        }
        case ClientMessage:
            if ((Atom)ev.xclient.data.l[0] == m_wmDelete)
                return false;
            break;
        default:
            break;
        }
        return true;
    }

    bool pollEvents()
    {
        if (!m_display)
            return false;

        while (XPending(m_display))
        {
            XEvent ev;
            XNextEvent(m_display, &ev);
            if (!handleXEvent(ev))
                return false;
        }
        return true;
    }

    int width() const { return m_width; }
    int height() const { return m_height; }
    bool isOpen() const { return m_display != nullptr; }
    uint32_t* pixels() { return m_pixels.data(); }

    void* nativeDisplay() const { return (void*)m_display; }
    uintptr_t nativeWindow() const { return (uintptr_t)m_window; }

    bool recreate(const std::string& title, int width, int height, void* vi_ptr)
    {
        XVisualInfo* vi = (XVisualInfo*)vi_ptr;
        m_window = 0;

        if (vi)
        {
            Colormap cmap = XCreateColormap(m_display, RootWindow(m_display, m_screen), vi->visual, AllocNone);
            XSetWindowAttributes swa = {};
            swa.colormap = cmap;
            swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                             ButtonPressMask | StructureNotifyMask;

            m_window = XCreateWindow(
                m_display, RootWindow(m_display, m_screen),
                0, 0, width, height, 0, vi->depth, InputOutput, vi->visual,
                CWColormap | CWEventMask, &swa
            );
        }

        if (!m_window)
            return false;

        XStoreName(m_display, m_window, title.c_str());

        m_wmDelete = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(m_display, m_window, &m_wmDelete, 1);

        XSelectInput(m_display, m_window,
            ExposureMask | KeyPressMask | KeyReleaseMask |
            ButtonPressMask | StructureNotifyMask);

        XMapWindow(m_display, m_window);

        XEvent ev;
        while (XCheckTypedEvent(m_display, Expose, &ev))
            XNextEvent(m_display, &ev);

        return true;
    }

private:
    Display* m_display = nullptr;
    Window m_window = 0;
    GC m_gc = nullptr;
    XImage* m_image = nullptr;
    std::vector<uint32_t> m_pixels;
    int m_screen = 0;
    int m_width = 0;
    int m_height = 0;
    Atom m_wmDelete = 0;
    std::string m_title;
};

#endif // __WINDOW_HPP__
