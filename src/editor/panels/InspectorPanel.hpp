#pragma once
#include <entt/entt.hpp>
#include <string>

class SceneGraph;

class InspectorPanel {
public:
    explicit InspectorPanel(SceneGraph* scene = nullptr);

    void SetScene(SceneGraph* scene)         { m_scene       = scene; }
    void SetSelected(entt::entity e)         { m_selected    = e; }
    void SetProjectPath(const std::string& p){ m_projectPath = p; }
    void OnImGuiRender();

private:
    void DrawTag();
    void DrawTransform2D();
    void DrawSpriteRenderer();
    void DrawCamera2D();
    void DrawBoxCollider2D();
    void DrawCircleCollider2D();
    void DrawAddComponentMenu();

    void DrawSceneSettings();
    void TryLoadTexture(const std::string& contentName);

    SceneGraph*  m_scene       = nullptr;
    entt::entity m_selected    = entt::null;
    std::string  m_projectPath;
};
