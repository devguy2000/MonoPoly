#pragma once
#include <imgui.h>

// Phase 0 stub – wired up fully in Phase 1 with the component inspector.
class InspectorPanel {
public:
    void OnImGuiRender() {
        ImGui::Begin("Inspector");
        ImGui::TextDisabled("(No selection)");
        ImGui::End();
    }
};
