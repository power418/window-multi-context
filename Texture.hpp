#ifndef __TEXTURE_HPP__
#define __TEXTURE_HPP__

#include <iostream>
#include <string>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include "vendor/stb_image.h"

class Texture
{
public:
    Texture() = default;

    ~Texture()
    {
        destroy();
    }

    // === SECTION: Load PNG via stb_image ===
    bool loadFromFile(const std::string& path)
    {
        destroy();

        // Flip vertical karena OpenGL origin di kiri-bawah, PNG di kiri-atas
        stbi_set_flip_vertically_on_load(1);
        int w = 0, h = 0, channels = 0;
        // Force 4 channel (RGBA) biar upload konsisten
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
        if (!data)
        {
            std::cerr << "failed to load texture '" << path << "': "
                      << stbi_failure_reason() << "\n";
            return false;
        }

        // === SECTION: Upload ke GPU ===
        glGenTextures(1, &m_id);
        glBindTexture(GL_TEXTURE_2D, m_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // === SECTION: Texture Filtering & Wrap Mode ===
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);

        m_width = w;
        m_height = h;
        m_loaded = true;
        std::cout << "texture loaded: " << path
                  << " (" << w << "x" << h << ")\n";
        return true;
    }

    void bind(GLenum unit = GL_TEXTURE0) const
    {
        if (!m_loaded)
            return;
        glActiveTexture(unit);
        glBindTexture(GL_TEXTURE_2D, m_id);
    }

    void destroy()
    {
        if (m_id)
            glDeleteTextures(1, &m_id);
        m_id = 0;
        m_loaded = false;
    }

    GLuint id() const { return m_id; }
    bool isLoaded() const { return m_loaded; }
    int width() const { return m_width; }
    int height() const { return m_height; }

private:
    GLuint m_id = 0;
    bool m_loaded = false;
    int m_width = 0;
    int m_height = 0;
};

#endif // __TEXTURE_HPP__
