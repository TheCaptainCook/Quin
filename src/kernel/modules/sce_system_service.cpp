#include "kernel/modules/sce_system_service.hpp"
#include "core/logging.hpp"

namespace quin::kernel::modules {

void register_sce_system_service(LibKernel& kernel) {
    kernel.register_stub("sceSystemServiceParamGetInt", [](uint64_t param_id, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("libSceSystemService: sceSystemServiceParamGetInt(ParamID: {})", param_id);
        return 0; // Success
    });

    kernel.register_stub("sceSystemServiceHideSplashScreen", [](uint64_t, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("libSceSystemService: sceSystemServiceHideSplashScreen()");
        return 0;
    });

    kernel.register_stub("sceSystemServiceGetInitialUser", [](uint64_t, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("libSceSystemService: sceSystemServiceGetInitialUser()");
        return 1000; // Default User ID
    });

    QUIN_LOG_INFO("Module 'libSceSystemService' registered.");
}

} // namespace quin::kernel::modules
