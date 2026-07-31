#include "ViewportPanel.hpp"
#include "scene/SceneGraph.hpp"
#include "scene/Components.hpp"

#include <imgui.h>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>
#include <vector>
#include <limits>

ViewportPanel::ViewportPanel() = default;

// ---------------------------------------------------------------------------
glm::mat4 ViewportPanel::GetViewProjection() const {
    float hw = m_vpWidth  * 0.5f / m_camZoom;
    float hh = m_vpHeight * 0.5f / m_camZoom;
    return glm::ortho(
        m_camPos.x - hw, m_camPos.x + hw,
        m_camPos.y - hh, m_camPos.y + hh,
        -1.f, 1.f);
}

glm::vec2 ViewportPanel::WorldToScreen(glm::vec2 w) const {
    float hw = m_vpWidth  * 0.5f / m_camZoom;
    float hh = m_vpHeight * 0.5f / m_camZoom;
    return {
        (w.x - (m_camPos.x - hw)) * m_camZoom + m_vpMin.x,
        (m_camPos.y + hh - w.y)   * m_camZoom + m_vpMin.y
    };
}

glm::vec2 ViewportPanel::ScreenToWorld(glm::vec2 s) const {
    float hw = m_vpWidth  * 0.5f / m_camZoom;
    float hh = m_vpHeight * 0.5f / m_camZoom;
    return {
        (s.x - m_vpMin.x) / m_camZoom + (m_camPos.x - hw),
        m_camPos.y + hh - (s.y - m_vpMin.y) / m_camZoom
    };
}

// ---------------------------------------------------------------------------
// 8 handle positions relative to entity center (fractions of half-scale):
//  0=TL  1=TC  2=TR
//  3=ML        4=MR
//  5=BL  6=BC  7=BR
static const glm::vec2 kHandleOff[8] = {
    {-0.5f,  0.5f}, {0.f,  0.5f}, {0.5f,  0.5f},
    {-0.5f,  0.f },               {0.5f,  0.f },
    {-0.5f, -0.5f}, {0.f, -0.5f}, {0.5f, -0.5f}
};

// For each handle: {affectsX, movesMaxX, affectsY, movesMaxY}
struct HDesc { bool aX, mXmax, aY, mYmax; };
static const HDesc kHDesc[8] = {
    {true,false,true,true }, {false,false,true,true }, {true,true,true,true },
    {true,false,false,false},                           {true,true,false,false},
    {true,false,true,false}, {false,false,true,false}, {true,true,true,false}
};

static glm::vec2 HandleWorld(const glm::vec2& pos, const glm::vec2& scale, int h) {
    return pos + kHandleOff[h] * scale;
}

bool ViewportPanel::HitHandle(const glm::vec2& pos, const glm::vec2& scale,
                               int h, glm::vec2 ms) const {
    glm::vec2 sp = WorldToScreen(HandleWorld(pos, scale, h));
    return std::abs(ms.x - sp.x) <= 7.f && std::abs(ms.y - sp.y) <= 7.f;
}

bool ViewportPanel::HitBody(const glm::vec2& pos, const glm::vec2& scale,
                             glm::vec2 mw) const {
    return mw.x >= pos.x - scale.x * 0.5f && mw.x <= pos.x + scale.x * 0.5f &&
           mw.y >= pos.y - scale.y * 0.5f && mw.y <= pos.y + scale.y * 0.5f;
}

