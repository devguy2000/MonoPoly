#pragma once
#include <entt/entt.hpp>
#include <string>

class SceneGraph;

class SceneHierarchyPanel {
public:
    explicit SceneHierarchyPanel(SceneGraph* scene = nullptr);

    void SetScene(SceneGraph* scene) { m_scene = scene; }
    void OnImGuiRender();

    entt::entity SelectedEntity() const { return m_selected; }
    void         SetSelected(entt::entity e) { m_selected = e; }

private:
    void DrawEntityNode(entt::entity e);
    void DrawContextMenu();

    SceneGraph*  m_scene    = nullptr;
    entt::entity m_selected = entt::null;
    entt::entity m_renaming = entt::null;
    char         m_renameBuffer[256] = {};
};
