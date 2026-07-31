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
