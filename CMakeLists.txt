cmake_minimum_required(VERSION 3.25)
project(MonoPoly VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------------------------------
# vcpkg-managed packages  (no Python dependency at all)
# --------------------------------------------------------------------------
find_package(SDL3          CONFIG REQUIRED)
find_package(glad          CONFIG REQUIRED)   # pre-built, GL 4.6 Core, no Python
find_package(nlohmann_json CONFIG REQUIRED)
find_package(EnTT          CONFIG REQUIRED)
find_package(glm           CONFIG REQUIRED)
find_package(Stb           REQUIRED)

# --------------------------------------------------------------------------
# Dear ImGui  –  docking branch  (FetchContent, no Python needed)
# --------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
# Tell the OpenGL3 backend to use vcpkg glad-style includes (<glad/glad.h>)
target_compile_definitions(imgui_lib PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
target_link_libraries(imgui_lib PUBLIC SDL3::SDL3 glad::glad)

# --------------------------------------------------------------------------
# ImGuizmo  (FetchContent; uses its own CMakeLists.txt with correct src/ paths)
# --------------------------------------------------------------------------
FetchContent_Declare(
    imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imguizmo)
# Wire our imgui headers into ImGuizmo (its CMakeLists doesn't add imgui by default)
target_link_libraries(imguizmo PUBLIC imgui_lib)

# --------------------------------------------------------------------------
# MonoPoly executable
# --------------------------------------------------------------------------
file(GLOB_RECURSE MONOPOLY_SOURCES "src/*.cpp")

add_executable(MonoPoly ${MONOPOLY_SOURCES})

target_include_directories(MonoPoly PRIVATE src)

target_link_libraries(MonoPoly PRIVATE
    SDL3::SDL3
    glad::glad
    imgui_lib
    imguizmo
    nlohmann_json::nlohmann_json
    EnTT::EnTT
    glm::glm
)
target_include_directories(MonoPoly PRIVATE ${Stb_INCLUDE_DIR})

# --------------------------------------------------------------------------
# Compiler settings
# --------------------------------------------------------------------------
if(MSVC)
    target_compile_options(MonoPoly PRIVATE /W4 /MP /utf-8)
    target_compile_options(MonoPoly PRIVATE /wd4503)  # decorated name length exceeded
else()
    target_compile_options(MonoPoly PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
    target_compile_definitions(MonoPoly PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
        _CRT_SECURE_NO_WARNINGS
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(MonoPoly PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()
