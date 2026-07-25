#ifndef QUIN_CPU_EXECUTION_ENGINE_HPP
#define QUIN_CPU_EXECUTION_ENGINE_HPP

#include "memory/address_space.hpp"
#include "kernel/libkernel.hpp"
#include "kernel/syscall_table.hpp"
#include "cpu/thread_manager.hpp"
#include "cpu/exception_handler.hpp"
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

class ExecutionEngine {
public:
    ExecutionEngine(quin::memory::GuestAddressSpace& memory, quin::kernel::LibKernel& kernel);
    ~ExecutionEngine();

    bool bootstrap(uint64_t entry_point, uint64_t stack_top);
    void step();
    void run();
    void pause();
    void reset();

    GuestThreadId spawn_thread(const std::string& name, uint64_t entry_point, uint64_t arg);

    CpuState get_state() const { return m_state; }
    const CpuRegisters& get_registers() const { return m_regs; }
    const std::string& get_last_trap_reason() const { return m_trap_reason; }

    ThreadManager& get_thread_manager() { return m_thread_manager; }
    const ThreadManager& get_thread_manager() const { return m_thread_manager; }

    quin::kernel::SyscallDispatcher& get_syscall_dispatcher() { return m_syscalls; }
    const quin::kernel::SyscallDispatcher& get_syscall_dispatcher() const { return m_syscalls; }

    // Execute a single instruction on a given register set (used by thread manager)
    bool step_with_regs(CpuRegisters& regs);

private:
    void trigger_trap(const std::string& reason);

    // x86-64 instruction decoder helpers
    size_t get_instruction_length(const uint8_t* code, size_t max_bytes) const;
    uint64_t& reg_by_index(CpuRegisters& regs, uint8_t idx);
    uint64_t read_reg_by_index(const CpuRegisters& regs, uint8_t idx) const;
    void update_flags_add(CpuRegisters& regs, uint64_t a, uint64_t b, uint64_t result);
    void update_flags_sub(CpuRegisters& regs, uint64_t a, uint64_t b, uint64_t result);
    void update_flags_logic(CpuRegisters& regs, uint64_t result);

    quin::memory::GuestAddressSpace& m_memory;
    quin::kernel::LibKernel& m_kernel;
    quin::kernel::SyscallDispatcher m_syscalls;
    ThreadManager m_thread_manager;

    CpuRegisters m_regs{};
    CpuState m_state{CpuState::Uninitialized};
    std::string m_trap_reason;
    uint64_t m_executed_instructions_count{0};
};

} // namespace quin::cpu

#endif // QUIN_CPU_EXECUTION_ENGINE_HPP
