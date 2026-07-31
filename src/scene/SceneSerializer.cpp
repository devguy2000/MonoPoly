#include "SceneSerializer.hpp"
#include "SceneGraph.hpp"
#include "Components.hpp"

#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
static json SerializeVec2(const glm::vec2& v) {
    return {{"x", v.x}, {"y", v.y}};
}
static json SerializeVec4(const glm::vec4& v) {
    return {{"r", v.r}, {"g", v.g}, {"b", v.b}, {"a", v.a}};
}
static glm::vec2 DeserVec2(const json& j) {
    return {j.value("x", 0.f), j.value("y", 0.f)};
}
static glm::vec4 DeserVec4(const json& j) {
    return {j.value("r", 1.f), j.value("g", 1.f), j.value("b", 1.f), j.value("a", 1.f)};
}

// ---------------------------------------------------------------------------
static json SerializeEntity(SceneGraph& scene, entt::entity e) {
    json je;
    je["id"] = static_cast<uint32_t>(e);

    auto& reg = scene.Registry();

    if (reg.all_of<TagComponent>(e))
        je["tag"] = reg.get<TagComponent>(e).name;

    if (reg.all_of<Transform2D>(e)) {
        auto& t = reg.get<Transform2D>(e);
        je["transform"] = {
            {"position", SerializeVec2(t.position)},
            {"rotation", t.rotation},
            {"scale",    SerializeVec2(t.scale)}
        };
    }

    if (reg.all_of<SpriteRenderer>(e)) {
        auto& sr = reg.get<SpriteRenderer>(e);
        je["sprite_renderer"] = {
            {"color",        SerializeVec4(sr.color)},
            {"texture_path", sr.texturePath},
            {"sorting_layer", sr.sortingLayer},
            {"flip_x",       sr.flipX},
            {"flip_y",       sr.flipY}
        };
    }

    if (reg.all_of<Camera2D>(e)) {
        auto& cam = reg.get<Camera2D>(e);
        je["camera2d"] = {{"zoom", cam.zoom}, {"is_primary", cam.isPrimary}};
    }

    if (reg.all_of<BoxCollider2D>(e)) {
        auto& bc = reg.get<BoxCollider2D>(e);
        je["box_collider2d"] = {
            {"offset",     SerializeVec2(bc.offset)},
            {"size",       SerializeVec2(bc.size)},
            {"is_trigger", bc.isTrigger}
        };
    }

    if (reg.all_of<CircleCollider2D>(e)) {
        auto& cc = reg.get<CircleCollider2D>(e);
        je["circle_collider2d"] = {
            {"offset",     SerializeVec2(cc.offset)},
            {"radius",     cc.radius},
            {"is_trigger", cc.isTrigger}
        };
    }

    // Recurse children
    auto children = scene.GetChildren(e);
    if (!children.empty()) {
        for (entt::entity child : children)
            je["children"].push_back(SerializeEntity(scene, child));
    }

    return je;
}

// ---------------------------------------------------------------------------
static entt::entity DeserializeEntity(SceneGraph& scene,
                                       const json& je,
                                       entt::entity parent)
{
    std::string name = je.value("tag", "Entity");
    entt::entity e = (parent == entt::null)
        ? scene.CreateEntity(name)
        : scene.CreateChildEntity(parent, name);

    auto& reg = scene.Registry();

    // Overwrite default tag (CreateEntity already sets it, but we want exact name)
    reg.get<TagComponent>(e).name = name;

    if (je.contains("transform")) {
        auto& jt = je["transform"];
        auto& t  = reg.get<Transform2D>(e);
        t.position = DeserVec2(jt.value("position", json{}));
        t.rotation = jt.value("rotation", 0.f);
        t.scale    = DeserVec2(jt.value("scale", json{{"x",1},{"y",1}}));
    }

    if (je.contains("sprite_renderer")) {
        auto& jsr = je["sprite_renderer"];
        auto& sr  = scene.AddComponent<SpriteRenderer>(e);
        sr.color        = DeserVec4(jsr.value("color", json{}));
        sr.texturePath  = jsr.value("texture_path", "");
        sr.sortingLayer = jsr.value("sorting_layer", 0);
        sr.flipX        = jsr.value("flip_x", false);
        sr.flipY        = jsr.value("flip_y", false);
    }

    if (je.contains("camera2d")) {
        auto& jc  = je["camera2d"];
        auto& cam = scene.AddComponent<Camera2D>(e);
        cam.zoom      = jc.value("zoom", 1.f);
        cam.isPrimary = jc.value("is_primary", true);
    }

    if (je.contains("box_collider2d")) {
        auto& jbc = je["box_collider2d"];
        auto& bc  = scene.AddComponent<BoxCollider2D>(e);
        bc.offset    = DeserVec2(jbc.value("offset", json{}));
        bc.size      = DeserVec2(jbc.value("size", json{{"x",1},{"y",1}}));
        bc.isTrigger = jbc.value("is_trigger", false);
    }

    if (je.contains("circle_collider2d")) {
        auto& jcc = je["circle_collider2d"];
        auto& cc  = scene.AddComponent<CircleCollider2D>(e);
        cc.offset    = DeserVec2(jcc.value("offset", json{}));
        cc.radius    = jcc.value("radius", 0.5f);
        cc.isTrigger = jcc.value("is_trigger", false);
    }

    if (je.contains("children") && je["children"].is_array()) {
        for (auto& child : je["children"])
            DeserializeEntity(scene, child, e);
    }

    return e;
}

// ---------------------------------------------------------------------------
bool SceneSerializer::SaveToFile(const std::string& filepath) {
    json root;
    root["version"] = 1;
    root["entities"] = json::array();

    for (entt::entity e : m_scene.GetRootEntities())
        root["entities"].push_back(SerializeEntity(m_scene, e));

    std::ofstream f(filepath);
    if (!f.is_open()) return false;
    f << root.dump(2);
    return true;
}

bool SceneSerializer::LoadFromFile(const std::string& filepath) {
    std::ifstream f(filepath);
    if (!f.is_open()) return false;

    json root;
    try { root = json::parse(f); }
    catch (...) { return false; }

    m_scene.Clear();

    if (!root.contains("entities")) return false;
    for (auto& je : root["entities"])
        DeserializeEntity(m_scene, je, entt::null);

    return true;
}