// ---------------------------------------------------------------------------
void ViewportPanel::RenderScene() {
    m_fbo.Bind();

    glm::vec4 bg = m_scene ? m_scene->Settings().backgroundColor : glm::vec4{0.f,0.f,0.f,1.f};
    glClearColor(bg.r, bg.g, bg.b, bg.a);
    glClear(GL_COLOR_BUFFER_BIT);

    m_renderer.BeginScene(GetViewProjection());

    // Grid
    if (m_showGrid) {
        float hw = m_vpWidth  * 0.5f / m_camZoom;
        float hh = m_vpHeight * 0.5f / m_camZoom;
        float L = m_camPos.x - hw, R = m_camPos.x + hw;
        float B = m_camPos.y - hh, T = m_camPos.y + hh;

        // Adaptive world-space cell size (target ~60 px per cell on screen)
        float ws = 60.f / m_camZoom;
        float mag = std::pow(10.f, std::floor(std::log10(std::max(ws, 0.001f))));
        float n = ws / mag;
        if      (n < 2.f) ws = mag;
        else if (n < 5.f) ws = 2.f * mag;
        else               ws = 5.f * mag;

        glm::vec4 gc = {0.25f, 0.25f, 0.25f, 0.5f};
        for (float x = std::floor(L / ws) * ws; x <= R + ws; x += ws)
            m_renderer.DrawLine({x, B - ws}, {x, T + ws}, gc);
        for (float y = std::floor(B / ws) * ws; y <= T + ws; y += ws)
            m_renderer.DrawLine({L - ws, y}, {R + ws, y}, gc);

        // Axis overlay
        if (m_showAxis) {
            m_renderer.DrawLine({L, 0.f}, {R, 0.f}, {0.8f, 0.25f, 0.25f, 1.f}); // X red
            m_renderer.DrawLine({0.f, B}, {0.f, T}, {0.25f, 0.8f, 0.25f, 1.f}); // Y green
        }
    }

    // Scene sprites + colliders
    if (m_scene)
        m_renderer.DrawScene(*m_scene);

    // Camera frustum (game-window boundary centered at primary Camera2D entity)
    if (m_scene) {
        auto& s = m_scene->Settings();
        glm::vec2 camCenter = {0.f, 0.f};
        for (auto [e, tf, cam] : m_scene->View<Transform2D, Camera2D>().each())
            if (cam.isPrimary) { camCenter = tf.position; break; }
        m_renderer.DrawRect(camCenter,
            {(float)s.screenWidth, (float)s.screenHeight},
            {0.4f, 0.6f, 1.f, 0.85f});
    }

    // Selection outline (rotated box, drawn last so it's on top)
    if (m_scene && m_selected != entt::null && m_scene->Registry().valid(m_selected) &&
        m_scene->HasComponent<Transform2D>(m_selected))
    {
        auto& t  = m_scene->GetComponent<Transform2D>(m_selected);
        float co = std::cos(t.rotation), si = std::sin(t.rotation);
        auto rot2d = [&](float lx, float ly) -> glm::vec2 {
            return {t.position.x + lx * co - ly * si,
                    t.position.y + lx * si + ly * co};
        };
        float hx = t.scale.x * 0.5f, hy = t.scale.y * 0.5f;
        glm::vec2 p0 = rot2d(-hx,-hy), p1 = rot2d(hx,-hy);
        glm::vec2 p2 = rot2d( hx, hy), p3 = rot2d(-hx, hy);
        glm::vec4 yel = {1.f, 1.f, 0.f, 1.f};
        m_renderer.DrawLine(p0, p1, yel);
        m_renderer.DrawLine(p1, p2, yel);
        m_renderer.DrawLine(p2, p3, yel);
        m_renderer.DrawLine(p3, p0, yel);
    }

    m_renderer.EndScene();
    m_fbo.Unbind();
}

// ---------------------------------------------------------------------------
void ViewportPanel::DrawOverlay() {
    if (!m_scene || m_selected == entt::null || !m_scene->Registry().valid(m_selected)) return;
    if (!m_scene->HasComponent<Transform2D>(m_selected)) return;

    auto&       t  = m_scene->GetComponent<Transform2D>(m_selected);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    constexpr float kHS = 5.f;

    for (int h = 0; h < 8; ++h) {
        glm::vec2 sp = WorldToScreen(HandleWorld(t.position, t.scale, h));
        dl->AddRectFilled({sp.x - kHS, sp.y - kHS}, {sp.x + kHS, sp.y + kHS},
                          IM_COL32(255, 255, 255, 220));
        dl->AddRect      ({sp.x - kHS, sp.y - kHS}, {sp.x + kHS, sp.y + kHS},
                          IM_COL32(60, 60, 60, 255));
    }
}

