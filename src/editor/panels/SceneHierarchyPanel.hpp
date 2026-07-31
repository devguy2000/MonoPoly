#pragma once
#include <imgui.h>
#include <string>

// Phase 0 stub – wired up fully in Phase 1 with the entt SceneGraph.
class SceneHierarchyPanel {
public:
    void OnImGuiRender() {
        ImGui::Begin("Scene Hierarchy");
        ImGui::TextDisabled("(No project open)");
        ImGui::End();
    }
};
