#pragma once
#include <memory>
#include <string>

class ConsolePanel;
class SceneHierarchyPanel;
class InspectorPanel;
class ViewportPanel;
class LaunchScreen;

class EditorLayer {
public:
    EditorLayer();
    ~EditorLayer();

    void OnAttach();
    void OnDetach();
    void OnImGuiRender(float dt);

    [[nodiscard]] bool IsProjectOpen() const { return m_projectOpen; }
    void               OpenProject(const std::string& path);

private:
    void DrawMenuBar();
    void DrawProjectClosedOverlay();

    bool m_projectOpen      = false;
    bool m_showDemoWindow   = false;

    std::unique_ptr<ConsolePanel>        m_console;
    std::unique_ptr<SceneHierarchyPanel> m_hierarchy;
    std::unique_ptr<InspectorPanel>      m_inspector;
    std::unique_ptr<ViewportPanel>       m_viewport;
    std::unique_ptr<LaunchScreen>        m_launchScreen;
};
