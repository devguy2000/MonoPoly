#pragma once
#include "renderer/Renderer2D.hpp"
#include "renderer/Framebuffer.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <functional>

class SceneGraph;

class ViewportPanel {
public:
    ViewportPanel();

    void SetScene(SceneGraph* s)                                { m_scene = s; }
    void SetSelected(entt::entity e)                            { m_selected = e; }
    void SetOnEntityPicked(std::function<void(entt::entity)> cb){ m_onEntityPicked = std::move(cb); }

    void      OnImGuiRender(float dt);
    glm::mat4 GetViewProjection() const;

private:
    void RenderScene();
    void DrawOverlay();
    void HandleInteraction(bool vpHovered);

    glm::vec2 WorldToScreen(glm::vec2 world) const;
    glm::vec2 ScreenToWorld(glm::vec2 screen) const;
    bool      HitHandle(const glm::vec2& pos, const glm::vec2& scale, int h, glm::vec2 mouseScr) const;
    bool      HitBody  (const glm::vec2& pos, const glm::vec2& scale, glm::vec2 mouseWorld) const;

    SceneGraph*  m_scene    = nullptr;
    entt::entity m_selected = entt::null;
    bool         m_showGrid = true;
    bool         m_showAxis = true;

    Framebuffer  m_fbo;
    Renderer2D   m_renderer;
    bool         m_initialized = false;
    int          m_vpWidth = 1, m_vpHeight = 1;
    glm::vec2    m_vpMin   = {};   // screen-space top-left of the viewport image

    // Editor camera
    glm::vec2  m_camPos  = {0.f, 0.f};
    float      m_camZoom = 1.f;
    bool       m_panning = false;
    glm::vec2  m_panStart= {};
    glm::vec2  m_camStart= {};

    // Gizmo drag: {minX, maxX, minY, maxY} of entity AABB at drag start
    enum class GizmoDrag { None, Translate, Scale };
    GizmoDrag  m_drag           = GizmoDrag::None;
    int        m_dragHandle     = -1;
    glm::vec2  m_dragStartMouse = {};
    glm::vec2  m_dragStartPos   = {};
    glm::vec4  m_dragStartAABB  = {};

    std::function<void(entt::entity)> m_onEntityPicked;
};
