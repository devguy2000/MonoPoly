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
    float     rotation = 0.f;         // radians
    glm::vec2 scale    = {100.f, 100.f}; // world units = pixels at zoom 1

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
// Scene-level settings (background, resolution)
// Stored once per scene, not per entity
// ---------------------------------------------------------------------------
struct SceneSettings {
    glm::vec4 backgroundColor = {0.f, 0.f, 0.f, 1.f}; // matches game default
    int       screenWidth     = 1280;
    int       screenHeight    = 720;
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

// ---------------------------------------------------------------------------
// ScriptComponent – associates a C# source file with an entity
// ---------------------------------------------------------------------------
struct ScriptComponent {
    std::string className; // C# class name, e.g. "PlayerController"
    std::string filePath;  // relative to project root, e.g. "Scripts/PlayerController.cs"
};
