#include "cpu/execution_engine.hpp"
#include "core/logging.hpp"
#include <cstring>

namespace quin::cpu {

// RFLAGS bit positions
constexpr uint64_t RFLAG_CF = (1ULL << 0);
constexpr uint64_t RFLAG_ZF = (1ULL << 6);
constexpr uint64_t RFLAG_SF = (1ULL << 7);
constexpr uint64_t RFLAG_OF = (1ULL << 11);

ExecutionEngine::ExecutionEngine(quin::memory::GuestAddressSpace& memory, quin::kernel::LibKernel& kernel)
    : m_memory(memory), m_kernel(kernel), m_syscalls(memory), m_thread_manager(memory) {
    ExceptionHandler::initialize();
}

ExecutionEngine::~ExecutionEngine() {
    ExceptionHandler::shutdown();
}

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

GuestThreadId ExecutionEngine::spawn_thread(const std::string& name, uint64_t entry_point, uint64_t arg) {
    return m_thread_manager.create_thread(name, entry_point, arg);
}

uint64_t& ExecutionEngine::reg_by_index(CpuRegisters& regs, uint8_t idx) {
    switch (idx & 0x0F) {
        case 0:  return regs.rax;
        case 1:  return regs.rcx;
        case 2:  return regs.rdx;
        case 3:  return regs.rbx;
        case 4:  return regs.rsp;
        case 5:  return regs.rbp;
        case 6:  return regs.rsi;
        case 7:  return regs.rdi;
        case 8:  return regs.r8;
        case 9:  return regs.r9;
        case 10: return regs.r10;
        case 11: return regs.r11;
        case 12: return regs.r12;
        case 13: return regs.r13;
        case 14: return regs.r14;
        case 15: return regs.r15;
        default: return regs.rax;
    }
}

uint64_t ExecutionEngine::read_reg_by_index(const CpuRegisters& regs, uint8_t idx) const {
    switch (idx & 0x0F) {
        case 0:  return regs.rax;
        case 1:  return regs.rcx;
        case 2:  return regs.rdx;
        case 3:  return regs.rbx;
        case 4:  return regs.rsp;
        case 5:  return regs.rbp;
        case 6:  return regs.rsi;
        case 7:  return regs.rdi;
        case 8:  return regs.r8;
        case 9:  return regs.r9;
        case 10: return regs.r10;
        case 11: return regs.r11;
        case 12: return regs.r12;
        case 13: return regs.r13;
        case 14: return regs.r14;
        case 15: return regs.r15;
        default: return regs.rax;
    }
}

void ExecutionEngine::update_flags_add(CpuRegisters& regs, uint64_t a, uint64_t b, uint64_t result) {
    regs.rflags &= ~(RFLAG_CF | RFLAG_ZF | RFLAG_SF | RFLAG_OF);
    if (result == 0) regs.rflags |= RFLAG_ZF;
    if (result & (1ULL << 63)) regs.rflags |= RFLAG_SF;
    if (result < a) regs.rflags |= RFLAG_CF;
    if (((a ^ result) & (b ^ result)) & (1ULL << 63)) regs.rflags |= RFLAG_OF;
}

void ExecutionEngine::update_flags_sub(CpuRegisters& regs, uint64_t a, uint64_t b, uint64_t result) {
    regs.rflags &= ~(RFLAG_CF | RFLAG_ZF | RFLAG_SF | RFLAG_OF);
    if (result == 0) regs.rflags |= RFLAG_ZF;
    if (result & (1ULL << 63)) regs.rflags |= RFLAG_SF;
    if (a < b) regs.rflags |= RFLAG_CF;
    if (((a ^ b) & (a ^ result)) & (1ULL << 63)) regs.rflags |= RFLAG_OF;
}

void ExecutionEngine::update_flags_logic(CpuRegisters& regs, uint64_t result) {
    regs.rflags &= ~(RFLAG_CF | RFLAG_ZF | RFLAG_SF | RFLAG_OF);
    if (result == 0) regs.rflags |= RFLAG_ZF;
    if (result & (1ULL << 63)) regs.rflags |= RFLAG_SF;
    // CF and OF are cleared for logical ops
}

size_t ExecutionEngine::get_instruction_length(const uint8_t* code, size_t max_bytes) const {
    if (max_bytes == 0) return 0;
    size_t pos = 0;

    // Skip legacy prefixes (66, 67, F2, F3, 2E, 3E, 26, 36, 64, 65)
    while (pos < max_bytes) {
        uint8_t b = code[pos];
        if (b == 0x66 || b == 0x67 || b == 0xF2 || b == 0xF3 ||
            b == 0x2E || b == 0x3E || b == 0x26 || b == 0x36 ||
            b == 0x64 || b == 0x65) {
            pos++;
        } else {
            break;
        }
    }

    // REX prefix (0x40-0x4F)
    bool has_rex = false;
    if (pos < max_bytes && (code[pos] & 0xF0) == 0x40) {
        has_rex = true;
        pos++;
    }
    (void)has_rex;

    if (pos >= max_bytes) return pos;
    uint8_t opcode = code[pos++];

    // Two-byte opcode escape
    if (opcode == 0x0F) {
        if (pos >= max_bytes) return pos;
        uint8_t op2 = code[pos++];
        if (op2 == 0x05) return pos; // SYSCALL
        if (op2 == 0x07) return pos; // SYSRET
        // Conditional jumps (0x80-0x8F rel32)
        if (op2 >= 0x80 && op2 <= 0x8F) return pos + 4;
        // Other 2-byte ops with ModR/M
        if (pos < max_bytes) pos++; // ModR/M
        return pos;
    }

    switch (opcode) {
        case 0x90: return pos; // NOP
        case 0xC3: return pos; // RET
        case 0xCC: return pos; // INT3
        case 0xF4: return pos; // HLT
        case 0xCB: return pos; // RETF
        case 0xC9: return pos; // LEAVE
        case 0x98: case 0x99: return pos; // CWD/CDQ/CQO
        case 0xF8: case 0xF9: case 0xFC: case 0xFD: return pos; // CLC/STC/CLD/STD

        // PUSH/POP reg (0x50-0x5F)
        case 0x50: case 0x51: case 0x52: case 0x53:
        case 0x54: case 0x55: case 0x56: case 0x57:
        case 0x58: case 0x59: case 0x5A: case 0x5B:
        case 0x5C: case 0x5D: case 0x5E: case 0x5F:
            return pos;

        // MOV reg, imm64 (0xB8-0xBF with REX.W)
        case 0xB8: case 0xB9: case 0xBA: case 0xBB:
        case 0xBC: case 0xBD: case 0xBE: case 0xBF:
            return pos + 8; // REX.W + 64-bit immediate (worst case; 32-bit without REX.W)

        // MOV reg, imm8 (0xB0-0xB7)
        case 0xB0: case 0xB1: case 0xB2: case 0xB3:
        case 0xB4: case 0xB5: case 0xB6: case 0xB7:
            return pos + 1;

        // CALL rel32 / JMP rel32
        case 0xE8: case 0xE9: return pos + 4;

        // JMP rel8, Jcc rel8 (0x70-0x7F, 0xEB)
        case 0xEB:
        case 0x70: case 0x71: case 0x72: case 0x73:
        case 0x74: case 0x75: case 0x76: case 0x77:
        case 0x78: case 0x79: case 0x7A: case 0x7B:
        case 0x7C: case 0x7D: case 0x7E: case 0x7F:
            return pos + 1;

        // ADD/OR/ADC/SBB/AND/SUB/XOR/CMP al, imm8
        case 0x04: case 0x0C: case 0x14: case 0x1C:
        case 0x24: case 0x2C: case 0x34: case 0x3C:
            return pos + 1;

        // ADD/OR/ADC/SBB/AND/SUB/XOR/CMP rax, imm32
        case 0x05: case 0x0D: case 0x15: case 0x1D:
        case 0x25: case 0x2D: case 0x35: case 0x3D:
            return pos + 4;

        // Instructions with ModR/M byte
        case 0x01: case 0x03: case 0x09: case 0x0B: case 0x11: case 0x13:
        case 0x19: case 0x1B: case 0x21: case 0x23: case 0x29: case 0x2B:
        case 0x31: case 0x33: case 0x39: case 0x3B:
        case 0x85: case 0x87: case 0x89: case 0x8B: case 0x8D:
        case 0x63: // MOVSXD
        {
            if (pos >= max_bytes) return pos;
            uint8_t modrm = code[pos++];
            uint8_t mod = (modrm >> 6) & 3;
            uint8_t rm = modrm & 7;
            if (mod != 3) {
                if (rm == 4 && pos < max_bytes) pos++; // SIB byte
                if (mod == 1) pos += 1; // disp8
                else if (mod == 2) pos += 4; // disp32
                else if (mod == 0 && rm == 5) pos += 4; // RIP-relative
            }
            return pos;
        }

        // Group1 Ev, Ib (0x83)
        case 0x80: case 0x82:
        {
            if (pos >= max_bytes) return pos;
            uint8_t modrm = code[pos++];
            uint8_t mod = (modrm >> 6) & 3;
            uint8_t rm = modrm & 7;
            if (mod != 3) {
                if (rm == 4 && pos < max_bytes) pos++;
                if (mod == 1) pos += 1;
                else if (mod == 2) pos += 4;
                else if (mod == 0 && rm == 5) pos += 4;
            }
            pos += 1; // imm8
            return pos;
        }
        case 0x83:
        {
            if (pos >= max_bytes) return pos;
            uint8_t modrm = code[pos++];
            uint8_t mod = (modrm >> 6) & 3;
            uint8_t rm = modrm & 7;
            if (mod != 3) {
                if (rm == 4 && pos < max_bytes) pos++;
                if (mod == 1) pos += 1;
                else if (mod == 2) pos += 4;
                else if (mod == 0 && rm == 5) pos += 4;
            }
            pos += 1; // imm8
            return pos;
        }
        case 0x81:
        {
            if (pos >= max_bytes) return pos;
            uint8_t modrm = code[pos++];
            uint8_t mod = (modrm >> 6) & 3;
            uint8_t rm = modrm & 7;
            if (mod != 3) {
                if (rm == 4 && pos < max_bytes) pos++;
                if (mod == 1) pos += 1;
                else if (mod == 2) pos += 4;
                else if (mod == 0 && rm == 5) pos += 4;
            }
            pos += 4; // imm32
            return pos;
        }

        // MOV r/m, imm (0xC7)
        case 0xC6:
        {
            if (pos >= max_bytes) return pos;
            uint8_t modrm = code[pos++];
            uint8_t mod = (modrm >> 6) & 3;
            uint8_t rm = modrm & 7;
            if (mod != 3) {
                if (rm == 4 && pos < max_bytes) pos++;
                if (mod == 1) pos += 1;
                else if (mod == 2) pos += 4;
                else if (mod == 0 && rm == 5) pos += 4;
            }
            pos += 1; // imm8
            return pos;
        }
        case 0xC7:
        {
            if (pos >= max_bytes) return pos;
            uint8_t modrm = code[pos++];
            uint8_t mod = (modrm >> 6) & 3;
            uint8_t rm = modrm & 7;
            if (mod != 3) {
                if (rm == 4 && pos < max_bytes) pos++;
                if (mod == 1) pos += 1;
                else if (mod == 2) pos += 4;
                else if (mod == 0 && rm == 5) pos += 4;
            }
            pos += 4; // imm32
            return pos;
        }

        // TEST al, imm8 / TEST rax, imm32
        case 0xA8: return pos + 1;
        case 0xA9: return pos + 4;

        // FF group (INC/DEC/CALL/JMP indirect)
        case 0xFF:
        {
            if (pos >= max_bytes) return pos;
            uint8_t modrm = code[pos++];
            uint8_t mod = (modrm >> 6) & 3;
            uint8_t rm = modrm & 7;
            if (mod != 3) {
                if (rm == 4 && pos < max_bytes) pos++;
                if (mod == 1) pos += 1;
                else if (mod == 2) pos += 4;
                else if (mod == 0 && rm == 5) pos += 4;
            }
            return pos;
        }

        default:
            // Unknown opcode — advance by 1 byte with warning
            return pos;
    }
}

bool ExecutionEngine::step_with_regs(CpuRegisters& regs) {
    void* host_code_ptr = m_memory.get_host_pointer(regs.rip);
    if (!host_code_ptr) {
        QUIN_LOG_ERROR("ExecutionEngine: Execute fault — RIP 0x{:016X} not mapped", regs.rip);
        return false;
    }

    const auto* block = m_memory.get_block_at(regs.rip);
    if (block && (static_cast<uint32_t>(block->permissions) & static_cast<uint32_t>(quin::memory::PagePermission::Execute)) == 0) {
        QUIN_LOG_ERROR("ExecutionEngine: Execute fault — RIP 0x{:016X} lacks Execute permission", regs.rip);
        return false;
    }

    const uint8_t* code = static_cast<const uint8_t*>(host_code_ptr);
    size_t inst_len = get_instruction_length(code, 15);

    // Decode REX prefix if present
    size_t pos = 0;
    bool rex_w = false;
    uint8_t rex_r = 0, rex_b = 0;

    // Skip legacy prefixes
    while (pos < inst_len) {
        uint8_t b = code[pos];
        if (b == 0x66 || b == 0x67 || b == 0xF2 || b == 0xF3 ||
            b == 0x2E || b == 0x3E || b == 0x26 || b == 0x36 ||
            b == 0x64 || b == 0x65) {
            pos++;
        } else {
            break;
        }
    }

    if (pos < inst_len && (code[pos] & 0xF0) == 0x40) {
        uint8_t rex = code[pos];
        rex_w = (rex & 0x08) != 0;
        rex_r = (rex & 0x04) ? 8 : 0;
        rex_b = (rex & 0x01) ? 8 : 0;
        pos++;
    }

    if (pos >= inst_len) {
        regs.rip += inst_len;
        return true;
    }

    uint8_t opcode = code[pos];

    // ===== NOP (0x90) =====
    if (opcode == 0x90) {
        regs.rip += inst_len;
        return true;
    }

    // ===== RET (0xC3) =====
    if (opcode == 0xC3) {
        uint64_t ret_addr = 0;
        if (m_memory.read_bytes(regs.rsp, &ret_addr, 8)) {
            regs.rsp += 8;
            regs.rip = ret_addr;
        } else {
            // If we can't read the return address, treat as clean exit
            return false;
        }
        return true;
    }

    // ===== INT3 (0xCC) =====
    if (opcode == 0xCC) {
        QUIN_LOG_WARN("ExecutionEngine: INT3 breakpoint hit at RIP 0x{:016X}", regs.rip);
        regs.rip += inst_len;
        return true;
    }

    // ===== HLT (0xF4) =====
    if (opcode == 0xF4) {
        QUIN_LOG_INFO("ExecutionEngine: HLT instruction at RIP 0x{:016X} — halting", regs.rip);
        return false;
    }

    // ===== LEAVE (0xC9) =====
    if (opcode == 0xC9) {
        regs.rsp = regs.rbp;
        uint64_t old_rbp = 0;
        m_memory.read_bytes(regs.rsp, &old_rbp, 8);
        regs.rsp += 8;
        regs.rbp = old_rbp;
        regs.rip += inst_len;
        return true;
    }

    // ===== SYSCALL (0F 05) =====
    if (opcode == 0x0F && pos + 1 < inst_len && code[pos + 1] == 0x05) {
        QUIN_LOG_INFO("ExecutionEngine: SYSCALL at RIP 0x{:016X} (RAX: {}, RDI: 0x{:X})",
                      regs.rip, regs.rax, regs.rdi);

        quin::kernel::SyscallArgs args{
            regs.rax, regs.rdi, regs.rsi, regs.rdx,
            regs.r10, regs.r8, regs.r9
        };

        int64_t sys_ret = m_syscalls.dispatch(args);
        regs.rax = static_cast<uint64_t>(sys_ret);
        regs.rip += inst_len;
        return true;
    }

    // ===== PUSH reg (0x50-0x57) =====
    if (opcode >= 0x50 && opcode <= 0x57) {
        uint8_t reg_idx = (opcode - 0x50) | rex_b;
        uint64_t val = read_reg_by_index(regs, reg_idx);
        regs.rsp -= 8;
        m_memory.write_bytes(regs.rsp, &val, 8);
        regs.rip += inst_len;
        return true;
    }

    // ===== POP reg (0x58-0x5F) =====
    if (opcode >= 0x58 && opcode <= 0x5F) {
        uint8_t reg_idx = (opcode - 0x58) | rex_b;
        uint64_t val = 0;
        m_memory.read_bytes(regs.rsp, &val, 8);
        regs.rsp += 8;
        reg_by_index(regs, reg_idx) = val;
        regs.rip += inst_len;
        return true;
    }

    // ===== MOV reg, imm64/imm32 (0xB8-0xBF) =====
    if (opcode >= 0xB8 && opcode <= 0xBF) {
        uint8_t reg_idx = (opcode - 0xB8) | rex_b;
        if (rex_w) {
            uint64_t imm = 0;
            std::memcpy(&imm, &code[pos + 1], 8);
            reg_by_index(regs, reg_idx) = imm;
        } else {
            uint32_t imm = 0;
            std::memcpy(&imm, &code[pos + 1], 4);
            reg_by_index(regs, reg_idx) = imm; // zero-extends to 64-bit
        }
        regs.rip += inst_len;
        return true;
    }

    // ===== CALL rel32 (0xE8) =====
    if (opcode == 0xE8) {
        int32_t rel = 0;
        std::memcpy(&rel, &code[pos + 1], 4);
        uint64_t ret_addr = regs.rip + inst_len;
        regs.rsp -= 8;
        m_memory.write_bytes(regs.rsp, &ret_addr, 8);
        regs.rip = regs.rip + inst_len + rel;
        return true;
    }

    // ===== JMP rel32 (0xE9) =====
    if (opcode == 0xE9) {
        int32_t rel = 0;
        std::memcpy(&rel, &code[pos + 1], 4);
        regs.rip = regs.rip + inst_len + rel;
        return true;
    }

    // ===== JMP rel8 (0xEB) =====
    if (opcode == 0xEB) {
        int8_t rel = static_cast<int8_t>(code[pos + 1]);
        regs.rip = regs.rip + inst_len + rel;
        return true;
    }

    // ===== Jcc rel8 (0x70-0x7F) =====
    if (opcode >= 0x70 && opcode <= 0x7F) {
        int8_t rel = static_cast<int8_t>(code[pos + 1]);
        uint8_t cc = opcode & 0x0F;
        bool taken = false;
        bool zf = (regs.rflags & RFLAG_ZF) != 0;
        bool sf = (regs.rflags & RFLAG_SF) != 0;
        bool of = (regs.rflags & RFLAG_OF) != 0;
        bool cf = (regs.rflags & RFLAG_CF) != 0;

        switch (cc) {
            case 0x0: taken = of; break;            // JO
            case 0x1: taken = !of; break;           // JNO
            case 0x2: taken = cf; break;            // JB/JNAE/JC
            case 0x3: taken = !cf; break;           // JNB/JAE/JNC
            case 0x4: taken = zf; break;            // JZ/JE
            case 0x5: taken = !zf; break;           // JNZ/JNE
            case 0x6: taken = cf || zf; break;      // JBE/JNA
            case 0x7: taken = !cf && !zf; break;    // JNBE/JA
            case 0x8: taken = sf; break;            // JS
            case 0x9: taken = !sf; break;           // JNS
            case 0xC: taken = sf != of; break;      // JL/JNGE
            case 0xD: taken = sf == of; break;      // JNL/JGE
            case 0xE: taken = zf || (sf != of); break; // JLE/JNG
            case 0xF: taken = !zf && (sf == of); break; // JNLE/JG
            default: break;
        }

        if (taken) {
            regs.rip = regs.rip + inst_len + rel;
        } else {
            regs.rip += inst_len;
        }
        return true;
    }

    // ===== MOV r64, r/m64 (0x8B) and MOV r/m64, r64 (0x89) =====
    if (opcode == 0x89 || opcode == 0x8B) {
        if (pos + 1 >= inst_len) { regs.rip += inst_len; return true; }
        uint8_t modrm = code[pos + 1];
        uint8_t mod = (modrm >> 6) & 3;
        uint8_t reg = ((modrm >> 3) & 7) | rex_r;
        uint8_t rm = (modrm & 7) | rex_b;

        if (mod == 3) {
            // Register-to-register
            if (opcode == 0x89) {
                reg_by_index(regs, rm) = read_reg_by_index(regs, reg);
            } else {
                reg_by_index(regs, reg) = read_reg_by_index(regs, rm);
            }
        }
        // Memory forms would need full ModR/M+SIB decoding — register-to-register covers the common case
        regs.rip += inst_len;
        return true;
    }

    // ===== LEA r64, [m] (0x8D) =====
    if (opcode == 0x8D) {
        if (pos + 1 >= inst_len) { regs.rip += inst_len; return true; }
        uint8_t modrm = code[pos + 1];
        uint8_t mod = (modrm >> 6) & 3;
        uint8_t reg = ((modrm >> 3) & 7) | rex_r;
        uint8_t rm = (modrm & 7) | rex_b;

        if (mod == 0 && (rm & 7) == 5) {
            // RIP-relative: LEA reg, [rip+disp32]
            int32_t disp = 0;
            std::memcpy(&disp, &code[pos + 2], 4);
            reg_by_index(regs, reg) = regs.rip + inst_len + disp;
        }
        regs.rip += inst_len;
        return true;
    }

    // ===== XOR r64, r/m64 (0x31 reg-to-reg / 0x33 reg-from-rm) =====
    if (opcode == 0x31 || opcode == 0x33) {
        if (pos + 1 >= inst_len) { regs.rip += inst_len; return true; }
        uint8_t modrm = code[pos + 1];
        uint8_t mod = (modrm >> 6) & 3;
        uint8_t reg = ((modrm >> 3) & 7) | rex_r;
        uint8_t rm = (modrm & 7) | rex_b;

        if (mod == 3) {
            if (opcode == 0x31) {
                uint64_t result = read_reg_by_index(regs, rm) ^ read_reg_by_index(regs, reg);
                reg_by_index(regs, rm) = result;
                update_flags_logic(regs, result);
            } else {
                uint64_t result = read_reg_by_index(regs, reg) ^ read_reg_by_index(regs, rm);
                reg_by_index(regs, reg) = result;
                update_flags_logic(regs, result);
            }
        }
        regs.rip += inst_len;
        return true;
    }

    // ===== ADD r/m64, r64 (0x01) / ADD r64, r/m64 (0x03) =====
    if (opcode == 0x01 || opcode == 0x03) {
        if (pos + 1 >= inst_len) { regs.rip += inst_len; return true; }
        uint8_t modrm = code[pos + 1];
        uint8_t mod = (modrm >> 6) & 3;
        uint8_t reg = ((modrm >> 3) & 7) | rex_r;
        uint8_t rm = (modrm & 7) | rex_b;

        if (mod == 3) {
            if (opcode == 0x01) {
                uint64_t a = read_reg_by_index(regs, rm);
                uint64_t b = read_reg_by_index(regs, reg);
                uint64_t result = a + b;
                reg_by_index(regs, rm) = result;
                update_flags_add(regs, a, b, result);
            } else {
                uint64_t a = read_reg_by_index(regs, reg);
                uint64_t b = read_reg_by_index(regs, rm);
                uint64_t result = a + b;
                reg_by_index(regs, reg) = result;
                update_flags_add(regs, a, b, result);
            }
        }
        regs.rip += inst_len;
        return true;
    }

    // ===== SUB r/m64, r64 (0x29) / SUB r64, r/m64 (0x2B) =====
    if (opcode == 0x29 || opcode == 0x2B) {
        if (pos + 1 >= inst_len) { regs.rip += inst_len; return true; }
        uint8_t modrm = code[pos + 1];
        uint8_t mod = (modrm >> 6) & 3;
        uint8_t reg = ((modrm >> 3) & 7) | rex_r;
        uint8_t rm = (modrm & 7) | rex_b;

        if (mod == 3) {
            if (opcode == 0x29) {
                uint64_t a = read_reg_by_index(regs, rm);
                uint64_t b = read_reg_by_index(regs, reg);
                uint64_t result = a - b;
                reg_by_index(regs, rm) = result;
                update_flags_sub(regs, a, b, result);
            } else {
                uint64_t a = read_reg_by_index(regs, reg);
                uint64_t b = read_reg_by_index(regs, rm);
                uint64_t result = a - b;
                reg_by_index(regs, reg) = result;
                update_flags_sub(regs, a, b, result);
            }
        }
        regs.rip += inst_len;
        return true;
    }

    // ===== CMP r/m64, r64 (0x39) / CMP r64, r/m64 (0x3B) =====
    if (opcode == 0x39 || opcode == 0x3B) {
        if (pos + 1 >= inst_len) { regs.rip += inst_len; return true; }
        uint8_t modrm = code[pos + 1];
        uint8_t mod = (modrm >> 6) & 3;
        uint8_t reg = ((modrm >> 3) & 7) | rex_r;
        uint8_t rm = (modrm & 7) | rex_b;

        if (mod == 3) {
            uint64_t a, b;
            if (opcode == 0x39) {
                a = read_reg_by_index(regs, rm);
                b = read_reg_by_index(regs, reg);
            } else {
                a = read_reg_by_index(regs, reg);
                b = read_reg_by_index(regs, rm);
            }
            uint64_t result = a - b;
            update_flags_sub(regs, a, b, result);
        }
        regs.rip += inst_len;
        return true;
    }

    // ===== TEST r/m64, r64 (0x85) =====
    if (opcode == 0x85) {
        if (pos + 1 >= inst_len) { regs.rip += inst_len; return true; }
        uint8_t modrm = code[pos + 1];
        uint8_t mod = (modrm >> 6) & 3;
        uint8_t reg = ((modrm >> 3) & 7) | rex_r;
        uint8_t rm = (modrm & 7) | rex_b;

        if (mod == 3) {
            uint64_t result = read_reg_by_index(regs, rm) & read_reg_by_index(regs, reg);
            update_flags_logic(regs, result);
        }
        regs.rip += inst_len;
        return true;
    }

    // ===== SUB/ADD/CMP/XOR/AND/OR r/m, imm8 (0x83) =====
    if (opcode == 0x83) {
        if (pos + 1 >= inst_len) { regs.rip += inst_len; return true; }
        uint8_t modrm = code[pos + 1];
        uint8_t mod = (modrm >> 6) & 3;
        uint8_t op_ext = (modrm >> 3) & 7;
        uint8_t rm = (modrm & 7) | rex_b;

        if (mod == 3) {
            // Find the immediate byte (last byte of instruction)
            int8_t imm8 = static_cast<int8_t>(code[inst_len - 1]);
            int64_t imm = imm8; // sign-extend
            uint64_t a = read_reg_by_index(regs, rm);
            uint64_t b = static_cast<uint64_t>(imm);
            uint64_t result = 0;

            switch (op_ext) {
                case 0: // ADD
                    result = a + b;
                    reg_by_index(regs, rm) = result;
                    update_flags_add(regs, a, b, result);
                    break;
                case 1: // OR
                    result = a | b;
                    reg_by_index(regs, rm) = result;
                    update_flags_logic(regs, result);
                    break;
                case 4: // AND
                    result = a & b;
                    reg_by_index(regs, rm) = result;
                    update_flags_logic(regs, result);
                    break;
                case 5: // SUB
                    result = a - b;
                    reg_by_index(regs, rm) = result;
                    update_flags_sub(regs, a, b, result);
                    break;
                case 6: // XOR
                    result = a ^ b;
                    reg_by_index(regs, rm) = result;
                    update_flags_logic(regs, result);
                    break;
                case 7: // CMP
                    result = a - b;
                    update_flags_sub(regs, a, b, result);
                    break;
                default:
                    break;
            }
        }
        regs.rip += inst_len;
        return true;
    }

    // ===== Unrecognized opcode — advance by decoded instruction length =====
    QUIN_LOG_DEBUG("ExecutionEngine: Unhandled opcode 0x{:02X} at RIP 0x{:016X} (len={}), advancing",
                   opcode, regs.rip, inst_len);
    regs.rip += inst_len;
    return true;
}

void ExecutionEngine::step() {
    if (m_state != CpuState::Ready && m_state != CpuState::Running && m_state != CpuState::Paused) {
        return;
    }

    m_state = CpuState::Running;

    void* host_code_ptr = m_memory.get_host_pointer(m_regs.rip);
    if (!host_code_ptr) {
        trigger_trap("Execute fault: RIP 0x" + fmt::format("{:016X}", m_regs.rip) + " is not mapped in guest memory");
        return;
    }

    const auto* block = m_memory.get_block_at(m_regs.rip);
    if (block && (static_cast<uint32_t>(block->permissions) & static_cast<uint32_t>(quin::memory::PagePermission::Execute)) == 0) {
        trigger_trap("Execute fault: Memory at RIP 0x" + fmt::format("{:016X}", m_regs.rip) + " does not have Execute permission");
        return;
    }

    const uint8_t* code = static_cast<const uint8_t*>(host_code_ptr);

    // Check for clean exit: bare RET with nothing on stack
    if (code[0] == 0xC3 && m_regs.rsp >= 0x00007FFFF01FFE00ULL) {
        m_state = CpuState::Exited;
        QUIN_LOG_INFO("ExecutionEngine: Guest entry point executed RET. Clean exit achieved.");
        return;
    }

    if (!step_with_regs(m_regs)) {
        if (m_state == CpuState::Running) {
            m_state = CpuState::Exited;
            QUIN_LOG_INFO("ExecutionEngine: Guest execution halted at RIP 0x{:016X}.", m_regs.rip);
        }
        return;
    }

    m_executed_instructions_count++;

    if (m_executed_instructions_count % 1000 == 0) {
        QUIN_LOG_DEBUG("ExecutionEngine: Stepped {} instructions (Current RIP: 0x{:016X})",
                       m_executed_instructions_count, m_regs.rip);
    }
}

void ExecutionEngine::run() {
    if (m_state != CpuState::Ready && m_state != CpuState::Paused) return;

    QUIN_LOG_INFO("ExecutionEngine: Running guest code starting from RIP 0x{:016X}...", m_regs.rip);
    m_state = CpuState::Running;

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
