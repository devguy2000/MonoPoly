#include "ProjectManager.hpp"
#include "CliRunner.hpp"
#include "editor/panels/ConsolePanel.hpp"
#include "core/Events.hpp"
#include "core/EventBus.hpp"

#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "Shell32.lib")
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace fs = std::filesystem;

ProjectManager& ProjectManager::Get() {
    static ProjectManager inst;
    return inst;
}

void ProjectManager::Init(ConsolePanel* console) {
    m_console = console;
    if (!m_cli) m_cli = std::make_unique<CliRunner>();
}

void ProjectManager::SetProject(const std::string& projectPath, const std::string& csprojPath) {
    m_projectPath = projectPath;
    m_csprojPath  = csprojPath;
    m_contentDir  = (fs::path(projectPath) / "Content").string();
    m_projectName = fs::path(csprojPath).stem().string();
    ScanContent();
}

void ProjectManager::Close() {
    m_projectPath.clear();
    m_csprojPath.clear();
    m_contentDir.clear();
    m_projectName.clear();
    m_contentNames.clear();
}

// ---------------------------------------------------------------------------
void ProjectManager::Build(std::function<void(int)> onDone) {
    if (!HasProject()) return;
    m_building = true;
    m_console->LogInfo("Building " + m_projectName + "...");
    m_cli->BuildProject(m_csprojPath, [this, onDone](int ec, std::string) {
        m_building = false;
        if (ec == 0) m_console->LogSuccess("Build succeeded.");
        else         m_console->LogError("Build FAILED (exit " + std::to_string(ec) + ").");
        if (onDone) onDone(ec);
    });
}

void ProjectManager::Run() {
    Build([this](int ec) {
        if (ec != 0) return;
        std::string exe = FindExe();
        if (exe.empty()) {
            m_console->LogError("Executable not found after build.");
            return;
        }
        m_console->LogSuccess("Launching: " + exe);
        std::wstring wExe(exe.begin(), exe.end());
        std::wstring wDir(m_projectPath.begin(), m_projectPath.end());
        ShellExecuteW(nullptr, L"open", wExe.c_str(), nullptr, wDir.c_str(), SW_SHOW);
    });
}

void ProjectManager::OpenInVSCode() {
    if (!HasProject()) return;
    // Launch VS Code via shell – cmd /c handles PATH lookup for 'code'
    std::wstring wPath(m_projectPath.begin(), m_projectPath.end());
    std::wstring args = L"/c code \"" + wPath + L"\"";
    ShellExecuteW(nullptr, L"open", L"cmd.exe", args.c_str(), nullptr, SW_HIDE);
    m_console->LogInfo("Opening in VS Code: " + m_projectPath);
}

void ProjectManager::OpenInVisualStudio() {
    if (m_csprojPath.empty()) return;
    std::wstring wCsproj(m_csprojPath.begin(), m_csprojPath.end());
    ShellExecuteW(nullptr, L"open", wCsproj.c_str(), nullptr, nullptr, SW_SHOW);
    m_console->LogInfo("Opening in Visual Studio: " + m_csprojPath);
}

// ---------------------------------------------------------------------------
void ProjectManager::ScanContent() {
    m_contentNames.clear();
    if (m_contentDir.empty() || !fs::exists(m_contentDir)) return;

    static const std::vector<std::string> kExts = {".png",".jpg",".jpeg",".bmp",".gif"};
    std::error_code ec;
    for (auto& entry : fs::directory_iterator(m_contentDir, ec)) {
        auto ext = entry.path().extension().string();
        // lowercase ext
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (std::find(kExts.begin(), kExts.end(), ext) != kExts.end())
            m_contentNames.push_back(entry.path().stem().string());
    }
}

void ProjectManager::AddImageToContent(const std::string& absImagePath) {
    fs::path src(absImagePath);
    if (!fs::exists(src)) return;

    std::string filename = src.filename().string();
    std::string stem     = src.stem().string();

    // Copy image to Content/ if not already there
    fs::path dst = fs::path(m_contentDir) / filename;
    if (!fs::exists(dst)) {
        std::error_code ec;
        fs::copy(src, dst, ec);
        if (ec) { m_console->LogError("Copy failed: " + ec.message()); return; }
    }

    // Append to Content.mgcb if not listed
    if (!IsInMgcb(filename)) {
        fs::path mgcb = fs::path(m_contentDir) / "Content.mgcb";
        std::ofstream f(mgcb.string(), std::ios::app);
        f << "\n#begin " << filename
          << "\n/importer:TextureImporter"
          << "\n/processor:TextureProcessor"
          << "\n/processorParam:ColorKeyColor=255,0,255,255"
          << "\n/processorParam:ColorKeyEnabled=True"
          << "\n/processorParam:GenerateMipmaps=False"
          << "\n/processorParam:PremultiplyAlpha=True"
          << "\n/processorParam:ResizeToPowerOfTwo=False"
          << "\n/processorParam:MakeSquare=False"
          << "\n/processorParam:TextureFormat=Color"
          << "\n/build:" << filename << "\n";
        m_console->LogSuccess("Added to Content.mgcb: " + filename);
    }

    ScanContent();
    EventBus::Get().EmitDeferred(AssetImportedEvent{dst.string(), stem});
}

std::string ProjectManager::ContentNameToFullPath(const std::string& name) const {
    static const std::vector<std::string> kExts = {".png",".jpg",".jpeg",".bmp"};
    for (auto& ext : kExts) {
        fs::path p = fs::path(m_contentDir) / (name + ext);
        if (fs::exists(p)) return p.string();
    }
    return {};
}

bool ProjectManager::IsInMgcb(const std::string& filename) const {
    fs::path mgcb = fs::path(m_contentDir) / "Content.mgcb";
    if (!fs::exists(mgcb)) return false;
    std::ifstream f(mgcb.string());
    std::string line;
    while (std::getline(f, line))
        if (line.find(filename) != std::string::npos) return true;
    return false;
}

std::string ProjectManager::FindExe() const {
    if (m_projectPath.empty() || m_projectName.empty()) return {};
    fs::path base(m_projectPath);
    std::error_code ec;
    for (auto& entry : fs::recursive_directory_iterator(base / "bin", ec)) {
        if (entry.path().extension() == ".exe" &&
            entry.path().stem().string() == m_projectName)
            return entry.path().string();
    }
    return {};
}
