#ifndef QUIN_GPU_GNM_PARSER_HPP
#define QUIN_GPU_GNM_PARSER_HPP

#include "gpu/gnm_types.hpp"
#include "memory/address_space.hpp"
#include <vector>
#include <cstdint>

namespace quin::gpu {

struct ParseResult {
    bool success{false};
    size_t dwords_parsed{0};
    std::vector<GnmDrawCommand> draw_commands;
    GnmContextState context_state;
};

class GnmCmdParser {
public:
    explicit GnmCmdParser(quin::memory::GuestAddressSpace& memory);

    ParseResult parse_command_buffer(uint64_t ring_vaddr, size_t size_dwords);

    uint64_t get_total_packets_parsed() const { return m_total_packets_parsed; }
    uint64_t get_total_draw_calls() const { return m_total_draw_calls; }

private:
    quin::memory::GuestAddressSpace& m_memory;
    uint64_t m_total_packets_parsed{0};
    uint64_t m_total_draw_calls{0};
};

} // namespace quin::gpu

#endif // QUIN_GPU_GNM_PARSER_HPP
