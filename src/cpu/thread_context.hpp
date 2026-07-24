#ifndef QUIN_CPU_THREAD_CONTEXT_HPP
#define QUIN_CPU_THREAD_CONTEXT_HPP

#include <cstdint>
#include <string>
#include <thread>
#include <atomic>

namespace quin::cpu {

using GuestThreadId = uint32_t;

enum class ThreadState {
    Uninitialized,
    Ready,
    Running,
    Waiting,
    Terminated
};

struct CpuRegisters {
    uint64_t rip{0};
    uint64_t rsp{0};
    uint64_t rbp{0};
    uint64_t rax{0};
    uint64_t rbx{0};
    uint64_t rcx{0};
    uint64_t rdx{0};
    uint64_t rsi{0};
    uint64_t rdi{0};
    uint64_t r8{0};
    uint64_t r9{0};
    uint64_t r10{0};
    uint64_t r11{0};
    uint64_t r12{0};
    uint64_t r13{0};
    uint64_t r14{0};
    uint64_t r15{0};
    uint64_t rflags{0x202};
};

struct ThreadContext {
    GuestThreadId id{0};
    std::string name;
    std::atomic<ThreadState> state{ThreadState::Uninitialized};

    CpuRegisters regs{};
    uint64_t stack_base{0};
    uint64_t stack_size{0};
    uint64_t guard_page_addr{0};
    uint64_t tls_base{0};

    uint64_t entry_point{0};
    uint64_t arg{0};

    std::thread host_thread;
};

} // namespace quin::cpu

#endif // QUIN_CPU_THREAD_CONTEXT_HPP
