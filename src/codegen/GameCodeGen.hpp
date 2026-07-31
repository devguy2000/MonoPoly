#pragma once
#include <string>
#include <glm/glm.hpp>

class GameCodeGen {
public:
    static bool GenerateGame1(const std::string& projectPath,
                              const std::string& projectName,
                              int screenW = 1280, int screenH = 720,
                              glm::vec4 bgColor = {0,0,0,1});
};
