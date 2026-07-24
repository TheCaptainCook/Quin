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

    // SYS_read (#3) - Integrated with VFS
    register_syscall(SYS_read, "sys_read", [this](const SyscallArgs& args, quin::memory::GuestAddressSpace& mem) -> int64_t {
        quin::fs::VfsFileHandle handle = static_cast<quin::fs::VfsFileHandle>(args.arg1);
        std::vector<uint8_t> buffer(args.arg3, 0);
        int64_t bytes_read = m_vfs.read_file(handle, buffer.data(), args.arg3);
        if (bytes_read > 0) {
            mem.write_bytes(args.arg2, buffer.data(), bytes_read);
        }
        return bytes_read;
    });

    // SYS_write (#4) - Integrated with VFS & Stdout
    register_syscall(SYS_write, "sys_write", [this](const SyscallArgs& args, quin::memory::GuestAddressSpace& mem) -> int64_t {
        if (args.arg1 == 1 || args.arg1 == 2) { // stdout or stderr
            std::vector<char> buf(args.arg3 + 1, 0);
            if (mem.read_bytes(args.arg2, buf.data(), args.arg3)) {
                QUIN_LOG_INFO("[Guest Stdout/Stderr]: {}", buf.data());
            }
            return static_cast<int64_t>(args.arg3);
        }
        quin::fs::VfsFileHandle handle = static_cast<quin::fs::VfsFileHandle>(args.arg1);
        std::vector<uint8_t> buffer(args.arg3, 0);
        if (mem.read_bytes(args.arg2, buffer.data(), args.arg3)) {
            return m_vfs.write_file(handle, buffer.data(), args.arg3);
        }
        return -1;
    });

    // SYS_open (#5) - Integrated with VFS
    register_syscall(SYS_open, "sys_open", [this](const SyscallArgs& args, quin::memory::GuestAddressSpace& mem) -> int64_t {
        std::vector<char> path_buf(512, 0);
        if (mem.read_bytes(args.arg1, path_buf.data(), 511)) {
            std::string guest_path(path_buf.data());
            quin::fs::VfsFileHandle handle = m_vfs.open_file(guest_path, static_cast<uint32_t>(args.arg2));
            return handle;
        }
        return -1;
    });

    // SYS_close (#6) - Integrated with VFS
    register_syscall(SYS_close, "sys_close", [this](const SyscallArgs& args, quin::memory::GuestAddressSpace&) -> int64_t {
        quin::fs::VfsFileHandle handle = static_cast<quin::fs::VfsFileHandle>(args.arg1);
        return m_vfs.close_file(handle) ? 0 : -1;
    });

    // SYS_getpid (#20)
    register_syscall(SYS_getpid, "sys_getpid", [](const SyscallArgs&, quin::memory::GuestAddressSpace&) -> int64_t {
        return 1001;
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
        return 0x2001;
    });
}

} // namespace quin::kernel
