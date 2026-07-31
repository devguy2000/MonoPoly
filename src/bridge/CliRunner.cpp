#include "CliRunner.hpp"
#include "core/EventBus.hpp"
#include "core/Events.hpp"

#include <windows.h>

#include <string>
#include <vector>
#include <sstream>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), len);
    return out;
}

// Splits a multi-line buffer into individual lines and fires CliOutputEvent
// for each non-empty line.
static void FlushLines(const std::string& jobTag, const std::string& chunk,
                        bool isError, std::string& lineBuffer) {
    lineBuffer += chunk;
    size_t pos = 0;
    while (true) {
        size_t nl = lineBuffer.find('\n', pos);
        if (nl == std::string::npos) break;
        std::string line = lineBuffer.substr(pos, nl - pos);
        // Strip CR for Windows line endings
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) {
            EventBus::Get().EmitDeferred(CliOutputEvent{
                "[" + jobTag + "] " + line, isError
            });
        }
        pos = nl + 1;
    }
    lineBuffer = lineBuffer.substr(pos); // keep incomplete line
}

// ---------------------------------------------------------------------------
std::string CliRunner::ExecCapture(const std::string& cmd,
                                    const std::string& workingDir,
                                    const std::string& jobTag,
                                    int& outExitCode)
{
    outExitCode = -1;

    // Create anonymous pipe for stdout/stderr
    HANDLE hReadPipe  = nullptr;
    HANDLE hWritePipe = nullptr;
    SECURITY_ATTRIBUTES sa{};
    sa.nLength              = sizeof(sa);
    sa.bInheritHandle       = TRUE;
    sa.lpSecurityDescriptor = nullptr;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
        return "CreatePipe failed.";
    // Read end must NOT be inherited
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb          = sizeof(si);
    si.dwFlags     = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput  = hWritePipe;
    si.hStdError   = hWritePipe;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};
    std::wstring wCmd = Utf8ToWide(cmd);
    std::wstring wDir = Utf8ToWide(workingDir);

    BOOL ok = CreateProcessW(
        nullptr,
        wCmd.data(),
        nullptr, nullptr,
        TRUE,            // inherit handles
        CREATE_NO_WINDOW,
        nullptr,
        wDir.empty() ? nullptr : wDir.c_str(),
        &si, &pi
    );

    // Parent no longer needs the write end
    CloseHandle(hWritePipe);

    if (!ok) {
        CloseHandle(hReadPipe);
        return "CreateProcess failed. Is 'dotnet' on PATH?";
    }

    // Read pipe until the child closes it
    std::string fullOutput;
    std::string lineBuffer;
    char  buf[4096];
    DWORD bytesRead = 0;
    while (ReadFile(hReadPipe, buf, sizeof(buf) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buf[bytesRead] = '\0';
        std::string chunk(buf, bytesRead);
        fullOutput += chunk;
        FlushLines(jobTag, chunk, false, lineBuffer);
    }
    // Flush any remaining incomplete line
    if (!lineBuffer.empty())
        EventBus::Get().EmitDeferred(CliOutputEvent{"[" + jobTag + "] " + lineBuffer, false});

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit = 0;
    GetExitCodeProcess(pi.hProcess, &exit);
    outExitCode = static_cast<int>(exit);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);

    return fullOutput;
}

// ---------------------------------------------------------------------------
CliRunner::CliRunner() = default;

void CliRunner::RunAsync(CliJob job) {
    m_pool.Submit([job = std::move(job)]() mutable {
        int exitCode = 0;
        EventBus::Get().EmitDeferred(
            CliOutputEvent{"─── " + job.tag + " ───", false});

        std::string output = ExecCapture(job.command, job.workingDir, job.tag, exitCode);

        EventBus::Get().EmitDeferred(
            CliJobCompletedEvent{job.tag, exitCode});

        if (job.onComplete) {
            int  ec  = exitCode;
            EventBus::Get().EmitDeferred(CliOutputEvent{
                ec == 0
                    ? "─── " + job.tag + " succeeded ───"
                    : "─── " + job.tag + " FAILED (exit " + std::to_string(ec) + ") ───",
                ec != 0
            });
            // PostToMainThread avoids accumulating per-job event subscribers.
            EventBus::Get().PostToMainThread(
                [cb = std::move(job.onComplete), ec, out = std::move(output)]() mutable {
                    cb(ec, std::move(out));
                }
            );
        }
    });
}

// ---------------------------------------------------------------------------
void CliRunner::CheckDotnet(CliCompleteCb cb) {
    RunAsync({"dotnet --version", "dotnet --version", "", std::move(cb)});
}

void CliRunner::InstallMonoGameTemplates(CliCompleteCb cb) {
    RunAsync({
        "install MG templates",
        "dotnet new install MonoGame.Templates.CSharp",
        "",
        std::move(cb)
    });
}

void CliRunner::NewProject(const std::string& dir, const std::string& name,
                            const std::string& dotnetTemplate, CliCompleteCb cb)
{
    RunAsync({
        "dotnet new " + dotnetTemplate,
        "dotnet new " + dotnetTemplate + " -n " + name + " -o " + name,
        dir,
        std::move(cb)
    });
}

void CliRunner::BuildProject(const std::string& csprojPath, CliCompleteCb cb) {
    RunAsync({
        "dotnet build",
        "dotnet build \"" + csprojPath + "\"",
        "",
        std::move(cb)
    });
}

void CliRunner::RestoreProject(const std::string& csprojPath, CliCompleteCb cb) {
    RunAsync({
        "dotnet restore",
        "dotnet restore \"" + csprojPath + "\"",
        "",
        std::move(cb)
    });
}
