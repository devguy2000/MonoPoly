#include "ViewportPanel.hpp"
#include "scene/SceneGraph.hpp"
#include "scene/Components.hpp"

#include <imgui.h>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

ViewportPanel::ViewportPanel() = default;

// ---------------------------------------------------------------------------
glm::mat4 ViewportPanel::GetViewProjection() const {
    float hw = (m_vpWidth  * 0.5f) / m_camZoom;
    float hh = (m_vpHeight * 0.5f) / m_camZoom;

    glm::mat4 proj = glm::ortho(
        m_camPos.x - hw, m_camPos.x + hw,
        m_camPos.y - hh, m_camPos.y + hh,
        -1.f, 1.f
    );
    return proj;
}

// ---------------------------------------------------------------------------
void ViewportPanel::RenderScene() {
    m_fbo.Bind();
    glClearColor(0.15f, 0.15f, 0.18f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_renderer.BeginScene(GetViewProjection());

    if (m_scene)
        m_renderer.DrawScene(*m_scene);

    m_renderer.EndScene();
    m_fbo.Unbind();
}

// ---------------------------------------------------------------------------
void ViewportPanel::OnImGuiRender(float /*dt*/) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.f, 0.f});
    ImGui::Begin("Viewport");

    ImVec2 avail = ImGui::GetContentRegionAvail();
    int w = (int)avail.x;
    int h = (int)avail.y;
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    // Lazy init / resize
    if (!m_initialized) {
        m_fbo.Create(w, h);
        m_renderer.Init();
        m_initialized = true;
        m_vpWidth  = w;
        m_vpHeight = h;
    } else if (w != m_vpWidth || h != m_vpHeight) {
        m_fbo.Resize(w, h);
        m_vpWidth  = w;
        m_vpHeight = h;
    }

    RenderScene();

    // Blit FBO color texture into ImGui image (flip V: ImGui UV origin is top-left)
    ImGui::Image(
        (ImTextureID)(uintptr_t)m_fbo.ColorAttachment(),
        avail,
        {0.f, 1.f}, {1.f, 0.f}
    );

    // --- Editor camera controls (only when viewport is hovered) ---
    if (ImGui::IsItemHovered()) {
        // Zoom with scroll
        float scroll = ImGui::GetIO().MouseWheel;
        if (scroll != 0.f) {
            m_camZoom = glm::clamp(m_camZoom * (1.f + scroll * 0.1f), 0.01f, 200.f);
        }

        // Pan with middle mouse
        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            if (!m_panning) {
                m_panning  = true;
                ImVec2 mp  = ImGui::GetMousePos();
                m_panStart = {mp.x, mp.y};
                m_camStart = m_camPos;
            }
            ImVec2 mp     = ImGui::GetMousePos();
            glm::vec2 delta = {mp.x - m_panStart.x, mp.y - m_panStart.y};
            // Screen pixels → world units
            float worldPerPx = 1.f / m_camZoom;
            m_camPos = m_camStart + glm::vec2{-delta.x, delta.y} * worldPerPx;
        } else {
            m_panning = false;
        }
    } else {
        m_panning = false;
    }

    ImGui::End();
    ImGui::PopStyleVar();
}
