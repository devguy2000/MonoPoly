#pragma once
#include <string>
#include <filesystem>

enum class GraphicsBackend { OpenGL, DirectX, Vulkan };
enum class ProjectTemplate  { Empty2D, Platformer2D, RTS2D, Roguelike2D };

inline const char* BackendName(GraphicsBackend b) {
    switch (b) {
    case GraphicsBackend::DirectX: return "DirectX";
    case GraphicsBackend::Vulkan:  return "Vulkan";
    default:                       return "OpenGL";
    }
}
inline const char* DotnetTemplateName(GraphicsBackend b) {
    switch (b) {
    case GraphicsBackend::DirectX: return "mgwindowsdx";
    case GraphicsBackend::Vulkan:  return "mgdesktopgl"; // MG uses GL or DX; Vulkan is future
    default:                       return "mgdesktopgl";
    }
}
inline const char* TemplateName(ProjectTemplate t) {
    switch (t) {
    case ProjectTemplate::Platformer2D: return "Platformer 2D";
    case ProjectTemplate::RTS2D:        return "2D RTS";
    case ProjectTemplate::Roguelike2D:  return "Roguelike 2D";
    default:                            return "Empty 2D";
    }
}

struct MonoGameProject {
    std::string      name;
    std::filesystem::path rootPath;    // parent directory chosen by user
    std::filesystem::path projectPath; // rootPath / name
    std::filesystem::path csprojPath;  // projectPath / name.csproj

    GraphicsBackend  backend  = GraphicsBackend::OpenGL;
    ProjectTemplate  tmplate  = ProjectTemplate::Empty2D;

    bool IsValid() const { return !name.empty() && !rootPath.empty(); }
};
