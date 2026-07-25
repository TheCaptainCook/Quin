#ifndef QUIN_GPU_SHADER_SHADER_RECOMPILER_HPP
#define QUIN_GPU_SHADER_SHADER_RECOMPILER_HPP

#include "gpu/shader/shader_types.hpp"
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

namespace quin::gpu::shader {

// RDNA2 instruction encoding categories
enum class Rdna2InsnCategory {
    SOP1,      // Scalar one-operand
    SOP2,      // Scalar two-operand
    SOPC,      // Scalar comparison
    SOPK,      // Scalar immediate
    SOPP,      // Scalar program control
    VOP1,      // Vector one-operand
    VOP2,      // Vector two-operand
    VOP3,      // Vector three-operand
    VOPC,      // Vector comparison
    FLAT,      // Flat/global memory
    SMEM,      // Scalar memory
    DS,        // Local data share
    MUBUF,     // Buffer memory
    MTBUF,     // Typed buffer
    MIMG,      // Image memory
    EXP,       // Export
    UNKNOWN
};

struct Rdna2DecodedInsn {
    uint32_t raw_word0{0};
    uint32_t raw_word1{0};
    Rdna2InsnCategory category{Rdna2InsnCategory::UNKNOWN};
    uint32_t opcode{0};
    uint8_t dst{0};
    uint8_t src0{0};
    uint8_t src1{0};
    uint8_t src2{0};
    bool is_64bit{false};     // true if instruction is 2 dwords
    std::string mnemonic;
};

class ShaderRecompiler {
public:
    ShaderRecompiler() = default;

    ShaderRecompileResult recompile(const void* rdna2_bytes, size_t size_bytes, ShaderType stage);
    static ShaderHash compute_hash(const void* bytes, size_t size);

    uint64_t get_total_shaders_compiled() const { return m_total_compiled; }

private:
    std::vector<uint32_t> emit_spirv_header(ShaderType stage);
    Rdna2DecodedInsn decode_rdna2_instruction(const uint32_t* words, size_t remaining_dwords);
    void emit_spirv_for_instruction(const Rdna2DecodedInsn& insn, std::vector<uint32_t>& spirv);

    uint64_t m_total_compiled{0};
};

} // namespace quin::gpu::shader

#endif // QUIN_GPU_SHADER_SHADER_RECOMPILER_HPP
