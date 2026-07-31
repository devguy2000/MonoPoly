#pragma once
#include <imgui.h>
#include <string>
#include <vector>
#include <mutex>

enum class LogLevel { Info, Success, Warning, Error };

struct LogEntry {
    std::string text;
    LogLevel    level     = LogLevel::Info;
    float       timestamp = 0.f;
};

class ConsolePanel {
public:
    ConsolePanel();
    ~ConsolePanel();

    void OnImGuiRender();

    // Thread-safe: safe to call from CliRunner worker threads.
    void Log(std::string_view text, LogLevel level = LogLevel::Info);
    void LogInfo   (std::string_view t) { Log(t, LogLevel::Info);    }
    void LogSuccess(std::string_view t) { Log(t, LogLevel::Success);  }
    void LogWarning(std::string_view t) { Log(t, LogLevel::Warning);  }
    void LogError  (std::string_view t) { Log(t, LogLevel::Error);    }
    void Clear();

private:
    std::vector<LogEntry> m_entries;
    std::mutex            m_mutex;
    bool                  m_autoScroll   = true;
    bool                  m_scrollToBot  = false;
    bool                  m_showInfo     = true;
    bool                  m_showSuccess  = true;
    bool                  m_showWarning  = true;
    bool                  m_showError    = true;
    char                  m_filter[256]  = {};

    static ImVec4 LevelColor(LogLevel l);
    static const char* LevelLabel(LogLevel l);
};
