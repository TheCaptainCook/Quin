#include "kernel/libkernel.hpp"
#include "core/logging.hpp"

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

    QUIN_LOG_INFO("LibKernel: Registered {} default system stubs.", m_stubs.size());
}

} // namespace quin::kernel
