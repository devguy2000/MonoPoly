#include "LaunchScreen.hpp"

#include <imgui.h>
#include <filesystem>
#include <cstring>

#include "editor/panels/ConsolePanel.hpp"
#include "bridge/CliRunner.hpp"
#include "core/EventBus.hpp"
#include "core/Events.hpp"

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
LaunchScreen::LaunchScreen(ConsolePanel* console)
    : m_console(console)
    , m_cli(std::make_unique<CliRunner>())
{
    // Default path: user's Documents\MonoPolyProjects
    const char* docs = std::getenv("USERPROFILE");
    if (docs) {
        std::string defaultPath = std::string(docs) + "\\Documents\\MonoPolyProjects";
        std::strncpy(m_pathBuffer, defaultPath.c_str(), sizeof(m_pathBuffer) - 1);
    }

    // Show the new-project modal on first launch (no ini present)
    m_showNew = true;
}
LaunchScreen::~LaunchScreen() = default;

// ---------------------------------------------------------------------------
void LaunchScreen::OpenNewProjectModal()  { m_showNew    = true; }
void LaunchScreen::OpenBrowseModal()      { m_showBrowse = true; }

// ---------------------------------------------------------------------------
void LaunchScreen::OnImGuiRender() {
    if (m_showNew)    DrawNewProjectModal();
    if (m_showBrowse) DrawBrowseModal();
}

