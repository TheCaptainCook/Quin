#include "gui/debug_shell.hpp"
#include "core/logging.hpp"
#include <imgui.h>
#include <cstring>
#include <algorithm>

namespace quin::gui {

DebugShell::DebugShell() {
    QUIN_LOG_INFO("Quin Debug Shell UI initialized.");
}

DebugShell::~DebugShell() = default;

void DebugShell::render() {
    render_menu_bar();
    render_log_pane();
    render_elf_loader_pane();
    render_telemetry_pane();

    if (m_show_about_dialog) {
        render_about_dialog();
    }
}

void DebugShell::render_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load ELF / SELF...", "Ctrl+O")) {
                m_show_file_dialog = true;
                QUIN_LOG_INFO("Open ELF dialog requested.");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                QUIN_LOG_INFO("User requested exit from menu.");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Emulation")) {
            if (ImGui::MenuItem("Run / Resume", nullptr, false, m_elf_loaded)) {
                QUIN_LOG_INFO("Emulation resumed.");
            }
            if (ImGui::MenuItem("Pause", nullptr, false, m_elf_loaded)) {
                QUIN_LOG_INFO("Emulation paused.");
            }
            if (ImGui::MenuItem("Reset", nullptr, false, m_elf_loaded)) {
                QUIN_LOG_INFO("Emulation reset.");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About Quin...")) {
                m_show_about_dialog = true;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void DebugShell::render_log_pane() {
    ImGui::SetNextWindowPos(ImVec2(10, 40), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(780, 360), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Log Console (spdlog)", nullptr)) {
        // Controls Toolbar
        if (ImGui::Button("Clear")) {
            auto sink = quin::core::get_imgui_sink();
            if (sink) sink->clear();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Info", &m_show_info);
        ImGui::SameLine();
        ImGui::Checkbox("Warn", &m_show_warn);
        ImGui::SameLine();
        ImGui::Checkbox("Error", &m_show_error);
        ImGui::SameLine();
        ImGui::Checkbox("Debug", &m_show_debug);
        ImGui::SameLine();
        ImGui::Checkbox("Auto-Scroll", &m_auto_scroll);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputText("Filter", m_search_filter, sizeof(m_search_filter));

        ImGui::Separator();

        // Scrolling Region
        ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        auto sink = quin::core::get_imgui_sink();
        if (sink) {
            auto entries = sink->get_entries();
            for (const auto& entry : entries) {
                // Apply Severity Filters
                if (entry.level == spdlog::level::info && !m_show_info) continue;
                if (entry.level == spdlog::level::warn && !m_show_warn) continue;
                if (entry.level == spdlog::level::err && !m_show_error) continue;
                if (entry.level == spdlog::level::debug && !m_show_debug) continue;

                // Apply Text Filter
                if (strlen(m_search_filter) > 0) {
                    if (entry.payload.find(m_search_filter) == std::string::npos) {
                        continue;
                    }
                }

                // Colorize Output
                ImVec4 color(1.0f, 1.0f, 1.0f, 1.0f);
                if (entry.level == spdlog::level::warn) {
                    color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // Yellow
                } else if (entry.level == spdlog::level::err || entry.level == spdlog::level::critical) {
                    color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red
                } else if (entry.level == spdlog::level::debug || entry.level == spdlog::level::trace) {
                    color = ImVec4(0.6f, 0.6f, 0.9f, 1.0f); // Soft Blue
                } else if (entry.level == spdlog::level::info) {
                    color = ImVec4(0.4f, 0.9f, 0.4f, 1.0f); // Soft Green
                }

                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextUnformatted(( "[" + entry.time_str + "] " + entry.payload ).c_str());
                ImGui::PopStyleColor();
            }

            if (m_auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                ImGui::SetScrollHereY(1.0f);
            }
        }

        ImGui::EndChild();
    }
    ImGui::End();
}

void DebugShell::render_elf_loader_pane() {
    ImGui::SetNextWindowPos(ImVec2(800, 40), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380, 240), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Executable Loader Status", nullptr)) {
        if (!m_elf_loaded) {
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "No executable loaded.");
            ImGui::Spacing();
            if (ImGui::Button("Load Sample ELF File")) {
                m_elf_loaded = true;
                m_loaded_file_path = "samples/hello_world.elf";
                m_elf_entry_point = "0x0000000000400080";
                m_elf_size_bytes = 1048576; // 1 MB
                QUIN_LOG_INFO("Sample ELF loaded: {} (Entry: {})", m_loaded_file_path, m_elf_entry_point);
            }
        } else {
            ImGui::Text("File Path: %s", m_loaded_file_path.c_str());
            ImGui::Text("Entry Point: %s", m_elf_entry_point.c_str());
            ImGui::Text("Size: %.2f MB", static_cast<double>(m_elf_size_bytes) / (1024.0 * 1024.0));
            ImGui::Text("Architecture: x86-64 (PS5 ABI)");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "Status: Ready to Execute (Phase 1)");

            ImGui::Spacing();
            if (ImGui::Button("Unload Binary")) {
                m_elf_loaded = false;
                QUIN_LOG_INFO("Executable unloaded.");
            }
        }
    }
    ImGui::End();
}

void DebugShell::render_telemetry_pane() {
    ImGui::SetNextWindowPos(ImVec2(800, 290), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380, 210), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("System Telemetry", nullptr)) {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("Application Performance:");
        ImGui::Text("Framerate: %.1f FPS", io.Framerate);
        ImGui::Text("Frame Time: %.3f ms", 1000.0f / io.Framerate);

        ImGui::Separator();
        ImGui::Text("Guest Memory Allocation:");
        ImGui::Text("Mapped Virt Memory: 0 MB");
        ImGui::Text("Active Guest Threads: 0");

        ImGui::Separator();
        ImGui::Text("Build Architecture: x86-64 Native");
        ImGui::Text("Graphics API: OpenGL / ImGui");
    }
    ImGui::End();
}

void DebugShell::render_about_dialog() {
    ImGui::OpenPopup("About Quin");
    if (ImGui::BeginPopupModal("About Quin", &m_show_about_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Quin PS5 Emulator — Phase 0 Foundation");
        ImGui::Separator();
        ImGui::Text("A lean x86-64 translation layer and system emulator.");
        ImGui::Text("License: BSD 3-Clause License");
        ImGui::Text("Clean-room development with zero proprietary code.");
        ImGui::Spacing();

        if (ImGui::Button("Close")) {
            m_show_about_dialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

} // namespace quin::gui
