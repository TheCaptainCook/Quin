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
        uint16_t count = (header_word >> 16) & 0x3FFF;
        uint8_t opcode = (header_word >> 8) & 0xFF;
        uint8_t type = (header_word >> 30) & 0x3;

        if (type != 3) {
            index++;
            continue;
        }

        m_total_packets_parsed++;
        size_t packet_size = static_cast<size_t>(count) + 2; // Header + count DWORDs

        if (index + packet_size > size_dwords) {
            QUIN_LOG_WARN("GnmCmdParser: Truncated PM4 packet at DWORD offset {}", index);
            break;
        }

        switch (opcode) {
            case IT_NOP: {
                // No operation — skip packet payload
                QUIN_LOG_DEBUG("GnmCmdParser: IT_NOP — {} DWORDs", count);
                break;
            }

            case IT_SET_BASE: {
                // Set indirect buffer base address
                if (count >= 2 && index + 3 <= size_dwords) {
                    uint32_t base_lo = buffer[index + 1];
                    uint32_t base_hi = buffer[index + 2];
                    uint64_t base_addr = (static_cast<uint64_t>(base_hi) << 32) | base_lo;
                    QUIN_LOG_INFO("GnmCmdParser: IT_SET_BASE — Base Address: 0x{:016X}", base_addr);
                }
                break;
            }

            case IT_INDEX_TYPE: {
                // Set index buffer element type (16-bit or 32-bit)
                if (count >= 1 && index + 2 <= size_dwords) {
                    uint32_t index_type = buffer[index + 1] & 0x3;
                    result.context_state.index_size_16bit = (index_type == 0); // 0 = 16-bit, 1 = 32-bit
                    QUIN_LOG_INFO("GnmCmdParser: IT_INDEX_TYPE — {}", index_type == 0 ? "16-bit" : "32-bit");
                }
                break;
            }

            case IT_DRAW_INDEX_AUTO: {
                GnmDrawCommand cmd{};
                if (index + 2 < size_dwords) {
                    cmd.index_count = buffer[index + 1];
                    cmd.primitive_type = static_cast<GnmPrimitiveType>(buffer[index + 2] & 0x3F);
                } else if (index + 1 < size_dwords) {
                    cmd.index_count = buffer[index + 1];
                }

                result.draw_commands.push_back(cmd);
                m_total_draw_calls++;

                QUIN_LOG_INFO("GnmCmdParser: IT_DRAW_INDEX_AUTO — Indices: {} | PrimType: {}",
                              cmd.index_count, static_cast<int>(cmd.primitive_type));
                break;
            }

            case IT_DRAW_INDEX_2: {
                // Indexed draw with max index count
                GnmDrawCommand cmd{};
                if (count >= 4 && index + 5 <= size_dwords) {
                    cmd.index_count = buffer[index + 1];
                    uint32_t index_lo = buffer[index + 2];
                    uint32_t index_hi = buffer[index + 3];
                    cmd.index_buffer_vaddr = (static_cast<uint64_t>(index_hi) << 32) | index_lo;
                    cmd.primitive_type = static_cast<GnmPrimitiveType>(buffer[index + 4] & 0x3F);
                }

                result.draw_commands.push_back(cmd);
                m_total_draw_calls++;

                QUIN_LOG_INFO("GnmCmdParser: IT_DRAW_INDEX_2 — Indices: {} | IndexBuf: 0x{:016X} | PrimType: {}",
                              cmd.index_count, cmd.index_buffer_vaddr, static_cast<int>(cmd.primitive_type));
                break;
            }

            case IT_SET_CONTEXT_REG: {
                if (count >= 2 && index + 3 <= size_dwords) {
                    uint32_t reg_offset = buffer[index + 1] & 0xFFFF;
                    uint32_t reg_value = buffer[index + 2];

                    switch (reg_offset) {
                        case 0x01:
                            result.context_state.render_target_format = static_cast<GnmSurfaceFormat>(reg_value);
                            break;
                        case 0x02:
                            result.context_state.depth_format = static_cast<GnmSurfaceFormat>(reg_value);
                            break;
                        case 0x10: // CB_BLEND_CONTROL
                            result.context_state.blend_enable = (reg_value & 0x1) != 0;
                            QUIN_LOG_DEBUG("GnmCmdParser: CB_BLEND_CONTROL — Blend {}", result.context_state.blend_enable ? "ON" : "OFF");
                            break;
                        case 0x20: // DB_DEPTH_CONTROL
                            result.context_state.depth_test_enable = (reg_value & 0x1) != 0;
                            result.context_state.depth_write_enable = (reg_value & 0x2) != 0;
                            QUIN_LOG_DEBUG("GnmCmdParser: DB_DEPTH_CONTROL — Test {} | Write {}",
                                          result.context_state.depth_test_enable ? "ON" : "OFF",
                                          result.context_state.depth_write_enable ? "ON" : "OFF");
                            break;
                        case 0x30: // PA_SC_VPORT_SCISSOR
                            result.context_state.viewport_width = (reg_value >> 16) & 0xFFFF;
                            result.context_state.viewport_height = reg_value & 0xFFFF;
                            QUIN_LOG_DEBUG("GnmCmdParser: PA_SC_VPORT — {}x{}",
                                          result.context_state.viewport_width, result.context_state.viewport_height);
                            break;
                        default:
                            QUIN_LOG_DEBUG("GnmCmdParser: IT_SET_CONTEXT_REG — Offset: 0x{:04X} = 0x{:08X}",
                                          reg_offset, reg_value);
                            break;
                    }
                }
                break;
            }

            case IT_EVENT_WRITE: {
                if (count >= 1 && index + 2 <= size_dwords) {
                    uint32_t event_type = buffer[index + 1] & 0x3F;
                    QUIN_LOG_INFO("GnmCmdParser: IT_EVENT_WRITE — EventType: {}", event_type);
                }
                break;
            }

            case IT_WAIT_REG_MEM: {
                // Wait for a register/memory value to match a condition
                if (count >= 5 && index + 6 <= size_dwords) {
                    uint32_t function = buffer[index + 1] & 0x7;
                    uint32_t poll_lo = buffer[index + 2];
                    uint32_t poll_hi = buffer[index + 3];
                    uint32_t reference = buffer[index + 4];
                    uint32_t mask = buffer[index + 5];
                    uint64_t poll_addr = (static_cast<uint64_t>(poll_hi) << 32) | poll_lo;
                    QUIN_LOG_INFO("GnmCmdParser: IT_WAIT_REG_MEM — Func: {} | Addr: 0x{:016X} | Ref: 0x{:08X} | Mask: 0x{:08X}",
                                  function, poll_addr, reference, mask);
                }
                break;
            }

            default: {
                QUIN_LOG_DEBUG("GnmCmdParser: Unknown PM4 opcode 0x{:02X} — {} DWORDs", opcode, count);
                break;
            }
        }

        index += packet_size;
    }

    result.success = true;
    result.dwords_parsed = index;
    return result;
}

} // namespace quin::gpu
