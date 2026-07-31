#include "SceneHierarchyPanel.hpp"
#include "scene/SceneGraph.hpp"
#include "scene/Components.hpp"

#include <imgui.h>
#include <cstring>

SceneHierarchyPanel::SceneHierarchyPanel(SceneGraph* scene)
    : m_scene(scene) {}

// ---------------------------------------------------------------------------
void SceneHierarchyPanel::OnImGuiRender() {
    ImGui::Begin("Scene Hierarchy");

    if (!m_scene) {
        ImGui::TextDisabled("(No project open)");
        ImGui::End();
        return;
    }

    // Toolbar
    if (ImGui::Button("+ Entity")) {
        entt::entity e = m_scene->CreateEntity("Entity");
        m_selected = e;
    }
    ImGui::SameLine();
    if (ImGui::Button("+ Child") && m_selected != entt::null) {
        entt::entity e = m_scene->CreateChildEntity(m_selected, "Entity");
        m_selected = e;
    }
    ImGui::Separator();

    // Tree
    for (entt::entity root : m_scene->GetRootEntities())
        DrawEntityNode(root);

    // Click on empty space to deselect
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered())
        m_selected = entt::null;

    // Right-click on empty space
    if (ImGui::BeginPopupContextWindow("##hier_ctx", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
        if (ImGui::MenuItem("Create Empty"))
            m_selected = m_scene->CreateEntity("Entity");
        ImGui::EndPopup();
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
void SceneHierarchyPanel::DrawEntityNode(entt::entity e) {
    if (!m_scene->Registry().valid(e)) return;

    auto& tag = m_scene->GetComponent<TagComponent>(e);
    auto  children = m_scene->GetChildren(e);
    bool  hasChildren = !children.empty();

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_FramePadding;

    if (!hasChildren)
        flags |= ImGuiTreeNodeFlags_Leaf;
    if (m_selected == e)
        flags |= ImGuiTreeNodeFlags_Selected;

    // Inline rename
    if (m_renaming == e) {
        ImGui::SetNextItemWidth(-1.f);
        if (ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer),
            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            tag.name = m_renameBuffer;
            m_renaming = entt::null;
        }
        if (!ImGui::IsItemFocused()) m_renaming = entt::null;
    } else {
        bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)e, flags, "%s", tag.name.c_str());

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            m_selected = e;

        // Right-click context
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Rename")) {
                m_renaming = e;
                std::strncpy(m_renameBuffer, tag.name.c_str(), sizeof(m_renameBuffer) - 1);
                m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
            }
            if (ImGui::MenuItem("Create Child"))
                m_selected = m_scene->CreateChildEntity(e, "Entity");
            ImGui::Separator();
            if (ImGui::MenuItem("Delete")) {
                if (m_selected == e) m_selected = entt::null;
                m_scene->DestroyEntity(e);
                ImGui::EndPopup();
                if (opened) ImGui::TreePop();
                return;
            }
            ImGui::EndPopup();
        }

        // Drag-to-reparent
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("ENTITY", &e, sizeof(entt::entity));
            ImGui::Text("Move: %s", tag.name.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY")) {
                entt::entity dragged = *(const entt::entity*)payload->Data;
                if (dragged != e)
                    m_scene->ReparentEntity(dragged, e);
            }
            ImGui::EndDragDropTarget();
        }

        if (opened) {
            for (entt::entity child : children)
                DrawEntityNode(child);
            ImGui::TreePop();
        }
    }
}
