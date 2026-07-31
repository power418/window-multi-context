#ifndef __WINDOW_MULTI_CONTEXT_HPP__
#define __WINDOW_MULTI_CONTEXT_HPP__

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <X11/Xlib.h>

#include "GLContext.hpp"
#include "Window.hpp"
#include "WindowConsole.hpp"

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
                 const std::string &shell = "/bin/bash") {
    auto con = std::make_unique<WindowConsole>(title, w, h, shell);
    if (!con->isOpen())
      return -1;
    m_consoles.push_back({std::move(con), true});
    return (int)m_consoles.size() - 1;
  }

  bool pollEvents() {
    if (m_entries.empty() && m_consoles.empty())
      return false;

    // Process events for all GL windows
    for (auto &e : m_entries) {
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
            std::string msg = "[resize] gl window " + std::to_string(e.xid) +
                              " rect=" + std::to_string(cfg.x) + "," +
                              std::to_string(cfg.y) + " " +
                              std::to_string(cfg.width) + "x" +
                              std::to_string(cfg.height);
            std::cout << msg << "\n";
#endif
          }
        }

        if (!e.win->handleXEvent(ev)) {
#if defined(DEBUG)
          std::cout << "[event] gl window closed (xid=" << e.xid << ")\n";
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

        if (ev.type == ConfigureNotify) {
          auto &cfg = ev.xconfigure;
          if (cfg.window == (::Window)(uintptr_t)c.con->getWindow()) {
#if defined(DEBUG)
            std::string msg =
                "[resize] console " + std::to_string((uintptr_t)cfg.window) +
                " rect=" + std::to_string(cfg.x) + "," + std::to_string(cfg.y) +
                " " + std::to_string(cfg.width) + "x" +
                std::to_string(cfg.height);
            std::cout << msg << "\n";
            for (auto &c2 : m_consoles)
              if (c2.active)
                c2.con->writeOutput(msg + "\r\n");
#endif
          }
        }

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

#endif // __WINDOW_MULTI_CONTEXT_HPP__
