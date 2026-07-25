#include "kernel/syscall_table.hpp"
#include "core/logging.hpp"
#include <chrono>
#include <thread>

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
    // =====================================================================
    // SYS_exit (#1)
    // =====================================================================
    register_syscall(SYS_exit, "sys_exit", [](const SyscallArgs& args, quin::memory::GuestAddressSpace&) -> int64_t {
        QUIN_LOG_INFO("sys_exit called with status code: {}", args.arg1);
        return 0;
    });

    // =====================================================================
    // SYS_read (#3) - Integrated with VFS
    // =====================================================================
    register_syscall(SYS_read, "sys_read", [this](const SyscallArgs& args, quin::memory::GuestAddressSpace& mem) -> int64_t {
        quin::fs::VfsFileHandle handle = static_cast<quin::fs::VfsFileHandle>(args.arg1);
        std::vector<uint8_t> buffer(args.arg3, 0);
        int64_t bytes_read = m_vfs.read_file(handle, buffer.data(), args.arg3);
        if (bytes_read > 0) {
            mem.write_bytes(args.arg2, buffer.data(), bytes_read);
        }
        return bytes_read;
    });

    // =====================================================================
    // SYS_write (#4) - Integrated with VFS & Stdout
    // =====================================================================
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

    // =====================================================================
    // SYS_open (#5) - Integrated with VFS
    // =====================================================================
    register_syscall(SYS_open, "sys_open", [this](const SyscallArgs& args, quin::memory::GuestAddressSpace& mem) -> int64_t {
        std::vector<char> path_buf(512, 0);
        if (mem.read_bytes(args.arg1, path_buf.data(), 511)) {
            std::string guest_path(path_buf.data());
            quin::fs::VfsFileHandle handle = m_vfs.open_file(guest_path, static_cast<uint32_t>(args.arg2));
            return handle;
        }
        return -1;
    });

    // =====================================================================
    // SYS_close (#6) - Integrated with VFS
    // =====================================================================
    register_syscall(SYS_close, "sys_close", [this](const SyscallArgs& args, quin::memory::GuestAddressSpace&) -> int64_t {
        quin::fs::VfsFileHandle handle = static_cast<quin::fs::VfsFileHandle>(args.arg1);
        return m_vfs.close_file(handle) ? 0 : -1;
    });

    // =====================================================================
    // SYS_getpid (#20)
    // =====================================================================
    register_syscall(SYS_getpid, "sys_getpid", [](const SyscallArgs&, quin::memory::GuestAddressSpace&) -> int64_t {
        return 1001;
    });

    // =====================================================================
    // SYS_getuid (#24) / SYS_geteuid (#25) / SYS_getgid (#47) / SYS_getegid (#43)
    // =====================================================================
    register_syscall(SYS_getuid, "sys_getuid", [](const SyscallArgs&, quin::memory::GuestAddressSpace&) -> int64_t {
        return 1000; // Default PS5 user ID
    });
    register_syscall(SYS_geteuid, "sys_geteuid", [](const SyscallArgs&, quin::memory::GuestAddressSpace&) -> int64_t {
        return 1000;
    });
    register_syscall(SYS_getgid, "sys_getgid", [](const SyscallArgs&, quin::memory::GuestAddressSpace&) -> int64_t {
        return 1000;
    });
    register_syscall(SYS_getegid, "sys_getegid", [](const SyscallArgs&, quin::memory::GuestAddressSpace&) -> int64_t {
        return 1000;
    });

    // =====================================================================
    // SYS_ioctl (#54) — stub (return 0 = success)
    // =====================================================================
    register_syscall(SYS_ioctl, "sys_ioctl", [](const SyscallArgs& args, quin::memory::GuestAddressSpace&) -> int64_t {
        QUIN_LOG_DEBUG("sys_ioctl: fd={}, request=0x{:X}", args.arg1, args.arg2);
        return 0;
    });

    // =====================================================================
    // SYS_lseek (#199) — VFS seek
    // =====================================================================
    register_syscall(SYS_lseek, "sys_lseek", [this](const SyscallArgs& args, quin::memory::GuestAddressSpace&) -> int64_t {
        quin::fs::VfsFileHandle handle = static_cast<quin::fs::VfsFileHandle>(args.arg1);
        int64_t offset = static_cast<int64_t>(args.arg2);
        int whence = static_cast<int>(args.arg3);
        return m_vfs.seek_file(handle, offset, whence);
    });

    // =====================================================================
    // SYS_fstat (#189) — file stat via VFS
    // =====================================================================
    register_syscall(SYS_fstat, "sys_fstat", [this](const SyscallArgs& args, quin::memory::GuestAddressSpace& mem) -> int64_t {
        // Minimal stat implementation: write file size
        // FreeBSD stat struct is complex, but we write enough for basic usage
        QUIN_LOG_DEBUG("sys_fstat: fd={}, buf=0x{:X}", args.arg1, args.arg2);
        if (args.arg2 != 0) {
            // Zero out the stat buffer (minimum 120 bytes for FreeBSD stat)
            std::vector<uint8_t> stat_buf(120, 0);
            // st_size is at offset 72 in FreeBSD stat struct (uint64_t)
            // For now, write 0 for size
            mem.write_bytes(args.arg2, stat_buf.data(), stat_buf.size());
        }
        return 0;
    });

    // =====================================================================
    // SYS_stat (#188) — file stat by path
    // =====================================================================
    register_syscall(SYS_stat, "sys_stat", [this](const SyscallArgs& args, quin::memory::GuestAddressSpace& mem) -> int64_t {
        std::vector<char> path_buf(512, 0);
        if (mem.read_bytes(args.arg1, path_buf.data(), 511)) {
            std::string guest_path(path_buf.data());
            QUIN_LOG_DEBUG("sys_stat: path='{}'", guest_path);

            uint64_t file_size = 0;
            m_vfs.stat_file(guest_path, file_size);

            if (args.arg2 != 0) {
                std::vector<uint8_t> stat_buf(120, 0);
                // st_size at offset 72
                std::memcpy(&stat_buf[72], &file_size, sizeof(uint64_t));
                mem.write_bytes(args.arg2, stat_buf.data(), stat_buf.size());
            }
        }
        return 0;
    });

    // =====================================================================
    // SYS_munmap (#73)
    // =====================================================================
    register_syscall(SYS_munmap, "sys_munmap", [](const SyscallArgs& args, quin::memory::GuestAddressSpace& mem) -> int64_t {
        bool ok = mem.munmap(args.arg1, args.arg2);
        return ok ? 0 : -1;
    });

    // =====================================================================
    // SYS_mprotect (#74)
    // =====================================================================
    register_syscall(SYS_mprotect, "sys_mprotect", [](const SyscallArgs& args, quin::memory::GuestAddressSpace& mem) -> int64_t {
        auto perm = static_cast<quin::memory::PagePermission>(args.arg3 & 0x7);
        bool ok = mem.mprotect(args.arg1, args.arg2, perm);
        return ok ? 0 : -1;
    });

    // =====================================================================
    // SYS_nanosleep (#240)
    // =====================================================================
    register_syscall(SYS_nanosleep, "sys_nanosleep", [](const SyscallArgs& args, quin::memory::GuestAddressSpace& mem) -> int64_t {
        struct Timespec { uint64_t tv_sec; uint64_t tv_nsec; };
        Timespec ts{0, 0};
        if (args.arg1 != 0) {
            mem.read_bytes(args.arg1, &ts, sizeof(ts));
        }
        uint64_t total_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
        if (total_us > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(total_us));
        }
        return 0;
    });

    // =====================================================================
    // SYS_sigaction (#416) — stub (pretend success)
    // =====================================================================
    register_syscall(SYS_sigaction, "sys_sigaction", [](const SyscallArgs& args, quin::memory::GuestAddressSpace&) -> int64_t {
        QUIN_LOG_DEBUG("sys_sigaction: sig={}, act=0x{:X}, oact=0x{:X}", args.arg1, args.arg2, args.arg3);
        return 0;
    });

    // =====================================================================
    // SYS_sigprocmask (#340) — stub
    // =====================================================================
    register_syscall(SYS_sigprocmask, "sys_sigprocmask", [](const SyscallArgs& args, quin::memory::GuestAddressSpace&) -> int64_t {
        QUIN_LOG_DEBUG("sys_sigprocmask: how={}", args.arg1);
        return 0;
    });

    // =====================================================================
    // SYS_clock_gettime (#232)
    // =====================================================================
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

    // =====================================================================
    // SYS_gettimeofday (#116)
    // =====================================================================
    register_syscall(SYS_gettimeofday, "sys_gettimeofday", [](const SyscallArgs& args, quin::memory::GuestAddressSpace& mem) -> int64_t {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        uint64_t sec = std::chrono::duration_cast<std::chrono::seconds>(now).count();
        uint64_t usec = std::chrono::duration_cast<std::chrono::microseconds>(now).count() % 1000000;

        struct Timeval { uint64_t tv_sec; uint64_t tv_usec; } tv{sec, usec};
        if (args.arg1 != 0) {
            mem.write_bytes(args.arg1, &tv, sizeof(tv));
        }
        return 0;
    });

    // =====================================================================
    // SYS_writev (#121) — vectored write to stdout/stderr
    // =====================================================================
    register_syscall(SYS_writev, "sys_writev", [](const SyscallArgs& args, quin::memory::GuestAddressSpace& mem) -> int64_t {
        int64_t total_written = 0;
        uint64_t iov_addr = args.arg2;
        uint64_t iov_cnt = args.arg3;

        // struct iovec { void* iov_base; size_t iov_len; }
        for (uint64_t i = 0; i < iov_cnt && i < 64; ++i) {
            uint64_t base_ptr = 0;
            uint64_t len = 0;
            mem.read_bytes(iov_addr + i * 16, &base_ptr, 8);
            mem.read_bytes(iov_addr + i * 16 + 8, &len, 8);

            if (base_ptr != 0 && len > 0 && len < 65536) {
                std::vector<char> buf(len + 1, 0);
                mem.read_bytes(base_ptr, buf.data(), len);
                if (args.arg1 == 1 || args.arg1 == 2) {
                    QUIN_LOG_INFO("[Guest writev]: {}", buf.data());
                }
                total_written += static_cast<int64_t>(len);
            }
        }
        return total_written;
    });

    // =====================================================================
    // SYS_thr_self (#432)
    // =====================================================================
    register_syscall(SYS_thr_self, "sys_thr_self", [](const SyscallArgs& args, quin::memory::GuestAddressSpace& mem) -> int64_t {
        uint64_t tid = 1;
        if (args.arg1 != 0) {
            mem.write_bytes(args.arg1, &tid, sizeof(tid));
        }
        return 0;
    });

    // =====================================================================
    // SYS_thr_exit (#431)
    // =====================================================================
    register_syscall(SYS_thr_exit, "sys_thr_exit", [](const SyscallArgs& args, quin::memory::GuestAddressSpace&) -> int64_t {
        QUIN_LOG_INFO("sys_thr_exit: status=0x{:X}", args.arg1);
        return 0;
    });

    // =====================================================================
    // SYS_thr_new (#455) — stub (thread creation is handled by ThreadManager)
    // =====================================================================
    register_syscall(SYS_thr_new, "sys_thr_new", [](const SyscallArgs& args, quin::memory::GuestAddressSpace&) -> int64_t {
        QUIN_LOG_INFO("sys_thr_new: param=0x{:X}, param_size={}", args.arg1, args.arg2);
        return 0; // Thread creation happens via ThreadManager at a higher level
    });

    // =====================================================================
    // SYS_umtx_op (#454) — basic futex/mutex stub
    // =====================================================================
    register_syscall(SYS_umtx_op, "sys_umtx_op", [](const SyscallArgs& args, quin::memory::GuestAddressSpace&) -> int64_t {
        // op codes: UMTX_OP_WAIT=1, UMTX_OP_WAKE=2, etc.
        QUIN_LOG_DEBUG("sys_umtx_op: obj=0x{:X}, op={}, val={}", args.arg1, args.arg2, args.arg3);
        if (args.arg2 == 2) {
            // UMTX_OP_WAKE: signal waiting threads — stub returns success
            return 0;
        }
        if (args.arg2 == 1) {
            // UMTX_OP_WAIT: would block — stub returns immediately
            return 0;
        }
        return 0; // All ops succeed for now
    });

    // =====================================================================
    // SYS_mmap (#477)
    // =====================================================================
    register_syscall(SYS_mmap, "sys_mmap", [](const SyscallArgs& args, quin::memory::GuestAddressSpace& mem) -> int64_t {
        uint64_t addr = mem.mmap(args.arg1, args.arg2, quin::memory::PagePermission::ReadWrite);
        return static_cast<int64_t>(addr);
    });

    // =====================================================================
    // SYS_dynlib_dlsym (#591)
    // =====================================================================
    register_syscall(SYS_dynlib_dlsym, "sys_dynlib_dlsym", [](const SyscallArgs& args, quin::memory::GuestAddressSpace&) -> int64_t {
        QUIN_LOG_INFO("sys_dynlib_dlsym: handle=0x{:X}, sym_addr=0x{:X}", args.arg1, args.arg2);
        return 0; // Return null symbol address
    });

    // =====================================================================
    // SYS_dynlib_load_prx (#594)
    // =====================================================================
    register_syscall(SYS_dynlib_load_prx, "sys_dynlib_load_prx", [](const SyscallArgs& args, quin::memory::GuestAddressSpace&) -> int64_t {
        QUIN_LOG_INFO("sys_dynlib_load_prx called: PathVAddr=0x{:016X}", args.arg1);
        return 0x2001;
    });

    // =====================================================================
    // SYS_dynlib_get_proc_param (#599)
    // =====================================================================
    register_syscall(SYS_dynlib_get_proc_param, "sys_dynlib_get_proc_param", [](const SyscallArgs& args, quin::memory::GuestAddressSpace&) -> int64_t {
        QUIN_LOG_DEBUG("sys_dynlib_get_proc_param: 0x{:X}", args.arg1);
        return 0;
    });
}

} // namespace quin::kernel
