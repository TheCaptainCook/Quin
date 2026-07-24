#ifndef QUIN_KERNEL_SYSCALL_TABLE_HPP
#define QUIN_KERNEL_SYSCALL_TABLE_HPP

#include "memory/address_space.hpp"
#include "fs/vfs.hpp"
#include <cstdint>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace quin::kernel {

// Standard FreeBSD / PS5 Syscall Numbers
constexpr uint64_t SYS_exit            = 1;
constexpr uint64_t SYS_read            = 3;
constexpr uint64_t SYS_write           = 4;
constexpr uint64_t SYS_open            = 5;
constexpr uint64_t SYS_close           = 6;
constexpr uint64_t SYS_getpid          = 20;
constexpr uint64_t SYS_munmap          = 73;
constexpr uint64_t SYS_mprotect        = 74;
constexpr uint64_t SYS_clock_gettime   = 232;
constexpr uint64_t SYS_thr_exit        = 431;
constexpr uint64_t SYS_thr_self        = 432;
constexpr uint64_t SYS_thr_new         = 455;
constexpr uint64_t SYS_mmap            = 477;
constexpr uint64_t SYS_dynlib_dlsym    = 591;
constexpr uint64_t SYS_dynlib_load_prx = 594;

struct SyscallArgs {
    uint64_t num{0};
    uint64_t arg1{0};
    uint64_t arg2{0};
    uint64_t arg3{0};
    uint64_t arg4{0};
    uint64_t arg5{0};
    uint64_t arg6{0};
};

using SyscallHandler = std::function<int64_t(const SyscallArgs& args, quin::memory::GuestAddressSpace& memory)>;

struct SyscallInfo {
    uint64_t num;
    std::string name;
    uint64_t call_count{0};
    bool implemented{false};
};

class SyscallDispatcher {
public:
    explicit SyscallDispatcher(quin::memory::GuestAddressSpace& memory);

    void register_syscall(uint64_t num, const std::string& name, SyscallHandler handler);
    int64_t dispatch(const SyscallArgs& args);

    std::vector<SyscallInfo> get_registered_syscalls() const;
    uint64_t get_total_syscall_calls() const { return m_total_calls; }

    quin::fs::VirtualFileSystem& get_vfs() { return m_vfs; }
    const quin::fs::VirtualFileSystem& get_vfs() const { return m_vfs; }

private:
    void register_defaults();

    quin::memory::GuestAddressSpace& m_memory;
    quin::fs::VirtualFileSystem m_vfs;
    std::unordered_map<uint64_t, SyscallHandler> m_handlers;
    std::unordered_map<uint64_t, SyscallInfo> m_syscall_info;
    uint64_t m_total_calls{0};
    mutable std::mutex m_mutex;
};

} // namespace quin::kernel

#endif // QUIN_KERNEL_SYSCALL_TABLE_HPP
