#include "SolutionExplorerPanel.hpp"
#include <imgui.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "Shell32.lib")
#include <vector>
#include <algorithm>

// ---------------------------------------------------------------------------
bool SolutionExplorerPanel::ShouldSkip(const fs::path& p) {
    const std::string n = p.filename().string();
    return n.empty() || n[0] == '.' || n == "obj" || n == "bin"
                     || n == "build" || n == "vendor" || n == ".vs";
}

// ---------------------------------------------------------------------------
void SolutionExplorerPanel::OnImGuiRender() {
    ImGui::Begin("Solution");

    if (m_projectPath.empty()) {
        ImGui::TextDisabled("No project open.");
        ImGui::End();
        return;
    }

    ImGui::TextColored({0.6f, 0.85f, 1.f, 1.f}, "%s",
        fs::path(m_projectPath).filename().string().c_str());
    ImGui::Separator();
    DrawEntries(m_projectPath);

    ImGui::End();
}

// ---------------------------------------------------------------------------
void SolutionExplorerPanel::DrawEntries(const fs::path& dir) {
    if (!fs::exists(dir)) return;

    std::vector<fs::path> dirs, files;
    try {
        for (auto& de : fs::directory_iterator(dir)) {
            if (ShouldSkip(de.path())) continue;
            if (de.is_directory()) dirs.push_back(de.path());
            else                   files.push_back(de.path());
        }
    } catch (...) { return; }

    std::sort(dirs.begin(),  dirs.end());
    std::sort(files.begin(), files.end());

    for (auto& d : dirs) {
        bool open = ImGui::TreeNodeEx(d.filename().string().c_str(),
                                      ImGuiTreeNodeFlags_SpanFullWidth);
        if (open) { DrawEntries(d); ImGui::TreePop(); }
    }

    for (auto& f : files) {
        const std::string name = f.filename().string();
        const std::string ext  = f.extension().string();

        ImVec4 col = {0.88f, 0.88f, 0.88f, 1.f};
        if      (ext == ".cs")      col = {0.45f, 0.90f, 0.45f, 1.f};
        else if (ext == ".csproj")  col = {0.90f, 0.75f, 0.30f, 1.f};
        else if (ext == ".mpscene") col = {1.00f, 0.80f, 0.35f, 1.f};
        else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
                                    col = {0.65f, 0.65f, 1.00f, 1.f};
        else if (ext == ".mgcb")    col = {0.80f, 0.55f, 0.90f, 1.f};

        bool selected = (f == m_selectedFile);
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        bool open = ImGui::TreeNodeEx(name.c_str(),
            ImGuiTreeNodeFlags_Leaf |
            ImGuiTreeNodeFlags_SpanFullWidth |
            (selected ? ImGuiTreeNodeFlags_Selected : 0));
        ImGui::PopStyleColor();

        if (open) ImGui::TreePop();

        if (ImGui::IsItemClicked())
            m_selectedFile = f;

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            if (m_onFileOpen) m_onFileOpen(f.string());
            ShellExecuteW(nullptr, L"open", f.wstring().c_str(),
                          nullptr, nullptr, SW_SHOW);
        }
    }
}
