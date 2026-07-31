#pragma once
#include <string>

// ---------------------------------------------------------------------------
// CLI / Bridge events
// ---------------------------------------------------------------------------

// Fired for each chunk of text arriving from a child process stdout/stderr.
struct CliOutputEvent {
    std::string text;
    bool        isError = false;
};

// Fired when a dotnet child process exits.
struct CliJobCompletedEvent {
    std::string jobTag;   // caller-supplied label ("dotnet build", etc.)
    int         exitCode = 0;
    bool        success() const { return exitCode == 0; }
};

// ---------------------------------------------------------------------------
// Project lifecycle events
// ---------------------------------------------------------------------------

struct ProjectCreatedEvent {
    std::string name;
    std::string path;        // absolute root directory
    std::string csprojPath;
};

struct ProjectOpenedEvent {
    std::string name;
    std::string path;
    std::string csprojPath;
};

struct ProjectClosedEvent {};

// ---------------------------------------------------------------------------
// Editor state events
// ---------------------------------------------------------------------------

struct PlayModeStartedEvent {};
struct PlayModeStoppedEvent {};
struct SceneModifiedEvent {};

// ---------------------------------------------------------------------------
// Content / asset events
// ---------------------------------------------------------------------------

struct AssetImportedEvent {
    std::string assetPath;    // absolute path to the image file
    std::string contentName;  // content name without extension (e.g. "player")
};

// ---------------------------------------------------------------------------
// File system events (from OS drag-drop)
// ---------------------------------------------------------------------------

struct FileDroppedEvent {
    std::string path;   // absolute path of the dropped file
};
