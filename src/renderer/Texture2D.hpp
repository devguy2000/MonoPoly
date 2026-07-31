#pragma once
#include <glad/glad.h>
#include <string>

class Texture2D {
public:
    Texture2D() = default;
    ~Texture2D() { Destroy(); }

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    bool LoadFromFile(const std::string& path);
    void Destroy();

    void   Bind(unsigned int slot = 0) const;
    GLuint ID()     const { return m_id; }
    int    Width()  const { return m_width; }
    int    Height() const { return m_height; }
    bool   IsLoaded() const { return m_id != 0; }

private:
    GLuint m_id     = 0;
    int    m_width  = 0;
    int    m_height = 0;
};
