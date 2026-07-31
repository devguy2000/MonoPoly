#include "ConsolePanel.hpp"
#include <SDL3/SDL.h>
#include <cstring>
#include <algorithm>

ConsolePanel::ConsolePanel() {
    m_entries.reserve(512);
}
ConsolePanel::~ConsolePanel() = default;

// ---------------------------------------------------------------------------
void ConsolePanel::Log(std::string_view text, LogLevel level) {
    // Strip trailing newlines so the table rows stay clean
    std::string cleaned(text);
    while (!cleaned.empty() && (cleaned.back() == '\n' || cleaned.back() == '\r'))
        cleaned.pop_back();
    if (cleaned.empty()) return;

    float ts = static_cast<float>(SDL_GetTicks()) * 0.001f;

    std::lock_guard lock(m_mutex);
    m_entries.push_back({std::move(cleaned), level, ts});
    if (m_autoScroll) m_scrollToBot = true;
}

void ConsolePanel::Clear() {
    std::lock_guard lock(m_mutex);
    m_entries.clear();
}

// ---------------------------------------------------------------------------
ImVec4 ConsolePanel::LevelColor(LogLevel l) {
    switch (l) {
    case LogLevel::Success: return {0.40f, 0.85f, 0.40f, 1.f};
    case LogLevel::Warning: return {1.00f, 0.80f, 0.10f, 1.f};
    case LogLevel::Error:   return {1.00f, 0.35f, 0.35f, 1.f};
    default:                return {0.85f, 0.85f, 0.85f, 1.f};
    }
}
const char* ConsolePanel::LevelLabel(LogLevel l) {
    switch (l) {
    case LogLevel::Success: return "[OK]  ";
    case LogLevel::Warning: return "[WARN]";
    case LogLevel::Error:   return "[ERR] ";
    default:                return "[INFO]";
    }
}

// ---------------------------------------------------------------------------
void ConsolePanel::OnImGuiRender() {
    ImGui::Begin("Console");

    // --- Toolbar -----------------------------------------------------------
    if (ImGui::Button("Clear")) Clear();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200.f);
    ImGui::InputTextWithHint("##filter", "Filter...", m_filter, sizeof(m_filter));
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_autoScroll);
    ImGui::SameLine(0.f, 16.f);
    ImGui::TextUnformatted("Show:");
    ImGui::SameLine();
    ImGui::Checkbox("Info",    &m_showInfo);    ImGui::SameLine();
    ImGui::Checkbox("OK",      &m_showSuccess); ImGui::SameLine();
    ImGui::Checkbox("Warn",    &m_showWarning); ImGui::SameLine();
    ImGui::Checkbox("Error",   &m_showError);

    ImGui::Separator();

    // --- Log area ----------------------------------------------------------
    const float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("##scroll", {0.f, -footerHeight}, false,
        ImGuiWindowFlags_HorizontalScrollbar);

    std::vector<LogEntry> snapshot;
    {
        std::lock_guard lock(m_mutex);
        snapshot = m_entries; // cheap copy for thread safety
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {4.f, 1.f});
    for (const auto& e : snapshot) {
        // Level filter
        bool visible = (e.level == LogLevel::Info    && m_showInfo)    ||
                       (e.level == LogLevel::Success && m_showSuccess)  ||
                       (e.level == LogLevel::Warning && m_showWarning)  ||
                       (e.level == LogLevel::Error   && m_showError);
        if (!visible) continue;

        // Text filter
        if (m_filter[0] != '\0' &&
            e.text.find(m_filter) == std::string::npos) continue;

        // Timestamp
        ImGui::TextDisabled("[%6.1f]", e.timestamp);
        ImGui::SameLine();
        // Level badge
        ImGui::TextColored(LevelColor(e.level), "%s", LevelLabel(e.level));
        ImGui::SameLine();
        // Message
        ImGui::TextColored(LevelColor(e.level), "%s", e.text.c_str());
    }
    ImGui::PopStyleVar();

    if (m_scrollToBot) {
        ImGui::SetScrollHereY(1.f);
        m_scrollToBot = false;
    }
    ImGui::EndChild();

    // --- Status line -------------------------------------------------------
    ImGui::Separator();
    std::lock_guard lock(m_mutex);
    ImGui::TextDisabled("%zu entries", m_entries.size());

    ImGui::End();
}
