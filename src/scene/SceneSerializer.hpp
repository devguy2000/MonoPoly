#pragma once
#include <string>

class SceneGraph;

class SceneSerializer {
public:
    explicit SceneSerializer(SceneGraph& scene) : m_scene(scene) {}

    bool SaveToFile(const std::string& filepath);
    bool LoadFromFile(const std::string& filepath);

private:
    SceneGraph& m_scene;
};
