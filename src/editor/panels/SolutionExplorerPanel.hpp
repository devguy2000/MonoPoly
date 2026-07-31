#pragma once
#include <string>
#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

class SolutionExplorerPanel {
public:
    void SetProject(const std::string& projectPath) { m_projectPath = projectPath; }
    void SetOnFileOpen(std::function<void(const std::string&)> cb) { m_onFileOpen = std::move(cb); }
    void OnImGuiRender();

private:
    void DrawEntries(const fs::path& dir);
    static bool ShouldSkip(const fs::path& p);

    std::string m_projectPath;
    fs::path    m_selectedFile;
    std::function<void(const std::string&)> m_onFileOpen;
};
