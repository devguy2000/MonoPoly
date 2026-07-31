#include "AssetBrowserPanel.hpp"
#include "bridge/ProjectManager.hpp"

#include <imgui.h>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

static const std::vector<std::string> kImageExts = {".png",".jpg",".jpeg",".bmp"};

// ---------------------------------------------------------------------------
void AssetBrowserPanel::SetProject(const std::string& projectPath,
                                    const std::string& contentDir)
{
    m_projectPath = projectPath;
    m_contentDir  = contentDir;
    m_selected    = -1;
    ScanDirectory();
}

void AssetBrowserPanel::Refresh() {
    ScanDirectory();
}

void AssetBrowserPanel::ScanDirectory() {
    m_assets.clear();
    if (m_contentDir.empty() || !fs::exists(m_contentDir)) return;

    std::error_code ec;
    for (auto& entry : fs::directory_iterator(m_contentDir, ec)) {
        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (std::find(kImageExts.begin(), kImageExts.end(), ext) != kImageExts.end()) {
            Asset a;
            a.name     = entry.path().stem().string();
            a.filename = entry.path().filename().string();
            a.fullPath = entry.path().string();
            m_assets.push_back(std::move(a));
        }
    }
    std::sort(m_assets.begin(), m_assets.end(),
              [](auto& a, auto& b){ return a.name < b.name; });
}

// ---------------------------------------------------------------------------
void AssetBrowserPanel::OnImGuiRender() {
    ImGui::Begin("Assets");

    if (m_contentDir.empty()) {
        ImGui::TextDisabled("(No project open)");
        ImGui::End();
        return;
    }

    // Toolbar
    if (ImGui::Button("Refresh")) ScanDirectory();
    ImGui::SameLine();
    ImGui::TextDisabled("%zu assets", m_assets.size());

    ImGui::Separator();

    // Drop zone: drag external files onto this panel
    DrawDropTarget();

    // Asset list
    for (int i = 0; i < (int)m_assets.size(); ++i) {
        const auto& a = m_assets[i];
        bool sel = (m_selected == i);

        if (ImGui::Selectable(a.filename.c_str(), sel,
            ImGuiSelectableFlags_AllowDoubleClick))
        {
            m_selected = i;
            if (ImGui::IsMouseDoubleClicked(0) && m_onSelect)
                m_onSelect(a.name, a.fullPath);
        }

        // Drag source: drag content name onto SpriteRenderer texture field
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("ASSET_NAME", a.name.c_str(), a.name.size() + 1);
            ImGui::Text("Sprite: %s", a.name.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", a.fullPath.c_str());
    }

    ImGui::End();
}

void AssetBrowserPanel::DrawDropTarget() {
    ImGui::Text("Drop images here to import:");
    ImGui::InvisibleButton("##drop_zone", {-1.f, 32.f});

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("FILES")) {
            const char* paths = static_cast<const char*>(p->Data);
            ProjectManager::Get().AddImageToContent(paths);
            ScanDirectory();
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::Separator();
}
