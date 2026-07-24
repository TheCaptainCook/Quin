#include "gpu/gnm_parser.hpp"
#include "core/logging.hpp"
#include <vector>

namespace quin::gpu {

GnmCmdParser::GnmCmdParser(quin::memory::GuestAddressSpace& memory)
    : m_memory(memory) {}

ParseResult GnmCmdParser::parse_command_buffer(uint64_t ring_vaddr, size_t size_dwords) {
    ParseResult result{};
    if (size_dwords == 0) return result;

    std::vector<uint32_t> buffer(size_dwords, 0);
    if (!m_memory.read_bytes(ring_vaddr, buffer.data(), size_dwords * sizeof(uint32_t))) {
        QUIN_LOG_ERROR("GnmCmdParser: Memory read fault at command buffer 0x{:016X}", ring_vaddr);
        return result;
    }

    size_t index = 0;
    while (index < size_dwords) {
        uint32_t header_word = buffer[index];
        uint8_t count = (header_word >> 16) & 0x3FFF;
        uint8_t opcode = (header_word >> 8) & 0xFF;
        uint8_t type = (header_word >> 30) & 0x3;

        if (type != 3) {
            index++;
            continue;
        }

        m_total_packets_parsed++;
        size_t packet_size = count + 2; // Header + count DWORDs

        if (index + packet_size > size_dwords) {
            QUIN_LOG_WARN("GnmCmdParser: Truncated PM4 packet at DWORD offset {}", index);
            break;
        }

        if (opcode == IT_DRAW_INDEX_AUTO) {
            GnmDrawCommand cmd{};
            cmd.index_count = buffer[index + 1];
            cmd.primitive_type = static_cast<GnmPrimitiveType>(buffer[index + 2] & 0x3F);

            result.draw_commands.push_back(cmd);
            m_total_draw_calls++;

            QUIN_LOG_INFO("GnmCmdParser: IT_DRAW_INDEX_AUTO — Indices: {} | PrimType: {}",
                          cmd.index_count, static_cast<int>(cmd.primitive_type));
        } else if (opcode == IT_SET_CONTEXT_REG) {
            uint32_t reg_offset = buffer[index + 1] & 0xFFFF;
            if (reg_offset == 0x01) {
                result.context_state.render_target_format = static_cast<GnmSurfaceFormat>(buffer[index + 2]);
            }
        }

        index += packet_size;
    }

    result.success = true;
    result.dwords_parsed = index;
    return result;
}

} // namespace quin::gpu
