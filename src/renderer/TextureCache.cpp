#include "TextureCache.hpp"
#include <stb_image.h>

TextureCache& TextureCache::Get() {
    static TextureCache inst;
    return inst;
}

GLuint TextureCache::Load(const std::string& absolutePath) {
    auto it = m_cache.find(absolutePath);
    if (it != m_cache.end()) return it->second;

    stbi_set_flip_vertically_on_load(1);
    int w = 0, h = 0, ch = 0;
    unsigned char* data = stbi_load(absolutePath.c_str(), &w, &h, &ch, 4);
    if (!data) return 0;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    stbi_image_free(data);

    m_cache[absolutePath] = tex;
    return tex;
}

void TextureCache::Evict(const std::string& absolutePath) {
    auto it = m_cache.find(absolutePath);
    if (it == m_cache.end()) return;
    glDeleteTextures(1, &it->second);
    m_cache.erase(it);
}

void TextureCache::Clear() {
    for (auto& [path, tex] : m_cache)
        glDeleteTextures(1, &tex);
    m_cache.clear();
}
