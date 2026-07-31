#pragma once
#include <string>

class GameCodeGen {
public:
    // Replaces Game1.cs with a runtime scene-reading version.
    // Returns false if the file can't be written.
    static bool GenerateGame1(const std::string& projectPath,
                              const std::string& projectName);
};
