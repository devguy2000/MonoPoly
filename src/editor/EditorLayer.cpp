#include "EditorLayer.hpp"

#include <imgui.h>
#include <string>
#include <filesystem>

#include "panels/ConsolePanel.hpp"
#include "panels/SceneHierarchyPanel.hpp"
#include "panels/InspectorPanel.hpp"
#include "panels/ViewportPanel.hpp"
#include "launch/LaunchScreen.hpp"
#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "core/Application.hpp"
#include "scene/SceneGraph.hpp"
#include "scene/SceneSerializer.hpp"
#include "scene/Components.hpp"

namespace fs = std::filesystem;

EditorLayer::EditorLayer()  = default;
EditorLayer::~EditorLayer() = default;

// ---------------------------------------------------------------------------
void EditorLayer::OnAttach() {
    m_scene      = std::make_unique<SceneGraph>();
    m_console    = std::make_unique<ConsolePanel>();
    m_hierarchy  = std::make_unique<SceneHierarchyPanel>(m_scene.get());
    m_inspector  = std::make_unique<InspectorPanel>(m_scene.get());
    m_viewport   = std::make_unique<ViewportPanel>();
    m_viewport->SetScene(m_scene.get());
    m_launchScreen = std::make_unique<LaunchScreen>(m_console.get());

    EventBus::Get().Subscribe<CliOutputEvent>([this](const CliOutputEvent& e) {
        if (e.isError) m_console->LogError(e.text);
        else           m_console->Log(e.text);
    });

    EventBus::Get().Subscribe<ProjectCreatedEvent>([this](const ProjectCreatedEvent& e) {
        m_projectOpen = true;
        m_projectPath = e.path;
        m_scenePath   = (fs::path(e.path) / "Scene.mpscene").string();
        m_hierarchy->SetScene(m_scene.get());
        m_inspector->SetScene(m_scene.get());
        m_viewport->SetScene(m_scene.get());
        NewScene();
        m_console->LogSuccess("Project created: " + e.name + "  →  " + e.path);
    });

    EventBus::Get().Subscribe<ProjectOpenedEvent>([this](const ProjectOpenedEvent& e) {
        m_projectOpen = true;
        m_projectPath = e.path;
        m_scenePath   = (fs::path(e.path) / "Scene.mpscene").string();
        m_hierarchy->SetScene(m_scene.get());
        m_inspector->SetScene(m_scene.get());
        m_viewport->SetScene(m_scene.get());

        SceneSerializer ser(*m_scene);
        if (fs::exists(m_scenePath)) {
            ser.LoadFromFile(m_scenePath);
            m_console->LogSuccess("Scene loaded: " + m_scenePath);
        } else {
            NewScene();
        }
        m_console->LogSuccess("Project opened: " + e.name);
    });

    m_console->LogInfo("MonoPoly IDE initialised. Welcome.");
    m_console->LogInfo("Use  File → New Project  or  File → Open Project  to begin.");
}

// ---------------------------------------------------------------------------
void EditorLayer::OnDetach() {
    m_launchScreen.reset();
    m_viewport.reset();
    m_inspector.reset();
    m_hierarchy.reset();
    m_console.reset();
    m_scene.reset();
}

// ---------------------------------------------------------------------------
void EditorLayer::NewScene() {
    m_scene->Clear();
    // Default: one camera entity
    entt::entity cam = m_scene->CreateEntity("Main Camera");
    m_scene->AddComponent<Camera2D>(cam);
    m_hierarchy->SetSelected(entt::null);
    m_inspector->SetSelected(entt::null);
}

void EditorLayer::SaveScene() {
    if (m_scenePath.empty()) return;
    SceneSerializer ser(*m_scene);
    if (ser.SaveToFile(m_scenePath))
        m_console->LogSuccess("Scene saved: " + m_scenePath);
    else
        m_console->LogError("Failed to save scene: " + m_scenePath);
}

// ---------------------------------------------------------------------------
void EditorLayer::OnImGuiRender(float dt) {
    DrawMenuBar();

    // Sync hierarchy → inspector selection
    if (m_hierarchy)
        m_inspector->SetSelected(m_hierarchy->SelectedEntity());

    m_hierarchy->OnImGuiRender();
    m_viewport->OnImGuiRender(dt);
    m_inspector->OnImGuiRender();
    m_console->OnImGuiRender();

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
            if (m_scene) {
                entt::entity e = m_scene->CreateEntity("Entity");
                m_hierarchy->SetSelected(e);
            }
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("ImGui Demo", nullptr, m_showDemoWindow))
            m_showDemoWindow = !m_showDemoWindow;
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Project")) {
        if (ImGui::MenuItem("Build",                nullptr, false, m_projectOpen)) {}
        if (ImGui::MenuItem("Open in VS Code",      nullptr, false, m_projectOpen)) {}
        if (ImGui::MenuItem("Open in Visual Studio",nullptr, false, m_projectOpen)) {}
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Run")) {
        if (ImGui::MenuItem("Play",  "F5",  false, m_projectOpen)) {}
        if (ImGui::MenuItem("Pause", "F6",  false, false)) {}
        if (ImGui::MenuItem("Stop",  "F7",  false, false)) {}
        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

// ---------------------------------------------------------------------------
void EditorLayer::OpenProject(const std::string& path) {
    m_projectPath = path;
    m_projectOpen = true;
}
