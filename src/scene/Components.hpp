#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <entt/entt.hpp>

// ---------------------------------------------------------------------------
// Tag
// ---------------------------------------------------------------------------
struct TagComponent {
    std::string name;
    TagComponent() = default;
    explicit TagComponent(std::string n) : name(std::move(n)) {}
};

// ---------------------------------------------------------------------------
// Transform2D
// ---------------------------------------------------------------------------
struct Transform2D {
    glm::vec2 position = {0.f, 0.f};
    float     rotation = 0.f;       // radians
    glm::vec2 scale    = {1.f, 1.f};

    glm::mat4 GetMatrix() const {
        glm::mat4 m = glm::translate(glm::mat4{1.f}, {position, 0.f});
        m = glm::rotate(m, rotation, {0.f, 0.f, 1.f});
        m = glm::scale(m, {scale, 1.f});
        return m;
    }
};

// ---------------------------------------------------------------------------
// SpriteRenderer
// ---------------------------------------------------------------------------
struct SpriteRenderer {
    glm::vec4   color       = {1.f, 1.f, 1.f, 1.f};
    std::string texturePath;        // empty = solid color quad
    unsigned int textureID  = 0;    // GL handle, 0 = unloaded
    int          sortingLayer = 0;
    bool         flipX      = false;
    bool         flipY      = false;
};

// ---------------------------------------------------------------------------
// Camera2D
// ---------------------------------------------------------------------------
struct Camera2D {
    float zoom          = 1.f;
    bool  isPrimary     = true;

    glm::mat4 GetProjection(float vpWidth, float vpHeight) const {
        float half_w = (vpWidth  * 0.5f) / zoom;
        float half_h = (vpHeight * 0.5f) / zoom;
        return glm::ortho(-half_w, half_w, -half_h, half_h, -1.f, 1.f);
    }
};

// ---------------------------------------------------------------------------
// Collider2D (AABB debug visualisation only in Phase 1)
// ---------------------------------------------------------------------------
struct BoxCollider2D {
    glm::vec2 offset = {0.f, 0.f};
    glm::vec2 size   = {1.f, 1.f};
    bool      isTrigger = false;
};

struct CircleCollider2D {
    glm::vec2 offset = {0.f, 0.f};
    float     radius = 0.5f;
    bool      isTrigger = false;
};

// ---------------------------------------------------------------------------
// Scene relationship (parent-child tree)
// ---------------------------------------------------------------------------
struct Parent {
    entt::entity entity = entt::null;
};

struct Children {
    std::vector<entt::entity> list;
};