// ---------------------------------------------------------------------------
void LaunchScreen::DrawNewProjectModal() {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({680.f, 520.f}, ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("New Project##modal", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::TextDisabled("MonoPoly — New MonoGame Project");
        ImGui::Separator();
        ImGui::Spacing();

        // --- Project name -------------------------------------------------
        ImGui::Text("Project Name");
        ImGui::SetNextItemWidth(-1.f);
        ImGui::InputTextWithHint("##pname", "e.g. MyPlatformer", m_nameBuffer, sizeof(m_nameBuffer));

        ImGui::Spacing();

        // --- Location ------------------------------------------------------
        ImGui::Text("Location");
        ImGui::SetNextItemWidth(-1.f);
        ImGui::InputText("##ploc", m_pathBuffer, sizeof(m_pathBuffer));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // --- Template cards -----------------------------------------------
        ImGui::Text("Template");
        ImGui::Spacing();

        const ProjectTemplate templates[] = {
            ProjectTemplate::Empty2D,
            ProjectTemplate::Platformer2D,
            ProjectTemplate::RTS2D,
            ProjectTemplate::Roguelike2D
        };
        for (int i = 0; i < 4; ++i) {
            if (i > 0) ImGui::SameLine();
            DrawTemplateCard(templates[i], m_template == templates[i]);
            if (ImGui::IsItemClicked()) m_template = templates[i];
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // --- Graphics API -------------------------------------------------
        ImGui::Text("Graphics API");
        ImGui::SameLine();
        ImGui::TextDisabled("(default: OpenGL)");
        ImGui::Spacing();

        auto radioBtn = [&](const char* label, GraphicsBackend b) {
            if (ImGui::RadioButton(label, m_backend == b)) m_backend = b;
        };
        radioBtn("OpenGL",  GraphicsBackend::OpenGL);  ImGui::SameLine();
        radioBtn("DirectX", GraphicsBackend::DirectX); ImGui::SameLine();
        radioBtn("Vulkan",  GraphicsBackend::Vulkan);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // --- Buttons -------------------------------------------------------
        bool canCreate = (m_nameBuffer[0] != '\0') && (m_pathBuffer[0] != '\0') && !m_creating;

        if (m_creating) {
            ImGui::TextColored({0.4f, 0.8f, 1.f, 1.f}, "Creating project...");
            ImGui::SameLine();
            // Spinner (simple frame-count dot animation)
            int dots = (int)(ImGui::GetTime() * 3.0) % 4;
            char spinner[] = "   ";
            for (int i = 0; i < dots; ++i) spinner[i] = '.';
            ImGui::TextUnformatted(spinner);
        }

        ImGui::BeginDisabled(!canCreate);
        if (ImGui::Button("Create Project", {140.f, 0.f}))
            BeginCreate();
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel", {80.f, 0.f})) {
            m_showNew  = false;
            m_creating = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // Open the modal if flagged (must call OpenPopup before BeginPopupModal)
    if (m_showNew)
        ImGui::OpenPopup("New Project##modal");
}

// ---------------------------------------------------------------------------
void LaunchScreen::DrawTemplateCard(ProjectTemplate t, bool selected) {
    const float cardW = 135.f;
    const float cardH = 80.f;

    ImVec4 bg = selected
        ? ImVec4{0.24f, 0.52f, 0.88f, 1.f}
        : ImVec4{0.18f, 0.18f, 0.22f, 1.f};

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.f);

    // Use an invisible button + manual draw to make the whole card clickable
    std::string id = std::string("##card_") + TemplateName(t);
    ImGui::BeginChild(id.c_str(), {cardW, cardH}, false, ImGuiWindowFlags_NoScrollbar);

    ImGui::Spacing();
    // Icon placeholder (unicode blocks as cheap stand-in)
    const char* icon = "[ ]";
    switch (t) {
    case ProjectTemplate::Platformer2D: icon = "[P]"; break;
    case ProjectTemplate::RTS2D:        icon = "[R]"; break;
    case ProjectTemplate::Roguelike2D:  icon = "[@]"; break;
    default:                            icon = "[ ]"; break;
    }
    float textW = ImGui::CalcTextSize(icon).x;
    ImGui::SetCursorPosX((cardW - textW) * 0.5f);
    ImGui::TextUnformatted(icon);

    ImGui::Spacing();
    const char* label = TemplateName(t);
    textW = ImGui::CalcTextSize(label).x;
    ImGui::SetCursorPosX((cardW - textW) * 0.5f);
    ImGui::TextUnformatted(label);

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

// ---------------------------------------------------------------------------
void LaunchScreen::DrawBrowseModal() {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({540.f, 220.f}, ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Open Project##browse", nullptr,
        ImGuiWindowFlags_NoResize))
    {
        ImGui::Text("Project Root Folder");
        ImGui::SetNextItemWidth(-1.f);
        ImGui::InputText("##bpath", m_pathBuffer, sizeof(m_pathBuffer));
        ImGui::TextDisabled("Enter the folder containing a .csproj file.");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Open", {100.f, 0.f})) {
            fs::path p(m_pathBuffer);
            if (fs::exists(p)) {
                // Scan for .csproj
                for (auto& entry : fs::directory_iterator(p)) {
                    if (entry.path().extension() == ".csproj") {
                        ProjectOpenedEvent evt;
                        evt.name       = entry.path().stem().string();
                        evt.path       = p.string();
                        evt.csprojPath = entry.path().string();
                        EventBus::Get().Emit(evt);
                        m_showBrowse = false;
                        ImGui::CloseCurrentPopup();
                        break;
                    }
                }
                if (m_showBrowse)
                    m_console->LogError("No .csproj found in: " + p.string());
            } else {
                m_console->LogError("Path does not exist: " + p.string());
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", {80.f, 0.f})) {
            m_showBrowse = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (m_showBrowse)
        ImGui::OpenPopup("Open Project##browse");
}

// ---------------------------------------------------------------------------
void LaunchScreen::BeginCreate() {
    m_creating = true;

    m_pendingProject.name        = m_nameBuffer;
    m_pendingProject.rootPath    = m_pathBuffer;
    m_pendingProject.projectPath = fs::path(m_pathBuffer) / m_nameBuffer;
    m_pendingProject.csprojPath  = m_pendingProject.projectPath / (std::string(m_nameBuffer) + ".csproj");
    m_pendingProject.backend     = m_backend;
    m_pendingProject.tmplate     = m_template;

    // Ensure root path exists
    std::error_code ec;
    fs::create_directories(m_pendingProject.rootPath, ec);

    const std::string dotnetTmpl = DotnetTemplateName(m_backend);
    const std::string dir        = m_pendingProject.rootPath.string();
    const std::string name       = m_pendingProject.name;

    m_console->LogInfo("Creating project '" + name + "' using template: " + dotnetTmpl);

    // Step 1: ensure MonoGame templates are installed, then create project
    m_cli->InstallMonoGameTemplates([this, dir, name, dotnetTmpl](int exitCode, std::string) {
        if (exitCode != 0) {
            m_console->LogError("Failed to install MonoGame templates. Is .NET SDK installed?");
            m_creating = false;
            return;
        }
        m_cli->NewProject(dir, name, dotnetTmpl, [this](int ec, std::string) {
            if (ec == 0) {
                OnProjectReady();
            } else {
                m_console->LogError("dotnet new failed. Check console for details.");
                m_creating = false;
            }
        });
    });
}

// ---------------------------------------------------------------------------
void LaunchScreen::OnProjectReady() {
    m_creating = false;
    m_showNew  = false;

    ProjectCreatedEvent evt;
    evt.name       = m_pendingProject.name;
    evt.path       = m_pendingProject.projectPath.string();
    evt.csprojPath = m_pendingProject.csprojPath.string();
    EventBus::Get().Emit(evt);

    // ImGui::CloseCurrentPopup() must run on the render thread; flag it closed
    // and it'll skip BeginPopupModal on the next frame.
}
