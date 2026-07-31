#pragma once
#include <glad/glad.h>

class Framebuffer {
public:
    Framebuffer() = default;
    ~Framebuffer() { Destroy(); }

    // Non-copyable, movable
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void Create(int width, int height);
    void Resize(int width, int height);
    void Bind()   const;
    void Unbind() const;
    void Destroy();

    GLuint ColorAttachment() const { return m_colorTex; }
    int    Width()  const { return m_width; }
    int    Height() const { return m_height; }
    bool   IsValid() const { return m_fbo != 0; }

private:
    GLuint m_fbo      = 0;
    GLuint m_colorTex = 0;
    GLuint m_rbo      = 0;   // depth+stencil renderbuffer
    int    m_width    = 0;
    int    m_height   = 0;
};
