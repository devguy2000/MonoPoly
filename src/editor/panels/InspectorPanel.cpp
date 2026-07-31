#include "InspectorPanel.hpp"
#include "scene/SceneGraph.hpp"
#include "scene/Components.hpp"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <cstring>

InspectorPanel::InspectorPanel(SceneGraph* scene)
    : m_scene(scene) {}

// ---------------------------------------------------------------------------
void InspectorPanel::OnImGuiRender() {
    ImGui::Begin("Inspector");

    if (!m_scene || m_selected == entt::null || !m_scene->Registry().valid(m_selected)) {
        ImGui::TextDisabled("(No selection)");
        ImGui::End();
        return;
    }

    DrawTag();
    ImGui::Separator();
    DrawTransform2D();

    if (m_scene->HasComponent<SpriteRenderer>(m_selected)) { ImGui::Separator(); DrawSpriteRenderer(); }
    if (m_scene->HasComponent<Camera2D>(m_selected))       { ImGui::Separator(); DrawCamera2D(); }
    if (m_scene->HasComponent<BoxCollider2D>(m_selected))  { ImGui::Separator(); DrawBoxCollider2D(); }
    if (m_scene->HasComponent<CircleCollider2D>(m_selected)){ ImGui::Separator(); DrawCircleCollider2D(); }

    ImGui::Spacing();
    DrawAddComponentMenu();

    ImGui::End();
}

// ---------------------------------------------------------------------------
void InspectorPanel::DrawTag() {
    auto& tag = m_scene->GetComponent<TagComponent>(m_selected);
    char buf[256];
    std::strncpy(buf, tag.name.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf)-1] = '\0';
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::InputText("##tag", buf, sizeof(buf)))
        tag.name = buf;
}

void InspectorPanel::DrawTransform2D() {
    if (!ImGui::CollapsingHeader("Transform 2D", ImGuiTreeNodeFlags_DefaultOpen)) return;

    auto& t = m_scene->GetComponent<Transform2D>(m_selected);
    ImGui::DragFloat2("Position", glm::value_ptr(t.position), 0.5f);
    float deg = glm::degrees(t.rotation);
    if (ImGui::DragFloat("Rotation",  &deg, 0.5f, -360.f, 360.f, "%.1f°"))
        t.rotation = glm::radians(deg);
    ImGui::DragFloat2("Scale", glm::value_ptr(t.scale), 0.01f, 0.001f, 1000.f);
}

void InspectorPanel::DrawSpriteRenderer() {
    if (!ImGui::CollapsingHeader("Sprite Renderer", ImGuiTreeNodeFlags_DefaultOpen)) return;

    auto& sr = m_scene->GetComponent<SpriteRenderer>(m_selected);
    ImGui::ColorEdit4("Color", glm::value_ptr(sr.color));

    char pathBuf[512];
    std::strncpy(pathBuf, sr.texturePath.c_str(), sizeof(pathBuf) - 1);
    pathBuf[sizeof(pathBuf)-1] = '\0';
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::InputText("Texture Path", pathBuf, sizeof(pathBuf)))
        sr.texturePath = pathBuf;

    ImGui::DragInt("Sorting Layer", &sr.sortingLayer);
    ImGui::Checkbox("Flip X", &sr.flipX); ImGui::SameLine();
    ImGui::Checkbox("Flip Y", &sr.flipY);

    if (ImGui::SmallButton("Remove##sr"))
        m_scene->RemoveComponent<SpriteRenderer>(m_selected);
}

void InspectorPanel::DrawCamera2D() {
    if (!ImGui::CollapsingHeader("Camera 2D", ImGuiTreeNodeFlags_DefaultOpen)) return;

    auto& cam = m_scene->GetComponent<Camera2D>(m_selected);
    ImGui::DragFloat("Zoom", &cam.zoom, 0.01f, 0.01f, 100.f);
    ImGui::Checkbox("Primary", &cam.isPrimary);

    if (ImGui::SmallButton("Remove##cam"))
        m_scene->RemoveComponent<Camera2D>(m_selected);
}

void InspectorPanel::DrawBoxCollider2D() {
    if (!ImGui::CollapsingHeader("Box Collider 2D", ImGuiTreeNodeFlags_DefaultOpen)) return;

    auto& bc = m_scene->GetComponent<BoxCollider2D>(m_selected);
    ImGui::DragFloat2("Offset##bc", glm::value_ptr(bc.offset), 0.01f);
    ImGui::DragFloat2("Size##bc",   glm::value_ptr(bc.size),   0.01f, 0.001f);
    ImGui::Checkbox("Is Trigger##bc", &bc.isTrigger);

    if (ImGui::SmallButton("Remove##bc"))
        m_scene->RemoveComponent<BoxCollider2D>(m_selected);
}

void InspectorPanel::DrawCircleCollider2D() {
    if (!ImGui::CollapsingHeader("Circle Collider 2D", ImGuiTreeNodeFlags_DefaultOpen)) return;

    auto& cc = m_scene->GetComponent<CircleCollider2D>(m_selected);
    ImGui::DragFloat2("Offset##cc", glm::value_ptr(cc.offset), 0.01f);
    ImGui::DragFloat("Radius##cc",  &cc.radius, 0.01f, 0.001f);
    ImGui::Checkbox("Is Trigger##cc", &cc.isTrigger);

    if (ImGui::SmallButton("Remove##cc"))
        m_scene->RemoveComponent<CircleCollider2D>(m_selected);
}

void InspectorPanel::DrawAddComponentMenu() {
    if (!ImGui::Button("Add Component"))
        return;
    ImGui::OpenPopup("##add_comp");

    if (ImGui::BeginPopup("##add_comp")) {
        if (!m_scene->HasComponent<SpriteRenderer>(m_selected))
            if (ImGui::MenuItem("Sprite Renderer"))
                m_scene->AddComponent<SpriteRenderer>(m_selected);
        if (!m_scene->HasComponent<Camera2D>(m_selected))
            if (ImGui::MenuItem("Camera 2D"))
                m_scene->AddComponent<Camera2D>(m_selected);
        if (!m_scene->HasComponent<BoxCollider2D>(m_selected))
            if (ImGui::MenuItem("Box Collider 2D"))
                m_scene->AddComponent<BoxCollider2D>(m_selected);
        if (!m_scene->HasComponent<CircleCollider2D>(m_selected))
            if (ImGui::MenuItem("Circle Collider 2D"))
                m_scene->AddComponent<CircleCollider2D>(m_selected);
        ImGui::EndPopup();
    }
}
