#include "EditorLayer.hpp"

#include <imgui.h>
#include <string>
#include <filesystem>

#include "panels/ConsolePanel.hpp"
#include "panels/SceneHierarchyPanel.hpp"
#include "panels/InspectorPanel.hpp"
#include "panels/ViewportPanel.hpp"
#include "panels/AssetBrowserPanel.hpp"
#include "launch/LaunchScreen.hpp"
#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "core/Application.hpp"
#include "scene/SceneGraph.hpp"
#include "scene/SceneSerializer.hpp"
#include "scene/Components.hpp"
#include "bridge/ProjectManager.hpp"
#include "codegen/GameCodeGen.hpp"

namespace fs = std::filesystem;

EditorLayer::EditorLayer()  = default;
EditorLayer::~EditorLayer() = default;

// ---------------------------------------------------------------------------
void EditorLayer::OnAttach() {
    m_scene        = std::make_unique<SceneGraph>();
    m_console      = std::make_unique<ConsolePanel>();
    m_hierarchy    = std::make_unique<SceneHierarchyPanel>(m_scene.get());
    m_inspector    = std::make_unique<InspectorPanel>(m_scene.get());
    m_viewport     = std::make_unique<ViewportPanel>();
    m_assetBrowser = std::make_unique<AssetBrowserPanel>();
    m_viewport->SetScene(m_scene.get());
    m_launchScreen = std::make_unique<LaunchScreen>(m_console.get());

    ProjectManager::Get().Init(m_console.get());

    // Wire asset browser → inspector texture assignment
    m_assetBrowser->SetOnSelect([this](const std::string& name, const std::string& fullPath) {
        auto e = m_hierarchy->SelectedEntity();
        if (e == entt::null || !m_scene->Registry().valid(e)) return;
        if (!m_scene->HasComponent<SpriteRenderer>(e))
            m_scene->AddComponent<SpriteRenderer>(e);
        auto& sr   = m_scene->GetComponent<SpriteRenderer>(e);
        sr.texturePath = name;
        m_inspector->SetProjectPath(m_projectPath);
        // Trigger texture load by refreshing the inspector on next frame
        (void)fullPath;
    });

    // EventBus subscriptions
    EventBus::Get().Subscribe<CliOutputEvent>([this](const CliOutputEvent& e) {
        if (e.isError) m_console->LogError(e.text);
        else           m_console->Log(e.text);
    });

    EventBus::Get().Subscribe<ProjectCreatedEvent>([this](const ProjectCreatedEvent& e) {
        OnProjectReady(e.path, e.csprojPath, e.name);
        NewScene();
        m_console->LogSuccess("Project created: " + e.name + " → " + e.path);
    });

    EventBus::Get().Subscribe<ProjectOpenedEvent>([this](const ProjectOpenedEvent& e) {
        OnProjectReady(e.path, e.csprojPath, e.name);
        SceneSerializer ser(*m_scene);
        if (fs::exists(m_scenePath))
            ser.LoadFromFile(m_scenePath);
        else
            NewScene();
        m_console->LogSuccess("Project opened: " + e.name);
    });

    EventBus::Get().Subscribe<AssetImportedEvent>([this](const AssetImportedEvent& e) {
        m_assetBrowser->Refresh();
        m_console->LogSuccess("Asset imported: " + e.contentName);
    });

    m_console->LogInfo("MonoPoly IDE initialised. Welcome.");
    m_console->LogInfo("File → New Project  or  File → Open Project  to begin.");
}

// ---------------------------------------------------------------------------
void EditorLayer::OnProjectReady(const std::string& projectPath,
                                  const std::string& csprojPath,
                                  const std::string& projectName)
{
    m_projectOpen = true;
    m_projectPath = projectPath;
    m_scenePath   = (fs::path(projectPath) / "Scene.mpscene").string();

    ProjectManager::Get().SetProject(projectPath, csprojPath);

    // Patch Game1.cs to our runtime scene reader
    if (GameCodeGen::GenerateGame1(projectPath, projectName))
        m_console->LogSuccess("Game1.cs generated for runtime scene loading.");
    else
        m_console->LogError("Could not write Game1.cs.");

    m_hierarchy->SetScene(m_scene.get());
    m_inspector->SetScene(m_scene.get());
    m_inspector->SetProjectPath(projectPath);
    m_viewport->SetScene(m_scene.get());
    m_assetBrowser->SetProject(projectPath, ProjectManager::Get().ContentDir());
}

