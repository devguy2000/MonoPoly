#pragma once
#include <SDL3/SDL.h>
#include <memory>
#include <string>

class EditorLayer;

class Application {
public:
    Application(std::string title, int width, int height);
    ~Application();

    // Not copyable or movable – singleton lifetime tied to main().
    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;

    void Run();
    void Quit() { m_running = false; }

    static Application& Get() { return *s_instance; }

    [[nodiscard]] int         GetWidth()  const { return m_width; }
    [[nodiscard]] int         GetHeight() const { return m_height; }
    [[nodiscard]] SDL_Window* GetWindow() const { return m_window; }

private:
    bool Init();
    void Shutdown();
    void ProcessEvents();
    void BeginFrame();
    void EndFrame();
    void BuildDefaultDockLayout(unsigned int dockspaceId);

    SDL_Window*   m_window    = nullptr;
    SDL_GLContext m_glContext = nullptr;
    bool          m_running   = false;
    int           m_width;
    int           m_height;
    std::string   m_title;

    bool m_dockLayoutBuilt = false;

    std::unique_ptr<EditorLayer> m_editorLayer;

    static Application* s_instance;
};
