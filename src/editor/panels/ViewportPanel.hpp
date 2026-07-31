#pragma once
#include "renderer/Framebuffer.hpp"
#include "renderer/Renderer2D.hpp"
#include <glm/glm.hpp>

class SceneGraph;

class ViewportPanel {
public:
    ViewportPanel();
    ~ViewportPanel() = default;

    void SetScene(SceneGraph* scene) { m_scene = scene; }
    void OnImGuiRender(float dt);

private:
    void RenderScene();
    glm::mat4 GetViewProjection() const;

    SceneGraph*  m_scene = nullptr;
    Framebuffer  m_fbo;
    Renderer2D   m_renderer;
    bool         m_initialized = false;

    // Editor camera state
    glm::vec2    m_camPos    = {0.f, 0.f};
    float        m_camZoom   = 1.f;
    int          m_vpWidth   = 1;
    int          m_vpHeight  = 1;

    // Pan state
    bool         m_panning   = false;
    glm::vec2    m_panStart  = {};
    glm::vec2    m_camStart  = {};
};
