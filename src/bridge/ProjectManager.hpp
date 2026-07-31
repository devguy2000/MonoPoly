#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>

class ConsolePanel;
class CliRunner;

class ProjectManager {
public:
    static ProjectManager& Get();

    void Init(ConsolePanel* console);
    void SetProject(const std::string& projectPath, const std::string& csprojPath);
    void Close();

    // Build / run
    void Build(std::function<void(int)> onDone = {});
    void Run();
    void OpenInVSCode();
    void OpenInVisualStudio();

    // Content pipeline
    void ScanContent();
    void AddImageToContent(const std::string& absImagePath);
    const std::vector<std::string>& ContentAssets() const { return m_contentNames; }
    std::string ContentNameToFullPath(const std::string& name) const;

    bool HasProject() const { return !m_projectPath.empty(); }
    bool IsBuilding()  const { return m_building; }
    const std::string& ProjectPath() const { return m_projectPath; }
    const std::string& ContentDir()  const { return m_contentDir; }
    const std::string& ProjectName() const { return m_projectName; }

private:
    ProjectManager() = default;
    bool        IsInMgcb(const std::string& filename) const;
    std::string FindExe() const;

    std::string m_projectPath;
    std::string m_csprojPath;
    std::string m_contentDir;
    std::string m_projectName;
    bool        m_building = false;

    ConsolePanel*              m_console = nullptr;
    std::unique_ptr<CliRunner> m_cli;
    std::vector<std::string>   m_contentNames; // no extension
};
