#ifndef __WINDOW_MULTI_CONTEXT_HPP__
#define __WINDOW_MULTI_CONTEXT_HPP__

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <X11/Xlib.h>

#include <streambuf>

#include "GLContext.hpp"
#include "Window.hpp"
#include "WindowConsole.hpp"

class WindowMultiContext;

class ConsoleRedirector {
public:
  ConsoleRedirector(WindowMultiContext& ctx, std::ostream& stream);
  ~ConsoleRedirector();

private:
  class ConsoleBuf : public std::streambuf {
  public:
      ConsoleBuf(std::streambuf* original, WindowMultiContext& ctx);
  protected:
      virtual int_type overflow(int_type c) override;
      virtual std::streamsize xsputn(const char* s, std::streamsize n) override;
  private:
      std::streambuf* m_original;
      WindowMultiContext& m_ctx;
  };

  std::ostream& m_stream;
  WindowMultiContext& m_ctx;
  std::streambuf* m_original;
  ConsoleBuf* m_buf;
};

class WindowMultiContext {
public:
  WindowMultiContext() = default;

  ~WindowMultiContext() {
    for (auto &e : m_entries)
      if (e.active) {
        e.gl->destroy();
        e.win->close();
      }
  }

  int addWindow(const std::string &title, int w, int h,
                const std::string &vert_src, const std::string &frag_src) {
    auto win = std::make_unique<WindowBuffer>(title, w, h);
    if (!win->isOpen())
      return -1;
    auto gl = std::make_unique<GLContext>();
    if (!gl->create(*win, title, w, h, vert_src, frag_src))
      return -1;
    m_entries.push_back(
        {std::move(win), std::move(gl), (::Window)(uintptr_t)0, true});
    m_entries.back().xid = (::Window)m_entries.back().win->nativeWindow();
    return (int)m_entries.size() - 1;
  }

  int addConsole(const std::string &title, int w, int h,
                 const std::string &shell = "/bin/bash",
                 const std::string &font_family = "monospace",
                 float font_size = 11.0f) {
    auto con = std::make_unique<WindowConsole>(title, w, h, shell, font_family, font_size);
    if (!con->isOpen())
      return -1;
    m_consoles.push_back({std::move(con), true});
    return (int)m_consoles.size() - 1;
  }

  void writeToConsoles(const std::string& msg) {
    for (auto &c : m_consoles) {
      if (c.active) {
        c.con->writeOutput(msg);
      }
    }
  }

  bool pollEvents() {
    if (m_entries.empty() && m_consoles.empty())
      return false;

    // Process events for all GL windows
    for (size_t i = 0; i < m_entries.size(); ++i) {
      auto &e = m_entries[i];
      if (!e.active)
        continue;
      Display *display = (Display *)e.win->nativeDisplay();
      if (!display)
        continue;

      while (XPending(display)) {
        XEvent ev;
        XNextEvent(display, &ev);

        if (ev.type == ConfigureNotify) {
          auto &cfg = ev.xconfigure;
          if (cfg.window == e.xid) {
            e.gl->resize(cfg.width, cfg.height);
#if defined(DEBUG)
            std::cout << "[resize] gl window id=" << i << " (xid=" << e.xid << ") rect=" 
                      << cfg.x << "," << cfg.y << " " 
                      << cfg.width << "x" << cfg.height << "\n";
#endif
          }
        }

        if (!e.win->handleXEvent(ev)) {
#if defined(DEBUG)
          std::cout << "[event] gl window id=" << i << " closed (xid=" << e.xid << ")\n";
#endif
          e.active = false;
        }
      }
    }

    // Process events for all consoles
    for (auto &c : m_consoles) {
      if (!c.active)
        continue;
      Display *display = (Display *)c.con->getDisplay();
      if (!display)
        continue;

      while (XPending(display)) {
        XEvent ev;
        XNextEvent(display, &ev);

        if (!c.con->handleXEvent(ev)) {
          c.active = false;
        }
      }
    }

    // read PTY for all consoles
    for (auto &c : m_consoles)
      if (c.active)
        c.con->readPty();

    // If any window was closed by the user, we exit the entire application
    // loop.
    for (auto &e : m_entries)
      if (!e.active)
        return false;
    for (auto &c : m_consoles)
      if (!c.active)
        return false;

    return true;
  }

  void render(int index) {
    if (index >= 0 && index < (int)m_entries.size() && m_entries[index].active)
      m_entries[index].gl->render();
  }

  void renderAll() {
    for (auto &e : m_entries)
      if (e.active)
        e.gl->render();
    for (auto &c : m_consoles)
      if (c.active)
        c.con->render();
  }

  void renderConsoles() {
    for (auto &c : m_consoles)
      if (c.active)
        c.con->render();
  }

  int count() const { return (int)m_entries.size(); }
  int consoleCount() const { return (int)m_consoles.size(); }
  bool isActive(int index) const {
    return index >= 0 && index < (int)m_entries.size() &&
           m_entries[index].active;
  }

  void close(int index) {
    if (index >= 0 && index < (int)m_entries.size() &&
        m_entries[index].active) {
      m_entries[index].gl->destroy();
      m_entries[index].win->close();
      m_entries[index].active = false;
    }
  }

  void closeConsole(int index) {
    if (index >= 0 && index < (int)m_consoles.size() &&
        m_consoles[index].active) {
      m_consoles[index].con->close();
      m_consoles[index].active = false;
    }
  }

private:
  struct Entry {
    std::unique_ptr<WindowBuffer> win;
    std::unique_ptr<GLContext> gl;
    ::Window xid;
    bool active;
  };
  std::vector<Entry> m_entries;

  struct ConsoleEntry {
    std::unique_ptr<WindowConsole> con;
    bool active;
  };
  std::vector<ConsoleEntry> m_consoles;
};

inline ConsoleRedirector::ConsoleBuf::ConsoleBuf(std::streambuf* original, WindowMultiContext& ctx)
    : m_original(original), m_ctx(ctx) {}

inline std::streambuf::int_type ConsoleRedirector::ConsoleBuf::overflow(int_type c) {
    if (c != traits_type::eof()) {
        char ch = traits_type::to_char_type(c);
        std::string s(1, ch);
        if (ch == '\n') s = "\r\n";
        m_ctx.writeToConsoles(s);
        if (m_original) m_original->sputc(c);
    }
    return c;
}

inline std::streamsize ConsoleRedirector::ConsoleBuf::xsputn(const char* s, std::streamsize n) {
    std::string str(s, n);
    size_t pos = 0;
    while ((pos = str.find('\n', pos)) != std::string::npos) {
        str.replace(pos, 1, "\r\n");
        pos += 2;
    }
    m_ctx.writeToConsoles(str);
    if (m_original) return m_original->sputn(s, n);
    return n;
}

inline ConsoleRedirector::ConsoleRedirector(WindowMultiContext& ctx, std::ostream& stream) 
    : m_stream(stream), m_ctx(ctx) 
{
    m_original = stream.rdbuf();
    m_buf = new ConsoleBuf(m_original, m_ctx);
    stream.rdbuf(m_buf);
}

inline ConsoleRedirector::~ConsoleRedirector() {
    m_stream.rdbuf(m_original);
    delete m_buf;
}

#endif // __WINDOW_MULTI_CONTEXT_HPP__
