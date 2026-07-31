#include "Application.hpp"
#include "EventBus.hpp"

#include <glad/glad.h>
#include <SDL3/SDL.h>

#include <imgui.h>
#include <imgui_internal.h>       // DockBuilder API
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

#include "editor/EditorLayer.hpp"

#include <stdexcept>
#include <cstdio>

Application* Application::s_instance = nullptr;

// ---------------------------------------------------------------------------
Application::Application(std::string title, int width, int height)
    : m_title(std::move(title)), m_width(width), m_height(height)
{
    if (s_instance)
        throw std::runtime_error("Only one Application instance is allowed.");
    s_instance = this;

    if (!Init())
        throw std::runtime_error("Application::Init() failed – check stderr.");
}

Application::~Application() {
    Shutdown();
    s_instance = nullptr;
}

// ---------------------------------------------------------------------------
bool Application::Init() {
    // SDL3 -----------------------------------------------------------------
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    // OpenGL 4.6 Core context attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,  8);

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, m_title.c_str());
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER,  m_width);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, m_height);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    m_window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);

    if (!m_window) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "SDL_GL_CreateContext failed: %s", SDL_GetError());
        return false;
    }
    SDL_GL_MakeCurrent(m_window, m_glContext);
    SDL_GL_SetSwapInterval(1); // vsync

    // glad (vcpkg v0.1.x — no Python dependency) ---------------------------
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "gladLoadGLLoader failed.");
        return false;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
        "OpenGL %s  Vendor: %s  Renderer: %s",
        reinterpret_cast<const char*>(glGetString(GL_VERSION)),
        reinterpret_cast<const char*>(glGetString(GL_VENDOR)),
        reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    // ImGui ----------------------------------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    io.IniFilename = "monopoly_layout.ini";

    // Dark style with minor tweaks for an IDE look
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 4.0f;
    style.FrameRounding     = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding      = 3.0f;
    style.TabRounding       = 4.0f;
    style.FramePadding      = {6.f, 4.f};
    style.ItemSpacing       = {8.f, 5.f};
    // When viewports are enabled the platform window background must match
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplSDL3_InitForOpenGL(m_window, m_glContext);
    ImGui_ImplOpenGL3_Init("#version 460 core");

    // Editor layer ---------------------------------------------------------
    m_editorLayer = std::make_unique<EditorLayer>();
    m_editorLayer->OnAttach();

    m_running = true;
    return true;
}

// ---------------------------------------------------------------------------
void Application::Shutdown() {
    if (m_editorLayer) {
        m_editorLayer->OnDetach();
        m_editorLayer.reset();
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    if (m_glContext) SDL_GL_DestroyContext(m_glContext);
    if (m_window)   SDL_DestroyWindow(m_window);
    SDL_Quit();
}

// ---------------------------------------------------------------------------
void Application::ProcessEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        ImGui_ImplSDL3_ProcessEvent(&e);
        switch (e.type) {
        case SDL_EVENT_QUIT:
            m_running = false;
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            m_width  = e.window.data1;
            m_height = e.window.data2;
            break;
        default: break;
        }
    }
}

// ---------------------------------------------------------------------------
void Application::BeginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Fullscreen invisible host window for the dockspace
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoDocking          |
        ImGuiWindowFlags_NoTitleBar         |
        ImGuiWindowFlags_NoCollapse         |
        ImGuiWindowFlags_NoResize           |
        ImGuiWindowFlags_NoMove             |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus         |
        ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,   {0.f, 0.f});
    ImGui::Begin("##DockHost", nullptr, hostFlags);
    ImGui::PopStyleVar(3);

    // Build default layout exactly once (first frame after ini is loaded)
    ImGuiID dockId = ImGui::GetID("MainDockSpace");
    if (!m_dockLayoutBuilt && !ImGui::DockBuilderGetNode(dockId)) {
        BuildDefaultDockLayout(dockId);
        m_dockLayoutBuilt = true;
    }

    ImGui::DockSpace(dockId, {0.f, 0.f}, ImGuiDockNodeFlags_PassthruCentralNode);
}

// ---------------------------------------------------------------------------
void Application::BuildDefaultDockLayout(unsigned int dockId) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();

    ImGui::DockBuilderRemoveNode(dockId);
    // ImGuiDockNodeFlags_DockSpace is in the private enum (imgui_internal.h).
    // Both operands cast to int first to suppress C5054 (mixed-enum OR).
    ImGui::DockBuilderAddNode(dockId,
        static_cast<ImGuiDockNodeFlags>(
            static_cast<int>(ImGuiDockNodeFlags_DockSpace) |
            static_cast<int>(ImGuiDockNodeFlags_PassthruCentralNode)));
    ImGui::DockBuilderSetNodeSize(dockId, vp->WorkSize);

    // Split: left panel (hierarchy)
    ImGuiID nodeLeft, nodeCenter;
    ImGui::DockBuilderSplitNode(dockId, ImGuiDir_Left, 0.18f, &nodeLeft, &nodeCenter);

    // Split: right panel (inspector)
    ImGuiID nodeRight;
    ImGui::DockBuilderSplitNode(nodeCenter, ImGuiDir_Right, 0.22f, &nodeRight, &nodeCenter);

    // Split: bottom panel (console)
    ImGuiID nodeBottom;
    ImGui::DockBuilderSplitNode(nodeCenter, ImGuiDir_Down, 0.25f, &nodeBottom, &nodeCenter);

    ImGui::DockBuilderDockWindow("Scene Hierarchy", nodeLeft);
    ImGui::DockBuilderDockWindow("Viewport",        nodeCenter);
    ImGui::DockBuilderDockWindow("Inspector",       nodeRight);
    ImGui::DockBuilderDockWindow("Console",         nodeBottom);

    ImGui::DockBuilderFinish(dockId);
}

// ---------------------------------------------------------------------------
void Application::EndFrame() {
    // Close the DockHost window opened in BeginFrame()
    ImGui::End();

    // Render
    ImGui::Render();

    int fbW, fbH;
    SDL_GetWindowSizeInPixels(m_window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);
    glClearColor(0.11f, 0.11f, 0.13f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Multi-viewport: tear-off windows rendered by SDL3 platform layer
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        SDL_GLContext currentCtx = SDL_GL_GetCurrentContext();
        SDL_Window*   currentWin = SDL_GL_GetCurrentWindow();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(currentWin, currentCtx);
    }

    SDL_GL_SwapWindow(m_window);
}

// ---------------------------------------------------------------------------
void Application::Run() {
    Uint64 lastTick  = SDL_GetTicksNS();
    float  deltaTime = 0.f;

    while (m_running) {
        Uint64 now   = SDL_GetTicksNS();
        deltaTime    = static_cast<float>(now - lastTick) * 1e-9f;
        lastTick     = now;

        // Drain deferred events queued by worker threads
        EventBus::Get().FlushDeferred();

        ProcessEvents();
        BeginFrame();
        m_editorLayer->OnImGuiRender(deltaTime);
        EndFrame();
    }
}
