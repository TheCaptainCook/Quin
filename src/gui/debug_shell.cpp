#include "gui/debug_shell.hpp"
#include "core/logging.hpp"
#include "cpu/exception_handler.hpp"
#include "fs/decompression/kraken_decoder.hpp"
#include <imgui.h>
#include <cstring>
#include <algorithm>

namespace quin::gui {

DebugShell::DebugShell()
    : m_execution_engine(m_address_space, m_kernel),
      m_module_manager(m_kernel, m_execution_engine.get_syscall_dispatcher()),
      m_savedata_mgr(m_execution_engine.get_syscall_dispatcher().get_vfs()),
      m_gpu_parser(m_address_space),
      m_async_shader_compiler(m_shader_recompiler, m_shader_cache, 2) {
    m_module_manager.register_all_modules();
    m_savedata_mgr.mount_savedata(quin::fs::SaveDataConfig{1000, "CUSA00001", 32 * 1024 * 1024});
    m_vulkan_backend.initialize();
    m_audio_engine.initialize();
    m_input_manager.initialize();
    m_async_shader_compiler.start();
    m_pso_disk_cache.load_from_disk();

    // Open default DualSense pad for UI telemetry demonstration
    m_input_manager.open_pad(0);

    // Open default audio port for UI telemetry demonstration
    quin::audio::AudioOutPortConfig config{};
    config.channel_count = 2;
    config.sample_rate = 48000;
    m_audio_engine.open_port(config);

    // Populate synthetic triage logs
    m_triage.log_missing_symbol("sceGnmSubmitDone", "libSceGnmDriver");
    m_triage.log_missing_symbol("sceAudioOutGetPortState", "libSceAudioOut");
    m_triage.log_missing_symbol("scePadGetControllerInformation", "libScePad");

    // Demonstrate shader recompilation of synthetic vertex/pixel shaders
    uint32_t dummy_vs[] = { 0x7E000200, 0x7E020201, 0xBF810000 };
    uint32_t dummy_ps[] = { 0x7E000202, 0x7E020203, 0xBF810000 };

    auto vs_res = m_shader_recompiler.recompile(dummy_vs, sizeof(dummy_vs), quin::gpu::shader::ShaderType::Vertex);
    if (vs_res.success) {
        m_shader_cache.put(vs_res.shader);
        quin::gpu::PsoKey vs_key{};
        vs_key.rt_format = quin::gpu::GnmSurfaceFormat::R8G8B8A8_UNORM;
        m_pso_disk_cache.put_record(vs_key, 1001);
    }

    auto ps_res = m_shader_recompiler.recompile(dummy_ps, sizeof(dummy_ps), quin::gpu::shader::ShaderType::Pixel);
    if (ps_res.success) {
        m_shader_cache.put(ps_res.shader);
    }

    m_pso_disk_cache.save_to_disk();
    QUIN_LOG_INFO("Quin Debug Shell UI initialized with Dynamic Responsive Screen Scaling.");
}

DebugShell::~DebugShell() {
    m_async_shader_compiler.stop();
}

void DebugShell::render() {
    m_input_manager.poll_input();
    m_frame_pacing.begin_frame();

    render_menu_bar();

    // Create a dynamic full-screen workspace window matching host application size
    ImGuiIO& io = ImGui::GetIO();
    float menu_bar_h = 24.0f;
    ImGui::SetNextWindowPos(ImVec2(0.0f, menu_bar_h), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - menu_bar_h), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    if (ImGui::Begin("Quin Main Workspace Window", nullptr, flags)) {
        if (ImGui::BeginTabBar("QuinMainWorkspaceTabBar", ImGuiTabBarFlags_Reorderable)) {
            
            // -----------------------------------------------------------------
            // TAB 1: Main Overview & Dashboard
            // -----------------------------------------------------------------
            if (ImGui::BeginTabItem("Main Dashboard")) {
                float avail_w = ImGui::GetContentRegionAvail().x;
                float avail_h = ImGui::GetContentRegionAvail().y;
                float col1_w = avail_w * 0.60f;
                float col2_w = avail_w - col1_w - 10.0f;

                ImGui::BeginChild("DashboardLeftCol", ImVec2(col1_w, avail_h), false);
                render_log_pane();
                render_performance_pane();
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("DashboardRightCol", ImVec2(col2_w, avail_h), false);
                render_elf_loader_pane();
                render_telemetry_pane();
                ImGui::EndChild();

                ImGui::EndTabItem();
            }

            // -----------------------------------------------------------------
            // TAB 2: CPU & Kernel Architecture
            // -----------------------------------------------------------------
            if (ImGui::BeginTabItem("CPU Execution & Kernel")) {
                float avail_w = ImGui::GetContentRegionAvail().x;
                float avail_h = ImGui::GetContentRegionAvail().y;

                ImGui::BeginChild("CpuLeftCol", ImVec2(avail_w * 0.55f, avail_h), false);
                render_threads_pane();
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("CpuRightCol", ImVec2(avail_w * 0.44f, avail_h), false);
                render_syscalls_pane();
                ImGui::EndChild();

                ImGui::EndTabItem();
            }

            // -----------------------------------------------------------------
            // TAB 3: GPU & Shader Recompiler
            // -----------------------------------------------------------------
            if (ImGui::BeginTabItem("GPU Rendering & Shaders")) {
                float avail_w = ImGui::GetContentRegionAvail().x;
                float avail_h = ImGui::GetContentRegionAvail().y;

                ImGui::BeginChild("GpuLeftCol", ImVec2(avail_w * 0.45f, avail_h), false);
                render_gpu_pane();
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("GpuRightCol", ImVec2(avail_w * 0.54f, avail_h), false);
                render_shader_pane();
                ImGui::EndChild();

                ImGui::EndTabItem();
            }

            // -----------------------------------------------------------------
            // TAB 4: Audio & DualSense Input
            // -----------------------------------------------------------------
            if (ImGui::BeginTabItem("Audio & DualSense Input")) {
                float avail_w = ImGui::GetContentRegionAvail().x;
                float avail_h = ImGui::GetContentRegionAvail().y;

                ImGui::BeginChild("AudioInputLeftCol", ImVec2(avail_w * 0.50f, avail_h), false);
                render_audio_pane();
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("AudioInputRightCol", ImVec2(avail_w * 0.49f, avail_h), false);
                render_input_pane();
                ImGui::EndChild();

                ImGui::EndTabItem();
            }

            // -----------------------------------------------------------------
            // TAB 5: Storage & Virtual Filesystem
            // -----------------------------------------------------------------
            if (ImGui::BeginTabItem("Storage & VFS")) {
                render_vfs_pane();
                ImGui::EndTabItem();
            }

            // -----------------------------------------------------------------
            // TAB 6: Compatibility & Stub Triage
            // -----------------------------------------------------------------
            if (ImGui::BeginTabItem("Compatibility & Triage")) {
                render_compat_pane();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();

    if (m_show_about_dialog) {
        render_about_dialog();
    }

    m_frame_pacing.end_frame();
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
            bool can_run = m_elf_loaded && (m_execution_engine.get_state() == quin::cpu::CpuState::Ready ||
                                           m_execution_engine.get_state() == quin::cpu::CpuState::Paused);
            if (ImGui::MenuItem("Run / Step", nullptr, false, can_run)) {
                m_execution_engine.step();
            }
            if (ImGui::MenuItem("Spawn Guest Thread", nullptr, false, m_elf_loaded)) {
                m_execution_engine.spawn_thread("worker_thread", 0x0000000000400080ULL, 0x1234);
            }
            if (ImGui::MenuItem("Pause", nullptr, false, m_execution_engine.get_state() == quin::cpu::CpuState::Running)) {
                m_execution_engine.pause();
            }
            if (ImGui::MenuItem("Reset", nullptr, false, m_elf_loaded)) {
                m_execution_engine.reset();
                m_elf_loaded = false;
                QUIN_LOG_INFO("Emulation state reset.");
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
    float avail_h = ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("LogConsoleSubChild", ImVec2(0, avail_h * 0.60f), true);

    ImGui::Text("Log Console (spdlog)");
    ImGui::Separator();

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

    ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    auto sink = quin::core::get_imgui_sink();
    if (sink) {
        auto entries = sink->get_entries();
        for (const auto& entry : entries) {
            if (entry.level == spdlog::level::info && !m_show_info) continue;
            if (entry.level == spdlog::level::warn && !m_show_warn) continue;
            if (entry.level == spdlog::level::err && !m_show_error) continue;
            if (entry.level == spdlog::level::debug && !m_show_debug) continue;

            if (strlen(m_search_filter) > 0) {
                if (entry.payload.find(m_search_filter) == std::string::npos) {
                    continue;
                }
            }

            ImVec4 color(1.0f, 1.0f, 1.0f, 1.0f);
            if (entry.level == spdlog::level::warn) {
                color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
            } else if (entry.level == spdlog::level::err || entry.level == spdlog::level::critical) {
                color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            } else if (entry.level == spdlog::level::debug || entry.level == spdlog::level::trace) {
                color = ImVec4(0.6f, 0.6f, 0.9f, 1.0f);
            } else if (entry.level == spdlog::level::info) {
                color = ImVec4(0.4f, 0.9f, 0.4f, 1.0f);
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
    ImGui::EndChild();
}

void DebugShell::render_elf_loader_pane() {
    float avail_h = ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("ElfLoaderSubChild", ImVec2(0, avail_h * 0.55f), true);

    ImGui::Text("Executable Loader Status");
    ImGui::Separator();

    if (!m_elf_loaded) {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "No executable loaded in guest memory.");
        ImGui::Spacing();
        if (ImGui::Button("Load & Bootstrap Homebrew ELF")) {
            std::vector<uint8_t> elf_buffer(256, 0);

            auto* ehdr = reinterpret_cast<quin::loader::Elf64_Ehdr*>(elf_buffer.data());
            ehdr->e_ident[0] = quin::loader::ELF_MAG0;
            ehdr->e_ident[1] = quin::loader::ELF_MAG1;
            ehdr->e_ident[2] = quin::loader::ELF_MAG2;
            ehdr->e_ident[3] = quin::loader::ELF_MAG3;
            ehdr->e_ident[4] = quin::loader::ELFCLASS64;
            ehdr->e_ident[5] = quin::loader::ELFDATA2LSB;
            ehdr->e_type = quin::loader::ET_EXEC;
            ehdr->e_machine = quin::loader::EM_X86_64;
            ehdr->e_entry = 0x0000000000400080ULL;
            ehdr->e_phoff = sizeof(quin::loader::Elf64_Ehdr);
            ehdr->e_phnum = 1;

            auto* phdr = reinterpret_cast<quin::loader::Elf64_Phdr*>(elf_buffer.data() + sizeof(quin::loader::Elf64_Ehdr));
            phdr->p_type = quin::loader::PT_LOAD;
            phdr->p_flags = quin::loader::PF_R | quin::loader::PF_X;
            phdr->p_offset = 0;
            phdr->p_vaddr = 0x0000000000400000ULL;
            phdr->p_filesz = elf_buffer.size();
            phdr->p_memsz = 4096;

            uint8_t* payload = elf_buffer.data() + 0x80;
            payload[0] = 0x90; // NOP
            payload[1] = 0x90; // NOP
            payload[2] = 0xC3; // RET

            m_parsed_elf = quin::loader::SelfParser::parse_buffer(elf_buffer);
            quin::loader::ElfLoader loader(m_address_space);
            m_load_result = loader.load(m_parsed_elf);

            if (m_load_result.success) {
                m_elf_loaded = true;
                m_loaded_file_path = "samples/homebrew_hello.elf";
                m_execution_engine.bootstrap(m_load_result.entry_point, m_load_result.stack_top);
            }
        }
    } else {
        ImGui::Text("File Path: %s", m_loaded_file_path.c_str());
        ImGui::Text("Entry Point: 0x%016llX", m_load_result.entry_point);
        ImGui::Text("Stack Top: 0x%016llX", m_load_result.stack_top);
        ImGui::Text("Mapped Segments: %zu", m_load_result.loaded_segments_count);
        ImGui::Text("Mapped Bytes: %.2f KB", static_cast<double>(m_load_result.total_mapped_bytes) / 1024.0);
        ImGui::Spacing();

        const auto& regs = m_execution_engine.get_registers();
        ImGui::Text("CPU State: RIP=0x%016llX | RSP=0x%016llX", regs.rip, regs.rsp);

        ImGui::Spacing();
        if (ImGui::Button("Step Instruction")) {
            m_execution_engine.step();
        }
        ImGui::SameLine();
        if (ImGui::Button("Unload Binary")) {
            m_execution_engine.reset();
            m_elf_loaded = false;
            QUIN_LOG_INFO("Executable unloaded from memory.");
        }
    }
    ImGui::EndChild();
}

void DebugShell::render_threads_pane() {
    ImGui::BeginChild("ThreadsSubChild", ImVec2(0, 0), true);
    ImGui::Text("Threads & TLS Manager");
    ImGui::Separator();

    const auto& thread_mgr = m_execution_engine.get_thread_manager();
    auto active_threads = thread_mgr.get_active_threads_info();

    ImGui::Text("Active Guest Threads: %zu", active_threads.size());
    ImGui::Spacing();

    if (ImGui::BeginTable("ThreadsTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("TID");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("State");
        ImGui::TableSetupColumn("RIP");
        ImGui::TableSetupColumn("RSP");
        ImGui::TableSetupColumn("TLS Base");
        ImGui::TableHeadersRow();

        for (const auto& t : active_threads) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%u", t.id);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", t.name.c_str());
            ImGui::TableSetColumnIndex(2);
            const char* state_str = "Unknown";
            if (t.state == quin::cpu::ThreadState::Ready) state_str = "Ready";
            else if (t.state == quin::cpu::ThreadState::Running) state_str = "Running";
            else if (t.state == quin::cpu::ThreadState::Waiting) state_str = "Waiting";
            else if (t.state == quin::cpu::ThreadState::Terminated) state_str = "Terminated";
            ImGui::Text("%s", state_str);
            ImGui::TableSetColumnIndex(3); ImGui::Text("0x%016llX", t.rip);
            ImGui::TableSetColumnIndex(4); ImGui::Text("0x%016llX", t.rsp);
            ImGui::TableSetColumnIndex(5); ImGui::Text("0x%016llX", t.tls_base);
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}

void DebugShell::render_syscalls_pane() {
    ImGui::BeginChild("SyscallsSubChild", ImVec2(0, 0), true);
    ImGui::Text("Syscalls & System Modules");
    ImGui::Separator();

    const auto& dispatcher = m_execution_engine.get_syscall_dispatcher();
    auto syscalls = dispatcher.get_registered_syscalls();

    ImGui::Text("Total Syscalls Dispatched: %llu", dispatcher.get_total_syscall_calls());
    ImGui::Text("Registered Syscalls Count: %zu", syscalls.size());
    ImGui::Spacing();

    if (ImGui::BeginTable("SyscallsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Syscall #");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Call Count");
        ImGui::TableHeadersRow();

        for (const auto& sys : syscalls) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("#%llu", sys.num);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", sys.name.c_str());
            ImGui::TableSetColumnIndex(2); ImGui::Text("%llu", sys.call_count);
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}

void DebugShell::render_vfs_pane() {
    ImGui::BeginChild("VfsSubChild", ImVec2(0, 0), true);
    ImGui::Text("Virtual Filesystem & Storage");
    ImGui::Separator();

    const auto& vfs = m_execution_engine.get_syscall_dispatcher().get_vfs();
    auto mounts = vfs.get_mount_points();
    auto open_files = vfs.get_open_files();

    ImGui::Text("Total Read: %.2f KB | Total Written: %.2f KB | Decompressed: %.2f KB",
                static_cast<double>(vfs.get_total_read_bytes()) / 1024.0,
                static_cast<double>(vfs.get_total_written_bytes()) / 1024.0,
                static_cast<double>(quin::fs::decompression::KrakenDecoder::get_total_decompressed_bytes()) / 1024.0);
    ImGui::Separator();

    if (ImGui::TreeNode("Active VFS Mount Points")) {
        for (const auto& m : mounts) {
            ImGui::Text("Prefix: '%s' -> Host: '%s'", m.virtual_prefix.c_str(), m.host_directory.c_str());
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Open File Handles")) {
        for (const auto& f : open_files) {
            ImGui::Text("FD: %d | Guest: '%s' | Size: %llu bytes | Pos: %llu",
                        f.handle, f.virtual_path.c_str(), f.size, f.position);
        }
        ImGui::TreePop();
    }
    ImGui::EndChild();
}

void DebugShell::render_gpu_pane() {
    ImGui::BeginChild("GpuSubChild", ImVec2(0, 0), true);
    ImGui::Text("GPU & Vulkan Backend");
    ImGui::Separator();

    const auto& dev_info = m_vulkan_backend.get_device_info();
    ImGui::Text("Physical GPU: %s", dev_info.device_name.c_str());
    ImGui::Text("Driver Version: %s", dev_info.driver_version.c_str());
    ImGui::Text("VRAM: %.2f GB", static_cast<double>(dev_info.vram_bytes) / (1024.0 * 1024.0 * 1024.0));
    ImGui::Separator();

    ImGui::Text("PM4 Packets Parsed: %llu", m_gpu_parser.get_total_packets_parsed());
    ImGui::Text("GNM Draw Calls Parsed: %llu", m_gpu_parser.get_total_draw_calls());
    ImGui::Text("Rendered Draw Calls: %llu", m_vulkan_backend.get_total_draw_calls_rendered());
    ImGui::Text("Cached Vulkan PSOs: %zu", m_vulkan_backend.get_cached_pipelines_count());
    ImGui::Text("PSO Cache Hits: %llu", m_vulkan_backend.get_total_pso_cache_hits());
    ImGui::EndChild();
}

void DebugShell::render_shader_pane() {
    ImGui::BeginChild("ShaderSubChild", ImVec2(0, 0), true);
    ImGui::Text("Shader Recompiler & SPIR-V Cache");
    ImGui::Separator();

    ImGui::Text("Total Recompiled Shaders: %llu", m_shader_recompiler.get_total_shaders_compiled());
    ImGui::Text("Cached SPIR-V Shaders: %zu", m_shader_cache.get_cached_shader_count());
    ImGui::Text("Shader Cache Hits: %llu | Misses: %llu", m_shader_cache.get_cache_hits(), m_shader_cache.get_cache_misses());
    ImGui::Separator();

    auto shaders = m_shader_cache.get_all_cached_shaders();
    if (ImGui::BeginTable("ShadersTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Hash Key");
        ImGui::TableSetupColumn("Stage");
        ImGui::TableSetupColumn("SPIR-V Words");
        ImGui::TableSetupColumn("RDNA2 Bytes");
        ImGui::TableHeadersRow();

        for (const auto& sh : shaders) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("0x%016llX", sh.hash);
            ImGui::TableSetColumnIndex(1);
            const char* stage_name = (sh.stage == quin::gpu::shader::ShaderType::Vertex) ? "Vertex" :
                                     ((sh.stage == quin::gpu::shader::ShaderType::Pixel) ? "Pixel" : "Compute");
            ImGui::Text("%s", stage_name);
            ImGui::TableSetColumnIndex(2); ImGui::Text("%zu words", sh.spirv_code.size());
            ImGui::TableSetColumnIndex(3); ImGui::Text("%zu bytes", sh.rdna2_bytecode.size());
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}

void DebugShell::render_audio_pane() {
    ImGui::BeginChild("AudioSubChild", ImVec2(0, 0), true);
    ImGui::Text("Audio Subsystem & Tempest 3D");
    ImGui::Separator();

    ImGui::Text("Host Audio Backend: SDL2 Audio (Master 48 kHz)");
    ImGui::Text("Audio Engine Status: %s", m_audio_engine.is_initialized() ? "Active" : "Inactive");
    ImGui::Text("Open Audio Ports: %zu", m_audio_engine.get_open_ports_count());
    ImGui::Text("Total PCM Samples Processed: %llu", m_audio_engine.get_total_samples_processed());
    ImGui::Separator();

    auto ports = m_audio_engine.get_active_ports();
    for (const auto& p : ports) {
        ImGui::Text("Port #%d | Rate: %u Hz | Channels: %u | Submitted: %llu",
                    p.handle, p.config.sample_rate, p.config.channel_count, p.total_samples_submitted);
    }
    ImGui::EndChild();
}

void DebugShell::render_input_pane() {
    ImGui::BeginChild("InputSubChild", ImVec2(0, 0), true);
    ImGui::Text("Input Subsystem & DualSense");
    ImGui::Separator();

    ImGui::Text("Connected Controllers: %zu", m_input_manager.get_connected_pads_count());
    ImGui::Separator();

    auto pads = m_input_manager.get_all_pads();
    for (const auto& pad : pads) {
        ImGui::Text("DualSense Pad #%d | Status: Connected", pad.handle);
        ImGui::Text("Buttons Mask: 0x%08X", pad.buttons);
        ImGui::Text("Left Stick: (%d, %d) | Right Stick: (%d, %d)",
                    pad.left_stick_x, pad.left_stick_y, pad.right_stick_x, pad.right_stick_y);
        ImGui::Text("Lightbar RGB: (%u, %u, %u)", pad.lightbar_color.r, pad.lightbar_color.g, pad.lightbar_color.b);
        ImGui::Spacing();

        if (ImGui::Button("Test Trigger Cross (A)")) {
            m_input_manager.set_button_state(pad.handle, quin::input::PAD_CROSS, true);
        }
        ImGui::SameLine();
        if (ImGui::Button("Test Trigger Circle (B)")) {
            m_input_manager.set_button_state(pad.handle, quin::input::PAD_CIRCLE, true);
        }
    }
    ImGui::EndChild();
}

void DebugShell::render_compat_pane() {
    ImGui::BeginChild("CompatSubChild", ImVec2(0, 0), true);
    ImGui::Text("Compatibility Tracker & Stub Triage");
    ImGui::Separator();

    ImGui::Text("Playable Titles: %zu | Perfect: %zu",
                m_title_db.get_count_by_status(quin::compat::CompatStatus::Playable),
                m_title_db.get_count_by_status(quin::compat::CompatStatus::Perfect));
    ImGui::Text("Total Unimplemented Calls: %llu", m_triage.get_total_unimplemented_calls());
    ImGui::Separator();

    if (ImGui::TreeNode("Top Missing System Symbols")) {
        auto missing = m_triage.get_top_missing_symbols(5);
        for (const auto& m : missing) {
            ImGui::Text("%s (%s) — %llu calls", m.symbol_name.c_str(), m.module_name.c_str(), m.call_count);
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Tracked Game Titles")) {
        auto titles = m_title_db.get_all_titles();
        for (const auto& t : titles) {
            ImGui::Text("[%s] %s — %s", t.title_id.c_str(), t.name.c_str(), quin::compat::compat_status_to_string(t.status).c_str());
        }
        ImGui::TreePop();
    }

    ImGui::Spacing();
    if (ImGui::Button("Run Automated Regression Suite")) {
        m_triage.run_regression_suite();
    }
    ImGui::EndChild();
}

void DebugShell::render_performance_pane() {
    ImGui::BeginChild("PerformanceSubChild", ImVec2(0, 0), true);
    ImGui::Text("Performance Tuning & 60FPS Engine");
    ImGui::Separator();

    ImGui::Text("Target Framerate: %.1f FPS (Mode: %s)",
                m_frame_pacing.get_current_fps(),
                m_frame_pacing.get_mode() == quin::gpu::FramePacingMode::Locked60 ? "60 FPS Lock" :
                (m_frame_pacing.get_mode() == quin::gpu::FramePacingMode::Locked30 ? "30 FPS Lock" : "Unlocked"));
    ImGui::Text("Last Frame Time: %.3f ms | Avg Frame Time: %.3f ms",
                m_frame_pacing.get_last_frame_time_ms(), m_frame_pacing.get_avg_frame_time_ms());
    ImGui::Separator();

    if (ImGui::Button("Lock 60 FPS")) m_frame_pacing.set_mode(quin::gpu::FramePacingMode::Locked60);
    ImGui::SameLine();
    if (ImGui::Button("Lock 30 FPS")) m_frame_pacing.set_mode(quin::gpu::FramePacingMode::Locked30);
    ImGui::SameLine();
    if (ImGui::Button("Unlocked FPS")) m_frame_pacing.set_mode(quin::gpu::FramePacingMode::Unlocked);

    ImGui::Spacing();
    float scale = m_frame_pacing.get_resolution_scale();
    if (ImGui::SliderFloat("Dynamic Resolution Scale (FSR)", &scale, 0.5f, 1.0f, "%.2fx")) {
        m_frame_pacing.set_resolution_scale(scale);
    }

    ImGui::Separator();
    ImGui::Text("Async Shader Compiler: %zu Workers | %llu Completed | %zu Pending",
                m_async_shader_compiler.get_worker_threads_count(),
                m_async_shader_compiler.get_completed_jobs_count(),
                m_async_shader_compiler.get_pending_jobs_count());
    ImGui::Text("Disk PSO Cache Directory: '%s' | %zu Persistent PSOs Saved",
                m_pso_disk_cache.get_cache_dir().c_str(), m_pso_disk_cache.get_records_count());
    ImGui::EndChild();
}

void DebugShell::render_telemetry_pane() {
    ImGui::BeginChild("TelemetrySubChild", ImVec2(0, 0), true);
    ImGui::Text("System Telemetry");
    ImGui::Separator();

    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Application Performance:");
    ImGui::Text("Framerate: %.1f FPS", io.Framerate);
    ImGui::Text("Frame Time: %.3f ms", 1000.0f / io.Framerate);

    ImGui::Separator();
    ImGui::Text("Guest Memory Allocation:");
    ImGui::Text("Total Mapped Memory: %.2f MB",
                static_cast<double>(m_address_space.get_total_allocated_bytes()) / (1024.0 * 1024.0));
    ImGui::Text("Allocated Memory Blocks: %zu", m_address_space.get_blocks().size());
    ImGui::Text("Registered libkernel Stubs: %zu", m_kernel.get_stubs().size());
    ImGui::EndChild();
}

void DebugShell::render_about_dialog() {
    ImGui::OpenPopup("About Quin");
    if (ImGui::BeginPopupModal("About Quin", &m_show_about_dialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Quin PS5 Emulator — Dynamic Responsive Workspace");
        ImGui::Separator();
        ImGui::Text("A lean x86-64 translation layer and system emulator.");
        ImGui::Text("License: Non-Commercial Personal Use License");
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
