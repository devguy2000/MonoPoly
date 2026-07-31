#pragma once
#include <imgui.h>

// Phase 0 stub – Framebuffer + debug renderer wired in Phase 1.
class ViewportPanel {
public:
    void OnImGuiRender() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.f, 0.f});
        ImGui::Begin("Viewport");

        ImVec2 size = ImGui::GetContentRegionAvail();

        // Placeholder fill so the panel isn't visually empty
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 p1 = {p0.x + size.x, p0.y + size.y};
        dl->AddRectFilled(p0, p1, IM_COL32(30, 30, 36, 255));
        dl->AddText({p0.x + size.x * 0.5f - 80.f, p0.y + size.y * 0.5f - 8.f},
                    IM_COL32(80, 80, 100, 255),
                    "2D Viewport (Phase 1)");

        ImGui::Dummy(size); // reserve the region
        ImGui::End();
        ImGui::PopStyleVar();
    }
};
