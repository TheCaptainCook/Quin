#ifndef QUIN_GUI_DEBUG_SHELL_HPP
#define QUIN_GUI_DEBUG_SHELL_HPP

#include <string>
#include <vector>
#include <memory>

namespace quin::gui {

class DebugShell {
public:
    DebugShell();
    ~DebugShell();

    void render();

private:
    void render_menu_bar();
    void render_log_pane();
    void render_elf_loader_pane();
    void render_telemetry_pane();
    void render_about_dialog();

    // Log pane filters
    bool m_show_info{true};
    bool m_show_warn{true};
    bool m_show_error{true};
    bool m_show_debug{true};
    bool m_auto_scroll{true};
    char m_search_filter[128]{""};

    // Loaded ELF state (Phase 0 stub)
    bool m_elf_loaded{false};
    std::string m_loaded_file_path;
    std::string m_elf_entry_point;
    size_t m_elf_size_bytes{0};

    // Dialog flags
    bool m_show_file_dialog{false};
    bool m_show_about_dialog{false};
};

} // namespace quin::gui

#endif // QUIN_GUI_DEBUG_SHELL_HPP
