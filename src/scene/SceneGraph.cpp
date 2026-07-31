#include "SceneGraph.hpp"
#include "Components.hpp"

SceneGraph::SceneGraph()  = default;
SceneGraph::~SceneGraph() = default;

// ---------------------------------------------------------------------------
entt::entity SceneGraph::CreateEntity(const std::string& name) {
    entt::entity e = m_registry.create();
    m_registry.emplace<TagComponent>(e, name.empty() ? "Entity" : name);
    m_registry.emplace<Transform2D>(e);
    m_dirty = true;
    return e;
}

entt::entity SceneGraph::CreateChildEntity(entt::entity parent, const std::string& name) {
    entt::entity e = CreateEntity(name);
    AddToParent(e, parent);
    return e;
}

// ---------------------------------------------------------------------------
void SceneGraph::DestroyEntity(entt::entity e) {
    // Recursively destroy children first
    if (m_registry.all_of<Children>(e)) {
        auto kids = m_registry.get<Children>(e).list;
        for (entt::entity child : kids)
            DestroyEntity(child);
    }
    RemoveFromParent(e);
    m_registry.destroy(e);
    m_dirty = true;
}

// ---------------------------------------------------------------------------
void SceneGraph::ReparentEntity(entt::entity e, entt::entity newParent) {
    RemoveFromParent(e);
    if (newParent != entt::null)
        AddToParent(e, newParent);
    else
        m_registry.remove<Parent>(e);
    m_dirty = true;
}

// ---------------------------------------------------------------------------
std::vector<entt::entity> SceneGraph::GetRootEntities() const {
    std::vector<entt::entity> roots;
    for (auto entity : m_registry.view<TagComponent>()) {
        if (!m_registry.all_of<Parent>(entity))
            roots.push_back(entity);
    }
    return roots;
}

std::vector<entt::entity> SceneGraph::GetChildren(entt::entity e) const {
    if (!m_registry.valid(e) || !m_registry.all_of<Children>(e))
        return {};
    return m_registry.get<Children>(e).list;
}

// ---------------------------------------------------------------------------
void SceneGraph::Clear() {
    m_registry.clear();
    m_dirty = true;
}

// ---------------------------------------------------------------------------
void SceneGraph::RemoveFromParent(entt::entity e) {
    if (!m_registry.all_of<Parent>(e)) return;

    entt::entity parentEnt = m_registry.get<Parent>(e).entity;
    if (m_registry.valid(parentEnt) && m_registry.all_of<Children>(parentEnt)) {
        auto& kids = m_registry.get<Children>(parentEnt).list;
        kids.erase(std::remove(kids.begin(), kids.end(), e), kids.end());
        if (kids.empty())
            m_registry.remove<Children>(parentEnt);
    }
    m_registry.remove<Parent>(e);
}

void SceneGraph::AddToParent(entt::entity e, entt::entity parent) {
    m_registry.emplace_or_replace<Parent>(e, parent);
    if (!m_registry.all_of<Children>(parent))
        m_registry.emplace<Children>(parent);
    m_registry.get<Children>(parent).list.push_back(e);
}
