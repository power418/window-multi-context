#ifndef __GL_CONTEXT_HPP__
#define __GL_CONTEXT_HPP__

#include <iostream>
#include <string>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <GL/glxext.h>

#include "Window.hpp"
#include "FileBuffer.hpp"
#include "Texture.hpp"

class GLContext
{
public:
    GLContext() = default;

    GLContext(WindowBuffer& win, const std::string& title,
              int w, int h)
    {
        create(win, title, w, h);
    }

    ~GLContext()
    {
        destroy();
    }

    bool create(WindowBuffer& win, const std::string& title,
                int w, int h)
    {
        m_display = (Display*)win.nativeDisplay();
        if (!m_display)
            return false;

        int nelements = 0;
        int fb_attribs[] = {
            GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
            GLX_RENDER_TYPE,   GLX_RGBA_BIT,
            GLX_DOUBLEBUFFER,  True,
            GLX_RED_SIZE,      8,
            GLX_GREEN_SIZE,    8,
            GLX_BLUE_SIZE,     8,
            GLX_ALPHA_SIZE,    8,
            GLX_DEPTH_SIZE,    24,
            None
        };

        GLXFBConfig* fbconfig = glXChooseFBConfig(m_display, DefaultScreen(m_display), fb_attribs, &nelements);
        if (!fbconfig || nelements == 0)
        {
            std::cerr << "no suitable GLX framebuffer config\n";
            return false;
        }

        XVisualInfo* vi = glXGetVisualFromFBConfig(m_display, fbconfig[0]);
        if (!vi)
        {
            XFree(fbconfig);
            return false;
        }

        if (!win.recreate(title, w, h, (void*)vi))
        {
            std::cerr << "failed to recreate window with GL visual\n";
            XFree(vi);
            XFree(fbconfig);
            return false;
        }

        m_context = glXCreateNewContext(m_display, fbconfig[0], GLX_RGBA_TYPE, nullptr, True);

        XFree(vi);
        XFree(fbconfig);

        if (!m_context)
        {
            std::cerr << "failed to create GL context\n";
            return false;
        }

        m_window = (Window)win.nativeWindow();
        glXMakeCurrent(m_display, m_window, m_context);

        // === SECTION: Texture Setup (stb_image + GL) ===
        m_texture.loadFromFile("res/prabowo_dajjal.png");
        return true;
    }

    void destroy()
    {
        // === SECTION: Cleanup GL Resources ===
        m_texture.destroy();
        if (m_context && m_display)
        {
            glXMakeCurrent(m_display, None, nullptr);
            glXDestroyContext(m_display, m_context);
        }
        m_context = nullptr;
    }

    void render() const
    {
        // === SECTION: Frame Render Loop ===
        glXMakeCurrent(m_display, m_window, m_context);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, m_texture.id());
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glBegin(GL_QUADS);
        // texcoords           // position
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-0.6f, -0.8f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( 0.6f, -0.8f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( 0.6f,  0.8f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-0.6f,  0.8f);
        glEnd();
        
        glDisable(GL_BLEND);
        
        glXSwapBuffers(m_display, m_window);
    }

    void resize(int w, int h)
    {
        if (m_context && m_display && m_window) {
            glXMakeCurrent(m_display, m_window, m_context);
            glViewport(0, 0, w, h);
        }
    }

    Display* m_display = nullptr;
    Window m_window = 0;
    GLXContext m_context = nullptr;
    Texture m_texture;
};

#endif // __GL_CONTEXT_HPP__
