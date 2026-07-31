#pragma once
#include <string>
#include <memory>
#include "bridge/MonoGameProject.hpp"

class ConsolePanel;
class CliRunner;

class LaunchScreen {
public:
    explicit LaunchScreen(ConsolePanel* console);
    ~LaunchScreen();

    void OnImGuiRender();

    // Called by EditorLayer menu items
    void OpenNewProjectModal();
    void OpenBrowseModal();

private:
    void DrawNewProjectModal();
    void DrawBrowseModal();
    void DrawTemplateCard(ProjectTemplate t, bool selected);

    void BeginCreate();    // kicks off the dotnet new pipeline
    void OnProjectReady(); // called on main thread after dotnet new succeeds

    // Modal state
    bool m_showNew    = false;
    bool m_showBrowse = false;
    bool m_creating   = false;   // spinning while dotnet runs

    // Form fields
    char             m_nameBuffer[128] = {};
    char             m_pathBuffer[512] = {};
    GraphicsBackend  m_backend         = GraphicsBackend::OpenGL;
    ProjectTemplate  m_template        = ProjectTemplate::Empty2D;

    // Set when user clicks "Browse" folder button (Phase 0: text entry only)
    std::string m_browseResult;

    ConsolePanel*             m_console = nullptr;
    std::unique_ptr<CliRunner> m_cli;
    MonoGameProject            m_pendingProject;
};
