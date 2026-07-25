#include "gpu/shader/shader_recompiler.hpp"
#include "core/logging.hpp"
#include <cstring>
#include <functional>

namespace quin::gpu::shader {

ShaderHash ShaderRecompiler::compute_hash(const void* bytes, size_t size) {
    if (!bytes || size == 0) return 0;
    std::string_view view(static_cast<const char*>(bytes), size);
    return static_cast<ShaderHash>(std::hash<std::string_view>{}(view));
}

std::vector<uint32_t> ShaderRecompiler::emit_spirv_header(ShaderType stage) {
    std::vector<uint32_t> spirv;
    spirv.reserve(64);

    // SPIR-V Header
    spirv.push_back(SPIRV_MAGIC_NUMBER); // Magic Number
    spirv.push_back(SPIRV_VERSION);      // Version 1.5
    spirv.push_back(0x00000000);         // Generator Magic
    spirv.push_back(64);                 // Bound ID (increased for real instructions)
    spirv.push_back(0);                  // Schema

    // OpCapability Shader (Word count 2, Opcode 17)
    spirv.push_back((2 << 16) | 17);
    spirv.push_back(1); // Shader capability

    // OpMemoryModel Logical GLSL450 (Word count 3, Opcode 14)
    spirv.push_back((3 << 16) | 14);
    spirv.push_back(0); // Logical
    spirv.push_back(1); // GLSL450

    // OpEntryPoint (Word count N, Opcode 15)
    uint32_t execution_model = (stage == ShaderType::Vertex) ? 0 : 4; // 0=Vertex, 4=Fragment
    spirv.push_back((5 << 16) | 15);
    spirv.push_back(execution_model);
    spirv.push_back(1); // Entry point ID
    spirv.push_back(0x69616D00); // "main" ASCII null terminated
    spirv.push_back(0);

    // OpExecutionMode (if Pixel/Fragment shader)
    if (stage == ShaderType::Pixel) {
        spirv.push_back((3 << 16) | 16);
        spirv.push_back(1); // Function ID
        spirv.push_back(7); // OriginUpperLeft
    }

    // ---- Type declarations for shader body ----
    // OpTypeVoid (ID=2, Opcode 19)
    spirv.push_back((2 << 16) | 19);
    spirv.push_back(2); // %void = OpTypeVoid

    // OpTypeFunction %void (ID=3, Opcode 33)
    spirv.push_back((3 << 16) | 33);
    spirv.push_back(3); // %func_void = OpTypeFunction %void
    spirv.push_back(2); // return type = %void

    // OpTypeFloat 32 (ID=4, Opcode 22)
    spirv.push_back((3 << 16) | 22);
    spirv.push_back(4); // %float = OpTypeFloat 32
    spirv.push_back(32);

    // OpTypeVector %float 4 (ID=5, Opcode 23)
    spirv.push_back((4 << 16) | 23);
    spirv.push_back(5); // %v4float = OpTypeVector %float 4
    spirv.push_back(4);
    spirv.push_back(4);

    // OpFunction (ID=1, Opcode 54)
    spirv.push_back((5 << 16) | 54);
    spirv.push_back(2); // return type %void
    spirv.push_back(1); // function ID (entry point)
    spirv.push_back(0); // function control = None
    spirv.push_back(3); // function type = %func_void

    // OpLabel (ID=6, Opcode 248)
    spirv.push_back((2 << 16) | 248);
    spirv.push_back(6); // %entry

    return spirv;
}

// =====================================================================
// RDNA2 Instruction Decoder (Clean-Room from AMD RDNA2 ISA Manual)
//
// RDNA2 instructions are 32-bit or 64-bit words.
// The encoding is identified by the top bits of the first dword:
//   [31:23] = 0b101111110 → SOPP  (scalar program flow)
//   [31:23] = 0b101111111 → SOPC  (scalar comparison)
//   [31:23] = 0b101111101 → SOP1  (scalar one-operand)
//   [31:23] = 0b10xxxxxxx → SOP2  (scalar two-operand, most patterns)
//   [31:25] = 0b0111111   → VOP1  (vector one-operand)
//   [31]    = 0b0          → VOP2  (vector two-operand, top bit 0)
//   [31:26] = 0b110100    → VOP3  (vector three-operand)
//   [31:26] = 0b111110    → EXP   (export)
//   [31:26] = 0b110000    → SMEM  (scalar memory)
// =====================================================================
Rdna2DecodedInsn ShaderRecompiler::decode_rdna2_instruction(const uint32_t* words, size_t remaining_dwords) {
    Rdna2DecodedInsn insn{};
    if (remaining_dwords == 0) return insn;

    insn.raw_word0 = words[0];

    uint32_t w = words[0];
    uint32_t top9 = (w >> 23) & 0x1FF;
    uint32_t top7 = (w >> 25) & 0x7F;
    uint32_t top6 = (w >> 26) & 0x3F;

    // ===== SOPP (0b101111110) =====
    if (top9 == 0b101111110) {
        insn.category = Rdna2InsnCategory::SOPP;
        insn.opcode = (w >> 16) & 0x7F;
        insn.is_64bit = false;

        switch (insn.opcode) {
            case 0:  insn.mnemonic = "s_nop"; break;
            case 1:  insn.mnemonic = "s_endpgm"; break;
            case 2:  insn.mnemonic = "s_branch"; break;
            case 4:  insn.mnemonic = "s_cbranch_scc0"; break;
            case 5:  insn.mnemonic = "s_cbranch_scc1"; break;
            case 10: insn.mnemonic = "s_cbranch_execz"; break;
            case 11: insn.mnemonic = "s_cbranch_execnz"; break;
            case 12: insn.mnemonic = "s_barrier"; break;
            case 28: insn.mnemonic = "s_waitcnt"; break;
            default: insn.mnemonic = "s_sopp_" + std::to_string(insn.opcode); break;
        }
        return insn;
    }

    // ===== SOP1 (0b101111101) =====
    if (top9 == 0b101111101) {
        insn.category = Rdna2InsnCategory::SOP1;
        insn.opcode = (w >> 8) & 0xFF;
        insn.dst = (w >> 16) & 0x7F;
        insn.src0 = w & 0xFF;
        insn.is_64bit = false;

        switch (insn.opcode) {
            case 3:  insn.mnemonic = "s_mov_b32"; break;
            case 4:  insn.mnemonic = "s_mov_b64"; break;
            case 33: insn.mnemonic = "s_getpc_b64"; break;
            case 36: insn.mnemonic = "s_swappc_b64"; break;
            default: insn.mnemonic = "s_sop1_" + std::to_string(insn.opcode); break;
        }
        return insn;
    }

    // ===== SOPC (0b101111111) =====
    if (top9 == 0b101111111) {
        insn.category = Rdna2InsnCategory::SOPC;
        insn.opcode = (w >> 16) & 0x7F;
        insn.src0 = w & 0xFF;
        insn.src1 = (w >> 8) & 0xFF;
        insn.is_64bit = false;
        insn.mnemonic = "s_cmp_" + std::to_string(insn.opcode);
        return insn;
    }

    // ===== SOP2 (top 2 bits = 10, but not SOPP/SOP1/SOPC) =====
    if ((w >> 30) == 0b10 && top9 != 0b101111110 && top9 != 0b101111101 && top9 != 0b101111111) {
        insn.category = Rdna2InsnCategory::SOP2;
        insn.opcode = (w >> 23) & 0x7F;
        insn.dst = (w >> 16) & 0x7F;
        insn.src0 = w & 0xFF;
        insn.src1 = (w >> 8) & 0xFF;
        insn.is_64bit = false;

        switch (insn.opcode) {
            case 0:  insn.mnemonic = "s_add_u32"; break;
            case 1:  insn.mnemonic = "s_sub_u32"; break;
            case 2:  insn.mnemonic = "s_add_i32"; break;
            case 3:  insn.mnemonic = "s_sub_i32"; break;
            case 8:  insn.mnemonic = "s_and_b32"; break;
            case 10: insn.mnemonic = "s_or_b32"; break;
            case 12: insn.mnemonic = "s_xor_b32"; break;
            case 14: insn.mnemonic = "s_andn2_b32"; break;
            case 20: insn.mnemonic = "s_lshl_b32"; break;
            case 22: insn.mnemonic = "s_lshr_b32"; break;
            default: insn.mnemonic = "s_sop2_" + std::to_string(insn.opcode); break;
        }
        return insn;
    }

    // ===== EXP (0b111110xx) =====
    if (top6 == 0b111110) {
        insn.category = Rdna2InsnCategory::EXP;
        insn.is_64bit = (remaining_dwords >= 2);
        if (insn.is_64bit) insn.raw_word1 = words[1];
        insn.opcode = (w >> 4) & 0x3F; // target
        insn.mnemonic = "exp";
        return insn;
    }

    // ===== SMEM (0b110000xx) =====
    if (top6 == 0b110000) {
        insn.category = Rdna2InsnCategory::SMEM;
        insn.opcode = (w >> 18) & 0x3F;
        insn.is_64bit = (remaining_dwords >= 2);
        if (insn.is_64bit) insn.raw_word1 = words[1];

        switch (insn.opcode) {
            case 0:  insn.mnemonic = "s_load_dword"; break;
            case 1:  insn.mnemonic = "s_load_dwordx2"; break;
            case 2:  insn.mnemonic = "s_load_dwordx4"; break;
            case 3:  insn.mnemonic = "s_load_dwordx8"; break;
            case 8:  insn.mnemonic = "s_buffer_load_dword"; break;
            default: insn.mnemonic = "s_smem_" + std::to_string(insn.opcode); break;
        }
        return insn;
    }

    // ===== VOP1 (top 7 bits = 0b0111111) =====
    if (top7 == 0b0111111) {
        insn.category = Rdna2InsnCategory::VOP1;
        insn.opcode = (w >> 9) & 0xFF;
        insn.dst = (w >> 17) & 0xFF;
        insn.src0 = w & 0x1FF;
        insn.is_64bit = false;

        switch (insn.opcode) {
            case 1:  insn.mnemonic = "v_mov_b32"; break;
            case 5:  insn.mnemonic = "v_cvt_f32_i32"; break;
            case 6:  insn.mnemonic = "v_cvt_f32_u32"; break;
            case 11: insn.mnemonic = "v_cvt_i32_f32"; break;
            case 33: insn.mnemonic = "v_rcp_f32"; break;
            case 34: insn.mnemonic = "v_rcp_iflag_f32"; break;
            case 35: insn.mnemonic = "v_rsq_f32"; break;
            case 39: insn.mnemonic = "v_sqrt_f32"; break;
            default: insn.mnemonic = "v_vop1_" + std::to_string(insn.opcode); break;
        }
        return insn;
    }

    // ===== VOP3 (0b110100xx) =====
    if (top6 == 0b110100) {
        insn.category = Rdna2InsnCategory::VOP3;
        insn.opcode = (w >> 16) & 0x3FF;
        insn.dst = w & 0xFF;
        insn.is_64bit = (remaining_dwords >= 2);
        if (insn.is_64bit) {
            insn.raw_word1 = words[1];
            insn.src0 = words[1] & 0x1FF;
            insn.src1 = (words[1] >> 9) & 0x1FF;
            insn.src2 = (words[1] >> 18) & 0x1FF;
        }
        insn.mnemonic = "v_vop3_" + std::to_string(insn.opcode);
        return insn;
    }

    // ===== VOP2 (top bit = 0, not VOP1) =====
    if ((w >> 31) == 0) {
        insn.category = Rdna2InsnCategory::VOP2;
        insn.opcode = (w >> 25) & 0x3F;
        insn.dst = (w >> 17) & 0xFF;
        insn.src0 = w & 0x1FF;
        insn.src1 = (w >> 9) & 0xFF;
        insn.is_64bit = false;

        switch (insn.opcode) {
            case 1:  insn.mnemonic = "v_add_f32"; break;
            case 2:  insn.mnemonic = "v_sub_f32"; break;
            case 3:  insn.mnemonic = "v_subrev_f32"; break;
            case 5:  insn.mnemonic = "v_mul_f32"; break;
            case 8:  insn.mnemonic = "v_mul_legacy_f32"; break;
            case 9:  insn.mnemonic = "v_mac_f32"; break;
            case 16: insn.mnemonic = "v_max_f32"; break;
            case 17: insn.mnemonic = "v_min_f32"; break;
            case 25: insn.mnemonic = "v_add_u32"; break;
            case 26: insn.mnemonic = "v_sub_u32"; break;
            case 27: insn.mnemonic = "v_and_b32"; break;
            case 28: insn.mnemonic = "v_or_b32"; break;
            case 29: insn.mnemonic = "v_xor_b32"; break;
            default: insn.mnemonic = "v_vop2_" + std::to_string(insn.opcode); break;
        }
        return insn;
    }

    // ===== Unknown =====
    insn.category = Rdna2InsnCategory::UNKNOWN;
    insn.mnemonic = "unknown_0x" + fmt::format("{:08X}", w);
    return insn;
}

void ShaderRecompiler::emit_spirv_for_instruction(const Rdna2DecodedInsn& insn, std::vector<uint32_t>& spirv) {
    // For each recognized RDNA2 instruction, emit corresponding SPIR-V opcodes
    // This is a foundation — real shaders need full register file tracking

    // Currently we emit OpNop (Word count 1, Opcode 0) as a placeholder for each
    // decoded instruction, proving the decode-emit pipeline works.
    // As the recompiler matures, each case will emit real SPIR-V compute.

    switch (insn.category) {
        case Rdna2InsnCategory::SOPP:
            if (insn.mnemonic == "s_endpgm") {
                // End of program — we'll emit OpReturn + OpFunctionEnd at the end
                return;
            }
            if (insn.mnemonic == "s_waitcnt" || insn.mnemonic == "s_nop" || insn.mnemonic == "s_barrier") {
                // These don't have SPIR-V equivalents — skip
                return;
            }
            break;

        case Rdna2InsnCategory::VOP2:
        case Rdna2InsnCategory::VOP1:
        case Rdna2InsnCategory::SOP1:
        case Rdna2InsnCategory::SOP2:
            // Emit OpNop as placeholder for each ALU operation
            spirv.push_back((1 << 16) | 0); // OpNop
            break;

        case Rdna2InsnCategory::SMEM:
            // Scalar memory load — placeholder
            spirv.push_back((1 << 16) | 0); // OpNop
            break;

        case Rdna2InsnCategory::EXP:
            // Export instruction — the final output
            spirv.push_back((1 << 16) | 0); // OpNop
            break;

        default:
            break;
    }
}

ShaderRecompileResult ShaderRecompiler::recompile(const void* rdna2_bytes, size_t size_bytes, ShaderType stage) {
    ShaderRecompileResult result{};
    if (!rdna2_bytes || size_bytes == 0) {
        result.error_message = "Empty RDNA2 shader binary buffer.";
        return result;
    }

    ShaderHash hash = compute_hash(rdna2_bytes, size_bytes);
    std::vector<uint32_t> spirv = emit_spirv_header(stage);

    // =====================================================================
    // RDNA2 Instruction Decode Pass
    // =====================================================================
    const uint32_t* words = static_cast<const uint32_t*>(rdna2_bytes);
    size_t total_dwords = size_bytes / 4;
    size_t decoded_count = 0;
    bool hit_endpgm = false;

    for (size_t i = 0; i < total_dwords && !hit_endpgm; ) {
        Rdna2DecodedInsn insn = decode_rdna2_instruction(&words[i], total_dwords - i);
        decoded_count++;

        QUIN_LOG_DEBUG("ShaderRecompiler: [{}] DW[{}] = 0x{:08X} → {}",
                       decoded_count, i, insn.raw_word0, insn.mnemonic);

        emit_spirv_for_instruction(insn, spirv);

        if (insn.mnemonic == "s_endpgm") {
            hit_endpgm = true;
        }

        // Advance by instruction size
        i += insn.is_64bit ? 2 : 1;
    }

    // ---- Close the SPIR-V function ----
    // OpReturn (Word count 1, Opcode 253)
    spirv.push_back((1 << 16) | 253);
    // OpFunctionEnd (Word count 1, Opcode 56)
    spirv.push_back((1 << 16) | 56);

    // ---- Build result ----
    CompiledShader shader{};
    shader.hash = hash;
    shader.stage = stage;
    shader.spirv_code = std::move(spirv);

    const uint8_t* byte_ptr = static_cast<const uint8_t*>(rdna2_bytes);
    shader.rdna2_bytecode.assign(byte_ptr, byte_ptr + size_bytes);
    shader.is_valid = true;

    result.success = true;
    result.shader = std::move(shader);
    result.instructions_recompiled = decoded_count;
    m_total_compiled++;

    QUIN_LOG_INFO("ShaderRecompiler: Recompiled {} Stage — Hash: 0x{:016X} | {} RDNA2 instructions decoded | {} SPIR-V words emitted",
                  (stage == ShaderType::Vertex ? "Vertex" : (stage == ShaderType::Pixel ? "Pixel" : "Compute")),
                  hash, decoded_count, result.shader.spirv_code.size());

    return result;
}

} // namespace quin::gpu::shader
