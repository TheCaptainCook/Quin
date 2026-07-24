#include "kernel/modules/sce_user_service.hpp"
#include "core/logging.hpp"

namespace quin::kernel::modules {

void register_sce_user_service(LibKernel& kernel) {
    kernel.register_stub("sceUserServiceInitialize", [](uint64_t, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("libSceUserService: sceUserServiceInitialize()");
        return 0; // Success
    });

    kernel.register_stub("sceUserServiceGetInitialUser", [](uint64_t, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("libSceUserService: sceUserServiceGetInitialUser()");
        return 1000;
    });

    kernel.register_stub("sceUserServiceGetUserName", [](uint64_t user_id, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("libSceUserService: sceUserServiceGetUserName(UserID: {})", user_id);
        return 0;
    });

    QUIN_LOG_INFO("Module 'libSceUserService' registered.");
}

} // namespace quin::kernel::modules
