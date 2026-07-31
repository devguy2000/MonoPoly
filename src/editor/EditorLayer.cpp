#include "EditorLayer.hpp"

#include <imgui.h>
#include <string>

#include "panels/ConsolePanel.hpp"
#include "panels/SceneHierarchyPanel.hpp"
#include "panels/InspectorPanel.hpp"
#include "panels/ViewportPanel.hpp"
#include "launch/LaunchScreen.hpp"
#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "core/Application.hpp"

EditorLayer::EditorLayer()  = default;
EditorLayer::~EditorLayer() = default;

// ---------------------------------------------------------------------------
void EditorLayer::OnAttach() {
    m_console    = std::make_unique<ConsolePanel>();
    m_hierarchy  = std::make_unique<SceneHierarchyPanel>();
    m_inspector  = std::make_unique<InspectorPanel>();
    m_viewport   = std::make_unique<ViewportPanel>();
    m_launchScreen = std::make_unique<LaunchScreen>(m_console.get());

    // Wire CLI output from worker threads into the console panel
    EventBus::Get().Subscribe<CliOutputEvent>([this](const CliOutputEvent& e) {
        if (e.isError)
            m_console->LogError(e.text);
        else
            m_console->Log(e.text);
    });

    EventBus::Get().Subscribe<ProjectCreatedEvent>([this](const ProjectCreatedEvent& e) {
        m_projectOpen = true;
        m_console->LogSuccess("Project created: " + e.name + "  →  " + e.path);
    });

    EventBus::Get().Subscribe<ProjectOpenedEvent>([this](const ProjectOpenedEvent& e) {
        m_projectOpen = true;
        m_console->LogSuccess("Project opened: " + e.name);
    });

    m_console->LogInfo("MonoPoly IDE initialised. Welcome.");
    m_console->LogInfo("Use  File → New Project  or  File → Open Project  to begin.");
}

// ---------------------------------------------------------------------------
void EditorLayer::OnDetach() {
    m_launchScreen.reset();
    m_console.reset();
    m_hierarchy.reset();
    m_inspector.reset();
    m_viewport.reset();
}

// ---------------------------------------------------------------------------
void EditorLayer::OnImGuiRender(float /*dt*/) {
    DrawMenuBar();

    // Always render all docked panels so their layout slots are claimed.
    m_hierarchy->OnImGuiRender();
    m_viewport->OnImGuiRender();
    m_inspector->OnImGuiRender();
    m_console->OnImGuiRender();

    // Launch screen modal (shown until a project is open)
    if (!m_projectOpen)
        m_launchScreen->OnImGuiRender();

    if (m_showDemoWindow)
        ImGui::ShowDemoWindow(&m_showDemoWindow);
}

// ---------------------------------------------------------------------------
void EditorLayer::DrawMenuBar() {
    // The MenuBar belongs to the DockHost window pushed by Application::BeginFrame()
    if (!ImGui::BeginMenuBar()) return;

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Project...",  "Ctrl+N")) m_launchScreen->OpenNewProjectModal();
        if (ImGui::MenuItem("Open Project...", "Ctrl+O")) m_launchScreen->OpenBrowseModal();
        ImGui::Separator();
        if (ImGui::MenuItem("Save Scene",      "Ctrl+S")) { /* Phase 1 */ }
        ImGui::Separator();
        if (ImGui::MenuItem("Quit",            "Alt+F4")) Application::Get().Quit();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {}
        if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {}
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("ImGui Demo",      nullptr, m_showDemoWindow))
            m_showDemoWindow = !m_showDemoWindow;
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Project")) {
        bool hasProj = m_projectOpen;
        if (ImGui::MenuItem("Build",           "F5", false, hasProj)) { /* Phase 2 */ }
        if (ImGui::MenuItem("Open in VS Code", nullptr, false, hasProj)) { /* Phase 2 */ }
        if (ImGui::MenuItem("Open in Visual Studio", nullptr, false, hasProj)) { /* Phase 2 */ }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Run")) {
        bool hasProj = m_projectOpen;
        if (ImGui::MenuItem("Play",  "F5",  false, hasProj)) { /* Phase 2 */ }
        if (ImGui::MenuItem("Pause", "F6",  false, false))   {}
        if (ImGui::MenuItem("Stop",  "F7",  false, false))   {}
        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

// ---------------------------------------------------------------------------
void EditorLayer::OpenProject(const std::string& /*path*/) {
    m_projectOpen = true;
}
