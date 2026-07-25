#include "kernel/libkernel.hpp"
#include "core/logging.hpp"
#include <chrono>
#include <thread>

namespace quin::kernel {

LibKernel::LibKernel() {
    register_default_stubs();
}

void LibKernel::register_stub(const std::string& symbol_name, StubHandler handler) {
    m_stubs[symbol_name] = handler;
}

bool LibKernel::has_symbol(const std::string& symbol_name) const {
    return m_stubs.find(symbol_name) != m_stubs.end();
}

int64_t LibKernel::dispatch_symbol(const std::string& symbol_name, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4) {
    auto it = m_stubs.find(symbol_name);
    if (it != m_stubs.end()) {
        QUIN_LOG_INFO("LibKernel: Dispatching stub for symbol '{}' (Args: 0x{:X}, 0x{:X}, 0x{:X}, 0x{:X})",
                      symbol_name, a1, a2, a3, a4);
        return it->second(a1, a2, a3, a4);
    }

    QUIN_LOG_WARN("LibKernel: Unimplemented symbol call hit: '{}' (Default return 0)", symbol_name);
    return 0; // Return success fallback for unimplemented symbols
}

void LibKernel::register_default_stubs() {
    register_stub("sceKernelExitProcess", [](uint64_t exit_code, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("sceKernelExitProcess called with exit code {}", exit_code);
        return 0;
    });

    register_stub("sceKernelWrite", [](uint64_t fd, uint64_t buf_ptr, uint64_t count, uint64_t) -> int64_t {
        QUIN_LOG_INFO("sceKernelWrite called (FD: {}, BufPtr: 0x{:016X}, Count: {})", fd, buf_ptr, count);
        return static_cast<int64_t>(count);
    });

    register_stub("__stack_chk_fail", [](uint64_t, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_CRITICAL("__stack_chk_fail called — stack corruption protection triggered!");
        return -1;
    });

    register_stub("sceKernelGetProcessTime", [](uint64_t time_ptr, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("sceKernelGetProcessTime called");
        return 0;
    });

    // ---- New stubs below ----

    register_stub("sceKernelLoadStartModule", [](uint64_t path_ptr, uint64_t argc, uint64_t argv, uint64_t) -> int64_t {
        QUIN_LOG_INFO("sceKernelLoadStartModule: path=0x{:X}, argc={}", path_ptr, argc);
        return 0x2001; // Return a fake module handle
    });

    register_stub("sceKernelDlsym", [](uint64_t handle, uint64_t sym_ptr, uint64_t out_ptr, uint64_t) -> int64_t {
        QUIN_LOG_INFO("sceKernelDlsym: handle=0x{:X}, sym=0x{:X}", handle, sym_ptr);
        return 0; // Success — symbol resolved (stub)
    });

    register_stub("sceKernelUsleep", [](uint64_t usec, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_DEBUG("sceKernelUsleep: {} microseconds", usec);
        if (usec > 0 && usec < 10000000) {
            std::this_thread::sleep_for(std::chrono::microseconds(usec));
        }
        return 0;
    });

    register_stub("sceKernelStat", [](uint64_t path_ptr, uint64_t stat_ptr, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_DEBUG("sceKernelStat: path=0x{:X}, stat=0x{:X}", path_ptr, stat_ptr);
        return 0;
    });

    register_stub("sceKernelGetFsSandboxRandomWord", [](uint64_t out_ptr, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_DEBUG("sceKernelGetFsSandboxRandomWord: out=0x{:X}", out_ptr);
        return 0;
    });

    register_stub("sceKernelOpen", [](uint64_t path_ptr, uint64_t flags, uint64_t mode, uint64_t) -> int64_t {
        QUIN_LOG_INFO("sceKernelOpen: path=0x{:X}, flags=0x{:X}, mode=0x{:X}", path_ptr, flags, mode);
        return 100; // Return a fake fd
    });

    register_stub("sceKernelClose", [](uint64_t fd, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_DEBUG("sceKernelClose: fd={}", fd);
        return 0;
    });

    register_stub("sceKernelRead", [](uint64_t fd, uint64_t buf_ptr, uint64_t count, uint64_t) -> int64_t {
        QUIN_LOG_DEBUG("sceKernelRead: fd={}, buf=0x{:X}, count={}", fd, buf_ptr, count);
        return 0; // EOF
    });

    register_stub("sceKernelLseek", [](uint64_t fd, uint64_t offset, uint64_t whence, uint64_t) -> int64_t {
        QUIN_LOG_DEBUG("sceKernelLseek: fd={}, offset={}, whence={}", fd, offset, whence);
        return static_cast<int64_t>(offset);
    });

    register_stub("sceKernelMmap", [](uint64_t addr, uint64_t len, uint64_t prot, uint64_t flags) -> int64_t {
        QUIN_LOG_INFO("sceKernelMmap: addr=0x{:X}, len={}, prot={}, flags={}", addr, len, prot, flags);
        return 0;
    });

    register_stub("sceKernelMunmap", [](uint64_t addr, uint64_t len, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_DEBUG("sceKernelMunmap: addr=0x{:X}, len={}", addr, len);
        return 0;
    });

    register_stub("sceKernelCreateEqueue", [](uint64_t eq_out, uint64_t name_ptr, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("sceKernelCreateEqueue: out=0x{:X}", eq_out);
        return 0;
    });

    register_stub("sceKernelDeleteEqueue", [](uint64_t eq, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_DEBUG("sceKernelDeleteEqueue: eq=0x{:X}", eq);
        return 0;
    });

    register_stub("sceKernelWaitEqueue", [](uint64_t eq, uint64_t events_out, uint64_t max, uint64_t timeout_ptr) -> int64_t {
        QUIN_LOG_DEBUG("sceKernelWaitEqueue: eq=0x{:X}, max={}", eq, max);
        return 0; // 0 events returned
    });

    register_stub("sceKernelCreateEventFlag", [](uint64_t ef_out, uint64_t name_ptr, uint64_t attr, uint64_t init_pattern) -> int64_t {
        QUIN_LOG_INFO("sceKernelCreateEventFlag: out=0x{:X}, attr={}, init=0x{:X}", ef_out, attr, init_pattern);
        return 0;
    });

    register_stub("sceKernelSetEventFlag", [](uint64_t ef, uint64_t pattern, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_DEBUG("sceKernelSetEventFlag: ef=0x{:X}, pattern=0x{:X}", ef, pattern);
        return 0;
    });

    register_stub("sceKernelGetProcessTimeCounter", [](uint64_t, uint64_t, uint64_t, uint64_t) -> int64_t {
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        return std::chrono::duration_cast<std::chrono::microseconds>(now).count();
    });

    register_stub("sceKernelGetProcessTimeCounterFrequency", [](uint64_t, uint64_t, uint64_t, uint64_t) -> int64_t {
        return 1000000; // 1 MHz
    });

    QUIN_LOG_INFO("LibKernel: Registered {} default system stubs.", m_stubs.size());
}

} // namespace quin::kernel