// ---------------------------------------------------------------------------
void ViewportPanel::HandleInteraction(bool vpHovered) {
    ImVec2    mp       = ImGui::GetMousePos();
    glm::vec2 mouseScr = {mp.x, mp.y};
    glm::vec2 mouseWld = ScreenToWorld(mouseScr);

    bool lmbDown  = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    bool lmbClick = vpHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

    // Continue active drag
    if (m_drag != GizmoDrag::None) {
        if (lmbDown && m_scene && m_selected != entt::null &&
            m_scene->Registry().valid(m_selected) &&
            m_scene->HasComponent<Transform2D>(m_selected))
        {
            auto& t = m_scene->GetComponent<Transform2D>(m_selected);
            glm::vec2 delta = mouseWld - m_dragStartMouse;

            if (m_drag == GizmoDrag::Translate) {
                t.position = m_dragStartPos + delta;
            } else {
                const auto& hd = kHDesc[m_dragHandle];
                float mnX = m_dragStartAABB.x, mxX = m_dragStartAABB.y;
                float mnY = m_dragStartAABB.z, mxY = m_dragStartAABB.w;
                const float kMin = 2.f;
                if (hd.aX) {
                    if (hd.mXmax) mxX = std::max(mnX + kMin, m_dragStartAABB.y + delta.x);
                    else          mnX = std::min(mxX - kMin, m_dragStartAABB.x + delta.x);
                }
                if (hd.aY) {
                    if (hd.mYmax) mxY = std::max(mnY + kMin, m_dragStartAABB.w + delta.y);
                    else          mnY = std::min(mxY - kMin, m_dragStartAABB.z + delta.y);
                }
                t.scale    = {mxX - mnX, mxY - mnY};
                t.position = {(mnX + mxX) * 0.5f, (mnY + mxY) * 0.5f};
            }
        } else {
            m_drag = GizmoDrag::None;
        }
        return; // skip new-click logic while dragging
    }

    // New click: check gizmo handles, body, then scene pick
    if (lmbClick) {
        bool handled = false;

        if (m_scene && m_selected != entt::null && m_scene->Registry().valid(m_selected) &&
            m_scene->HasComponent<Transform2D>(m_selected))
        {
            auto& t = m_scene->GetComponent<Transform2D>(m_selected);

            for (int h = 0; h < 8 && !handled; ++h) {
                if (HitHandle(t.position, t.scale, h, mouseScr)) {
                    m_drag       = GizmoDrag::Scale;
                    m_dragHandle = h;
                    m_dragStartMouse = mouseWld;
                    m_dragStartPos   = t.position;
                    m_dragStartAABB  = {
                        t.position.x - t.scale.x * 0.5f,
                        t.position.x + t.scale.x * 0.5f,
                        t.position.y - t.scale.y * 0.5f,
                        t.position.y + t.scale.y * 0.5f
                    };
                    handled = true;
                }
            }

            if (!handled && HitBody(t.position, t.scale, mouseWld)) {
                m_drag           = GizmoDrag::Translate;
                m_dragStartMouse = mouseWld;
                m_dragStartPos   = t.position;
                handled          = true;
            }
        }

        // Entity pick (click on empty area or different sprite)
        if (!handled && m_scene) {
            entt::entity picked = entt::null;
            int topLayer = std::numeric_limits<int>::lowest();
            for (auto [e, tf, sr] : m_scene->View<Transform2D, SpriteRenderer>().each()) {
                if (HitBody(tf.position, tf.scale, mouseWld) && sr.sortingLayer >= topLayer) {
                    picked   = e;
                    topLayer = sr.sortingLayer;
                }
            }
            if (m_onEntityPicked) m_onEntityPicked(picked);
        }
    }

    // Camera pan (middle mouse) and zoom (scroll)
    if (vpHovered && m_drag == GizmoDrag::None) {
        float scroll = ImGui::GetIO().MouseWheel;
        if (scroll != 0.f)
            m_camZoom = glm::clamp(m_camZoom * (1.f + scroll * 0.1f), 0.01f, 200.f);

        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            if (!m_panning) {
                m_panning  = true;
                m_panStart = {mp.x, mp.y};
                m_camStart = m_camPos;
            }
            glm::vec2 d = glm::vec2{mp.x, mp.y} - m_panStart;
            m_camPos = m_camStart + glm::vec2{-d.x, d.y} / m_camZoom;
        } else {
            m_panning = false;
        }
    } else if (m_drag == GizmoDrag::None) {
        m_panning = false;
    }
}

// ---------------------------------------------------------------------------
void ViewportPanel::OnImGuiRender(float /*dt*/) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.f, 0.f});
    ImGui::Begin("Viewport");

    // ---- Toolbar row ----
    ImGui::SetCursorPos({6.f, 6.f});
    ImGui::Checkbox("Grid", &m_showGrid); ImGui::SameLine(0.f, 10.f);
    ImGui::Checkbox("Axis", &m_showAxis); ImGui::SameLine(0.f, 16.f);
    ImGui::TextDisabled("Zoom %.2fx", m_camZoom); ImGui::SameLine(0.f, 12.f);
    ImGui::TextDisabled("Cam (%.0f, %.0f)", m_camPos.x, m_camPos.y);
    ImGui::SetCursorPosX(0.f); // reset X so the image starts at the left edge

    // ---- Viewport image ----
    ImVec2 avail = ImGui::GetContentRegionAvail();
    int w = std::max(1, (int)avail.x);
    int h = std::max(1, (int)avail.y);

    if (!m_initialized) {
        m_fbo.Create(w, h);
        m_renderer.Init();
        m_initialized = true;
        m_vpWidth = w; m_vpHeight = h;
    } else if (w != m_vpWidth || h != m_vpHeight) {
        m_fbo.Resize(w, h);
        m_vpWidth = w; m_vpHeight = h;
    }

    RenderScene();

    ImGui::Image((ImTextureID)(uintptr_t)m_fbo.ColorAttachment(), avail,
                 {0.f, 1.f}, {1.f, 0.f});

    // Capture image rect for world↔screen transforms
    ImVec2 imgMin = ImGui::GetItemRectMin();
    m_vpMin = {imgMin.x, imgMin.y};

    bool vpHovered = ImGui::IsItemHovered();

    // ImDrawList overlay (selection handles, on top of image)
    DrawOverlay();

    // Input: gizmo drag + camera pan/zoom
    HandleInteraction(vpHovered);

    ImGui::End();
    ImGui::PopStyleVar();
}
