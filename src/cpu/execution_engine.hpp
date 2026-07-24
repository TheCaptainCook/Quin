#ifndef QUIN_CPU_EXECUTION_ENGINE_HPP
#define QUIN_CPU_EXECUTION_ENGINE_HPP

#include "memory/address_space.hpp"
#include "kernel/libkernel.hpp"
#include <cstdint>
#include <string>

namespace quin::cpu {

enum class CpuState {
    Uninitialized,
    Ready,
    Running,
    Paused,
    Trapped,
    Exited
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
    uint64_t rflags{0x202}; // Default IF set
};

class ExecutionEngine {
public:
    ExecutionEngine(quin::memory::GuestAddressSpace& memory, quin::kernel::LibKernel& kernel);

    bool bootstrap(uint64_t entry_point, uint64_t stack_top);
    void step();
    void run();
    void pause();
    void reset();

    CpuState get_state() const { return m_state; }
    const CpuRegisters& get_registers() const { return m_regs; }
    const std::string& get_last_trap_reason() const { return m_trap_reason; }

private:
    void trigger_trap(const std::string& reason);

    quin::memory::GuestAddressSpace& m_memory;
    quin::kernel::LibKernel& m_kernel;
    CpuRegisters m_regs{};
    CpuState m_state{CpuState::Uninitialized};
    std::string m_trap_reason;
    uint64_t m_executed_instructions_count{0};
};

} // namespace quin::cpu

#endif // QUIN_CPU_EXECUTION_ENGINE_HPP
