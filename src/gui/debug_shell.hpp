#ifndef QUIN_GUI_DEBUG_SHELL_HPP
#define QUIN_GUI_DEBUG_SHELL_HPP

#include "memory/address_space.hpp"
#include "kernel/libkernel.hpp"
#include "cpu/execution_engine.hpp"
#include "loader/self_parser.hpp"
#include "loader/elf_loader.hpp"

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

    // Phase 1 Engine Subsystems
    quin::memory::GuestAddressSpace m_address_space;
    quin::kernel::LibKernel m_kernel;
    quin::cpu::ExecutionEngine m_execution_engine;
    quin::loader::ParsedElf m_parsed_elf;
    quin::loader::LoadResult m_load_result;

    // Loaded ELF state
    bool m_elf_loaded{false};
    std::string m_loaded_file_path;

    // Dialog flags
    bool m_show_file_dialog{false};
    bool m_show_about_dialog{false};
};

} // namespace quin::gui

#endif // QUIN_GUI_DEBUG_SHELL_HPP
