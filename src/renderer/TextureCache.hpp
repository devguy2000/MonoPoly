#pragma once
#include <glad/glad.h>
#include <string>
#include <unordered_map>

// Singleton GL texture cache keyed by absolute file path.
// Call Clear() when a project is closed.
class TextureCache {
public:
    static TextureCache& Get();

    GLuint Load(const std::string& absolutePath);
    void   Evict(const std::string& absolutePath);
    void   Clear();

private:
    TextureCache() = default;
    std::unordered_map<std::string, GLuint> m_cache;
};
