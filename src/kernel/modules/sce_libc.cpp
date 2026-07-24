#include "kernel/modules/sce_libc.hpp"
#include "core/logging.hpp"

namespace quin::kernel::modules {

void register_sce_libc(LibKernel& kernel) {
    kernel.register_stub("sceLibcMalloc", [](uint64_t size, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("libSceLibcInternal: sceLibcMalloc({})", size);
        return 0;
    });

    kernel.register_stub("sceLibcFree", [](uint64_t ptr, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("libSceLibcInternal: sceLibcFree(0x{:X})", ptr);
        return 0;
    });

    kernel.register_stub("sceLibcMemset", [](uint64_t dest, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("libSceLibcInternal: sceLibcMemset(0x{:X})", dest);
        return 0;
    });

    kernel.register_stub("sceLibcMemcpy", [](uint64_t dest, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("libSceLibcInternal: sceLibcMemcpy(0x{:X})", dest);
        return 0;
    });

    QUIN_LOG_INFO("Module 'libSceLibcInternal' registered.");
}

} // namespace quin::kernel::modules
