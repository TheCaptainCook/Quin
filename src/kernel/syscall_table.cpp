#include "kernel/syscall_table.hpp"
#include "core/logging.hpp"
#include <chrono>

namespace quin::kernel {

SyscallDispatcher::SyscallDispatcher(quin::memory::GuestAddressSpace& memory)
    : m_memory(memory) {
    register_defaults();
}

void SyscallDispatcher::register_syscall(uint64_t num, const std::string& name, SyscallHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers[num] = handler;
    m_syscall_info[num] = SyscallInfo{num, name, 0, true};
    QUIN_LOG_DEBUG("Syscall Registered: #{} ('{}')", num, name);
}

int64_t SyscallDispatcher::dispatch(const SyscallArgs& args) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_total_calls++;

    auto info_it = m_syscall_info.find(args.num);
    if (info_it != m_syscall_info.end()) {
        info_it->second.call_count++;
    }

    auto handler_it = m_handlers.find(args.num);
    if (handler_it != m_handlers.end() && handler_it->second) {
        QUIN_LOG_INFO("Syscall Dispatched: #{} ('{}') | Args: [0x{:X}, 0x{:X}, 0x{:X}]",
                      args.num, info_it != m_syscall_info.end() ? info_it->second.name : "unknown",
                      args.arg1, args.arg2, args.arg3);
        return handler_it->second(args, m_memory);
    }

    QUIN_LOG_WARN("UNIMPLEMENTED SYSCALL HIT: #{} | Args: [0x{:X}, 0x{:X}, 0x{:X}, 0x{:X}]",
                  args.num, args.arg1, args.arg2, args.arg3, args.arg4);
    return -1; // ENOSYS
}

std::vector<SyscallInfo> SyscallDispatcher::get_registered_syscalls() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<SyscallInfo> result;
    for (const auto& [num, info] : m_syscall_info) {
        result.push_back(info);
    }
    return result;
}

void SyscallDispatcher::register_defaults() {
    // SYS_exit (#1)
    register_syscall(SYS_exit, "sys_exit", [](const SyscallArgs& args, quin::memory::GuestAddressSpace&) -> int64_t {
        QUIN_LOG_INFO("sys_exit called with status code: {}", args.arg1);
        return 0;
    });

    // SYS_read (#3)
    register_syscall(SYS_read, "sys_read", [](const SyscallArgs& args, quin::memory::GuestAddressSpace&) -> int64_t {
        QUIN_LOG_INFO("sys_read called: FD={}, Buf=0x{:016X}, Count={}", args.arg1, args.arg2, args.arg3);
        return 0;
    });

    // SYS_write (#4)
    register_syscall(SYS_write, "sys_write", [](const SyscallArgs& args, quin::memory::GuestAddressSpace& mem) -> int64_t {
        if (args.arg1 == 1 || args.arg1 == 2) { // stdout or stderr
            std::vector<char> buf(args.arg3 + 1, 0);
            if (mem.read_bytes(args.arg2, buf.data(), args.arg3)) {
                QUIN_LOG_INFO("[Guest Stdout/Stderr]: {}", buf.data());
            }
        }
        return static_cast<int64_t>(args.arg3);
    });

    // SYS_open (#5)
    register_syscall(SYS_open, "sys_open", [](const SyscallArgs& args, quin::memory::GuestAddressSpace&) -> int64_t {
        QUIN_LOG_INFO("sys_open called: PathVAddr=0x{:016X}, Flags=0x{:X}", args.arg1, args.arg2);
        return 3; // Synthetic file descriptor
    });

    // SYS_close (#6)
    register_syscall(SYS_close, "sys_close", [](const SyscallArgs& args, quin::memory::GuestAddressSpace&) -> int64_t {
        QUIN_LOG_INFO("sys_close called: FD={}", args.arg1);
        return 0;
    });

    // SYS_getpid (#20)
    register_syscall(SYS_getpid, "sys_getpid", [](const SyscallArgs&, quin::memory::GuestAddressSpace&) -> int64_t {
        return 1001; // Synthetic PS5 PID
    });

    // SYS_clock_gettime (#232)
    register_syscall(SYS_clock_gettime, "sys_clock_gettime", [](const SyscallArgs& args, quin::memory::GuestAddressSpace& mem) -> int64_t {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        uint64_t sec = std::chrono::duration_cast<std::chrono::seconds>(now).count();
        uint64_t nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count() % 1000000000;

        struct Timespec { uint64_t tv_sec; uint64_t tv_nsec; } ts{sec, nsec};
        if (args.arg2 != 0) {
            mem.write_bytes(args.arg2, &ts, sizeof(ts));
        }
        return 0;
    });

    // SYS_thr_self (#432)
    register_syscall(SYS_thr_self, "sys_thr_self", [](const SyscallArgs& args, quin::memory::GuestAddressSpace& mem) -> int64_t {
        uint64_t tid = 1;
        if (args.arg1 != 0) {
            mem.write_bytes(args.arg1, &tid, sizeof(tid));
        }
        return 0;
    });

    // SYS_mmap (#477)
    register_syscall(SYS_mmap, "sys_mmap", [](const SyscallArgs& args, quin::memory::GuestAddressSpace& mem) -> int64_t {
        uint64_t addr = mem.mmap(args.arg1, args.arg2, quin::memory::PagePermission::ReadWrite);
        return static_cast<int64_t>(addr);
    });

    // SYS_dynlib_load_prx (#594)
    register_syscall(SYS_dynlib_load_prx, "sys_dynlib_load_prx", [](const SyscallArgs& args, quin::memory::GuestAddressSpace&) -> int64_t {
        QUIN_LOG_INFO("sys_dynlib_load_prx called: PathVAddr=0x{:016X}", args.arg1);
        return 0x2001; // Synthetic PRX module handle
    });
}

} // namespace quin::kernel
