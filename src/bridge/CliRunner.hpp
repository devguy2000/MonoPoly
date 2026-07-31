#pragma once
#include <functional>
#include <string>
#include <memory>
#include "core/ThreadPool.hpp"

// Callback types
using CliCompleteCb = std::function<void(int exitCode, std::string output)>;

struct CliJob {
    std::string    tag;           // display name in console ("dotnet build", etc.)
    std::string    command;       // full command line, e.g. "dotnet new mgdesktopgl -n MyGame"
    std::string    workingDir;    // empty = inherit CWD
    CliCompleteCb  onComplete;    // called on MAIN thread via EventBus::EmitDeferred
};

// Spawns dotnet child processes without a console window, captures all output,
// posts lines to EventBus<CliOutputEvent> in real-time, and fires onComplete
// on the main thread when the process exits.
class CliRunner {
public:
    CliRunner();
    ~CliRunner() = default;

    void RunAsync(CliJob job);

    // High-level helpers
    void CheckDotnet(CliCompleteCb cb);
    void InstallMonoGameTemplates(CliCompleteCb cb);
    void NewProject(const std::string& dir, const std::string& name,
                    const std::string& dotnetTemplate, CliCompleteCb cb);
    void BuildProject(const std::string& csprojPath, CliCompleteCb cb);
    void RestoreProject(const std::string& csprojPath, CliCompleteCb cb);

private:
    // Runs synchronously inside a worker thread. Returns stdout+stderr combined.
    static std::string ExecCapture(const std::string& cmd,
                                   const std::string& workingDir,
                                   const std::string& jobTag,
                                   int& outExitCode);

    ThreadPool m_pool{2};
};