// ---------------------------------------------------------------------------
void EditorLayer::OnDetach() {
    m_launchScreen.reset();
    m_assetBrowser.reset();
    m_viewport.reset();
    m_inspector.reset();
    m_hierarchy.reset();
    m_console.reset();
    m_scene.reset();
    ProjectManager::Get().Close();
}

// ---------------------------------------------------------------------------
void EditorLayer::NewScene() {
    m_scene->Clear();
    entt::entity cam = m_scene->CreateEntity("Main Camera");
    m_scene->AddComponent<Camera2D>(cam);
    m_hierarchy->SetSelected(entt::null);
    m_inspector->SetSelected(entt::null);
}

void EditorLayer::SaveScene() {
    if (m_scenePath.empty()) return;
    SceneSerializer ser(*m_scene);
    if (ser.SaveToFile(m_scenePath))
        m_console->LogSuccess("Scene saved → " + m_scenePath);
    else
        m_console->LogError("Scene save failed: " + m_scenePath);
}

// ---------------------------------------------------------------------------
void EditorLayer::OnImGuiRender(float dt) {
    DrawMenuBar();

    if (m_hierarchy)
        m_inspector->SetSelected(m_hierarchy->SelectedEntity());

    m_hierarchy->OnImGuiRender();
    m_viewport->OnImGuiRender(dt);
    m_inspector->OnImGuiRender();
    m_console->OnImGuiRender();
    m_assetBrowser->OnImGuiRender();

    if (!m_projectOpen)
        m_launchScreen->OnImGuiRender();

    if (m_showDemoWindow)
        ImGui::ShowDemoWindow(&m_showDemoWindow);
}

// ---------------------------------------------------------------------------
void EditorLayer::DrawMenuBar() {
    if (!ImGui::BeginMenuBar()) return;

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Project...",  "Ctrl+N")) m_launchScreen->OpenNewProjectModal();
        if (ImGui::MenuItem("Open Project...", "Ctrl+O")) m_launchScreen->OpenBrowseModal();
        ImGui::Separator();
        if (ImGui::MenuItem("Save Scene", "Ctrl+S", false, m_projectOpen)) SaveScene();
        if (ImGui::MenuItem("New Scene",  nullptr,  false, m_projectOpen)) NewScene();
        ImGui::Separator();
        if (ImGui::MenuItem("Quit", "Alt+F4")) Application::Get().Quit();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {}
        if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {}
        ImGui::Separator();
        if (ImGui::MenuItem("Create Entity", nullptr, false, m_projectOpen)) {
            if (m_scene) m_hierarchy->SetSelected(m_scene->CreateEntity("Entity"));
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("ImGui Demo", nullptr, m_showDemoWindow))
            m_showDemoWindow = !m_showDemoWindow;
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Project")) {
        bool busy = ProjectManager::Get().IsBuilding();
        if (ImGui::MenuItem("Build",                "F7",  false, m_projectOpen && !busy))
            ProjectManager::Get().Build();
        if (ImGui::MenuItem("Open in VS Code",      nullptr, false, m_projectOpen))
            ProjectManager::Get().OpenInVSCode();
        if (ImGui::MenuItem("Open in Visual Studio",nullptr, false, m_projectOpen))
            ProjectManager::Get().OpenInVisualStudio();
        ImGui::Separator();
        if (ImGui::MenuItem("Import Image Asset...", nullptr, false, m_projectOpen)) {
            // User pastes an absolute path in console — real file dialog in Phase 3
            m_console->LogInfo("Paste image path in console, or drop file onto Assets panel.");
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Run")) {
        bool busy = ProjectManager::Get().IsBuilding();
        if (ImGui::MenuItem("Play (Build & Run)", "F5", false, m_projectOpen && !busy)) {
            SaveScene();
            ProjectManager::Get().Run();
        }
        if (ImGui::MenuItem("Build Only", "F7", false, m_projectOpen && !busy))
            ProjectManager::Get().Build();
        ImGui::EndMenu();
    }

    // Building indicator
    if (ProjectManager::Get().IsBuilding()) {
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 90.f);
        ImGui::TextColored({1.f,0.8f,0.2f,1.f}, "Building...");
    }

    ImGui::EndMenuBar();
}

// ---------------------------------------------------------------------------
void EditorLayer::OpenProject(const std::string& path) {
    m_projectPath = path;
    m_projectOpen = true;
}
