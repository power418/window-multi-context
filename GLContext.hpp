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

class GLContext
{
public:
    GLContext() = default;

    GLContext(WindowBuffer& win, const std::string& title,
              int w, int h,
              const std::string& vert_src, const std::string& frag_src)
    {
        create(win, title, w, h, vert_src, frag_src);
    }

    ~GLContext()
    {
        destroy();
    }

    bool create(WindowBuffer& win, const std::string& title,
                int w, int h,
                const std::string& vert_src, const std::string& frag_src)
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

        typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
        auto glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)
            glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");

        if (glXCreateContextAttribsARB)
        {
            int ctx_attribs[] = {
                GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
                GLX_CONTEXT_MINOR_VERSION_ARB, 3,
                GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
                None
            };
            m_context = glXCreateContextAttribsARB(m_display, fbconfig[0], nullptr, True, ctx_attribs);
        }

        if (!m_context)
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

        if (!setupShaders(vert_src, frag_src))
        {
            std::cerr << "shader compilation failed\n";
            return false;
        }

        setupGeometry();
        return true;
    }

    void destroy()
    {
        if (m_program) glDeleteProgram(m_program);
        if (m_vao) glDeleteVertexArrays(1, &m_vao);
        if (m_vbo) glDeleteBuffers(1, &m_vbo);
        if (m_context && m_display)
        {
            glXMakeCurrent(m_display, None, nullptr);
            glXDestroyContext(m_display, m_context);
        }
        m_context = nullptr;
        m_vao = m_vbo = m_program = 0;
    }

    void render() const
    {
        glXMakeCurrent(m_display, m_window, m_context);
        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(m_program);
        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glXSwapBuffers(m_display, m_window);
    }

    GLuint program() const { return m_program; }

private:
    bool setupShaders(const std::string& vert_src, const std::string& frag_src)
    {
        auto compile = [](GLuint type, const std::string& src) -> GLuint
        {
            GLuint shader = glCreateShader(type);
            const char* csrc = src.c_str();
            glShaderSource(shader, 1, &csrc, nullptr);
            glCompileShader(shader);

            GLint ok = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
            if (!ok)
            {
                char log[512];
                glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
                std::cerr << "shader compile error:\n" << log << "\n";
                glDeleteShader(shader);
                return 0;
            }
            return shader;
        };

        GLuint vs = compile(GL_VERTEX_SHADER, vert_src);
        GLuint fs = compile(GL_FRAGMENT_SHADER, frag_src);
        if (!vs || !fs)
        {
            glDeleteShader(vs); glDeleteShader(fs);
            return false;
        }

        m_program = glCreateProgram();
        glAttachShader(m_program, vs);
        glAttachShader(m_program, fs);
        glLinkProgram(m_program);

        GLint ok = 0;
        glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
        if (!ok)
        {
            char log[512];
            glGetProgramInfoLog(m_program, sizeof(log), nullptr, log);
            std::cerr << "program link error:\n" << log << "\n";
            glDeleteProgram(m_program);
            glDeleteShader(vs); glDeleteShader(fs);
            m_program = 0;
            return false;
        }

        glDetachShader(m_program, vs);
        glDetachShader(m_program, fs);
        glDeleteShader(vs); glDeleteShader(fs);
        return true;
    }

    void setupGeometry()
    {
        float verts[] = {
            -0.8f, -0.8f, 0.0f,
             0.8f, -0.8f, 0.0f,
             0.0f,  0.8f, 0.0f
        };

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    Display* m_display = nullptr;
    Window m_window = 0;
    GLXContext m_context = nullptr;
    GLuint m_program = 0;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
};

#endif // __GL_CONTEXT_HPP__
