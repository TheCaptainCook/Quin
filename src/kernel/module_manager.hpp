#ifndef QUIN_KERNEL_MODULE_MANAGER_HPP
#define QUIN_KERNEL_MODULE_MANAGER_HPP

#include "kernel/libkernel.hpp"
#include "kernel/syscall_table.hpp"

namespace quin::kernel {

class ModuleManager {
public:
    explicit ModuleManager(LibKernel& kernel, SyscallDispatcher& syscalls);

    void register_all_modules();
    LibKernel& get_kernel() { return m_kernel; }
    SyscallDispatcher& get_syscalls() { return m_syscalls; }

private:
    LibKernel& m_kernel;
    SyscallDispatcher& m_syscalls;
};

} // namespace quin::kernel

#endif // QUIN_KERNEL_MODULE_MANAGER_HPP
