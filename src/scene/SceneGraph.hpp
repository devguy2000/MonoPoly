#pragma once
#include <entt/entt.hpp>
#include "Components.hpp"
#include <string>
#include <vector>
#include <functional>

class SceneGraph {
public:
    SceneGraph();
    ~SceneGraph();

    // Entity CRUD
    entt::entity CreateEntity(const std::string& name = "Entity");
    entt::entity CreateChildEntity(entt::entity parent, const std::string& name = "Entity");
    void         DestroyEntity(entt::entity e);
    void         ReparentEntity(entt::entity e, entt::entity newParent); // newParent==entt::null => root

    // Component helpers (pass-through to registry)
    template<typename T, typename... Args>
    T& AddComponent(entt::entity e, Args&&... args) {
        return m_registry.emplace<T>(e, std::forward<Args>(args)...);
    }

    template<typename T>
    T& GetComponent(entt::entity e) { return m_registry.get<T>(e); }

    template<typename T>
    bool HasComponent(entt::entity e) const { return m_registry.all_of<T>(e); }

    template<typename T>
    void RemoveComponent(entt::entity e) { m_registry.remove<T>(e); }

    template<typename... T>
    auto View() { return m_registry.view<T...>(); }

    // Root-level entities (no Parent component)
    std::vector<entt::entity> GetRootEntities() const;

    // Direct children of an entity
    std::vector<entt::entity> GetChildren(entt::entity e) const;

    entt::registry& Registry() { return m_registry; }

    // Scene-level settings (background color, resolution)
    SceneSettings&       Settings()       { return m_settings; }
    const SceneSettings& Settings() const { return m_settings; }

    void Clear();

    bool IsDirty() const { return m_dirty; }
    void ClearDirty()    { m_dirty = false; }

private:
    entt::registry m_registry;
    SceneSettings  m_settings;
    bool           m_dirty  = false;
    uint32_t       m_nextId = 1;

    void RemoveFromParent(entt::entity e);
    void AddToParent(entt::entity e, entt::entity parent);
};
