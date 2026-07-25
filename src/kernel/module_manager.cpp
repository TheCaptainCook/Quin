#include "kernel/module_manager.hpp"
#include "kernel/modules/sce_libc.hpp"
#include "kernel/modules/sce_system_service.hpp"
#include "kernel/modules/sce_user_service.hpp"
#include "kernel/modules/sce_np_trophy.hpp"
#include "core/logging.hpp"

namespace quin::kernel {

void register_sce_audio_out(LibKernel& kernel);
void register_sce_pad(LibKernel& kernel);

ModuleManager::ModuleManager(LibKernel& kernel, SyscallDispatcher& syscalls)
    : m_kernel(kernel), m_syscalls(syscalls) {}

void ModuleManager::register_all_modules() {
    modules::register_sce_libc(m_kernel);
    modules::register_sce_system_service(m_kernel);
    modules::register_sce_user_service(m_kernel);
    modules::register_sce_np_trophy(m_kernel);
    register_sce_audio_out(m_kernel);
    register_sce_pad(m_kernel);

    QUIN_LOG_INFO("ModuleManager: Registered core system modules (libSceLibcInternal, libSceSystemService, libSceUserService, libSceNpTrophy, libSceAudioOut, libScePad).");
}

} // namespace quin::kernel
