#include "kernel/module_manager.hpp"
#include "kernel/modules/sce_libc.hpp"
#include "kernel/modules/sce_system_service.hpp"
#include "kernel/modules/sce_user_service.hpp"
#include "core/logging.hpp"

namespace quin::kernel {

ModuleManager::ModuleManager(LibKernel& kernel, SyscallDispatcher& syscalls)
    : m_kernel(kernel), m_syscalls(syscalls) {}

void ModuleManager::register_all_modules() {
    modules::register_sce_libc(m_kernel);
    modules::register_sce_system_service(m_kernel);
    modules::register_sce_user_service(m_kernel);

    QUIN_LOG_INFO("ModuleManager: Registered core system modules (libSceLibcInternal, libSceSystemService, libSceUserService).");
}

} // namespace quin::kernel
