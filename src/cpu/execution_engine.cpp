#include "cpu/execution_engine.hpp"
#include "core/logging.hpp"

namespace quin::cpu {

ExecutionEngine::ExecutionEngine(quin::memory::GuestAddressSpace& memory, quin::kernel::LibKernel& kernel)
    : m_memory(memory), m_kernel(kernel) {}

bool ExecutionEngine::bootstrap(uint64_t entry_point, uint64_t stack_top) {
    if (entry_point == 0) {
        QUIN_LOG_ERROR("ExecutionEngine: Cannot bootstrap with null entry point.");
        return false;
    }

    m_regs = CpuRegisters{};
    m_regs.rip = entry_point;
    m_regs.rsp = stack_top;
    m_regs.rbp = stack_top;

    m_state = CpuState::Ready;
    m_executed_instructions_count = 0;
    m_trap_reason.clear();

    QUIN_LOG_INFO("ExecutionEngine: Bootstrapped guest CPU harness — RIP: 0x{:016X} | RSP: 0x{:016X}",
                  m_regs.rip, m_regs.rsp);
    return true;
}

void ExecutionEngine::step() {
    if (m_state != CpuState::Ready && m_state != CpuState::Running && m_state != CpuState::Paused) {
        return;
    }

    m_state = CpuState::Running;

    // Check if RIP is valid in guest address space
    void* host_code_ptr = m_memory.get_host_pointer(m_regs.rip);
    if (!host_code_ptr) {
        trigger_trap("Execute fault: RIP 0x" + fmt::format("{:016X}", m_regs.rip) + " is not mapped in guest memory");
        return;
    }

    uint8_t opcode = *static_cast<const uint8_t*>(host_code_ptr);
    m_executed_instructions_count++;

    // Soft emulation / decoding harness for basic control flow & system traps
    if (opcode == 0x90) { // NOP
        m_regs.rip += 1;
    } else if (opcode == 0xC3) { // RET
        m_state = CpuState::Exited;
        QUIN_LOG_INFO("ExecutionEngine: Guest entry point executed RET. Clean exit achieved.");
    } else if (opcode == 0x0F && static_cast<const uint8_t*>(host_code_ptr)[1] == 0x05) { // SYSCALL
        QUIN_LOG_INFO("ExecutionEngine: SYSCALL instruction intercepted at RIP 0x{:016X} (RAX: {})",
                      m_regs.rip, m_regs.rax);
        m_regs.rip += 2;
    } else {
        // Safe instruction stepping harness: advance RIP and record execution
        m_regs.rip += 1;
    }

    if (m_executed_instructions_count % 1000 == 0) {
        QUIN_LOG_DEBUG("ExecutionEngine: Stepped {} instructions (Current RIP: 0x{:016X})",
                       m_executed_instructions_count, m_regs.rip);
    }
}

void ExecutionEngine::run() {
    if (m_state != CpuState::Ready && m_state != CpuState::Paused) return;

    QUIN_LOG_INFO("ExecutionEngine: Running guest code starting from RIP 0x{:016X}...", m_regs.rip);
    m_state = CpuState::Running;

    // Run steps until completion, trap, or pause
    size_t step_count = 0;
    while (m_state == CpuState::Running && step_count < 10000) {
        step();
        step_count++;
    }

    if (m_state == CpuState::Running) {
        m_state = CpuState::Ready;
        QUIN_LOG_INFO("ExecutionEngine: Execution quantum reached (10,000 steps). Harness paused cleanly.");
    }
}

void ExecutionEngine::pause() {
    if (m_state == CpuState::Running) {
        m_state = CpuState::Paused;
        QUIN_LOG_INFO("ExecutionEngine: Harness paused by user.");
    }
}

void ExecutionEngine::reset() {
    m_state = CpuState::Uninitialized;
    m_regs = CpuRegisters{};
    m_executed_instructions_count = 0;
    m_trap_reason.clear();
    QUIN_LOG_INFO("ExecutionEngine: Harness reset.");
}

void ExecutionEngine::trigger_trap(const std::string& reason) {
    m_state = CpuState::Trapped;
    m_trap_reason = reason;
    QUIN_LOG_ERROR("ExecutionEngine: TRAP TRIGGERED — {}", reason);
    QUIN_LOG_ERROR("CPU State: RIP=0x{:016X} RSP=0x{:016X} RAX=0x{:016X} RBX=0x{:016X}",
                   m_regs.rip, m_regs.rsp, m_regs.rax, m_regs.rbx);
}

} // namespace quin::cpu
