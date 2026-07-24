#ifndef QUIN_FS_DECOMPRESSION_KRAKEN_DECODER_HPP
#define QUIN_FS_DECOMPRESSION_KRAKEN_DECODER_HPP

#include <cstdint>
#include <cstddef>
#include <vector>

namespace quin::fs::decompression {

constexpr uint32_t KRAKEN_MAGIC = 0x4B52414B; // 'KRAK'

struct KrakenChunkHeader {
    uint32_t magic{0};
    uint32_t uncompressed_size{0};
    uint32_t compressed_size{0};
    uint32_t flags{0};
};

struct DecompressionResult {
    bool success{false};
    size_t decompressed_bytes{0};
    size_t compressed_bytes_read{0};
};

class KrakenDecoder {
public:
    KrakenDecoder() = default;

    static DecompressionResult decompress_chunk(const void* src, size_t src_size, void* dest, size_t dest_capacity);
    static bool is_kraken_chunk(const void* src, size_t src_size);

    static uint64_t get_total_decompressed_bytes() { return s_total_decompressed_bytes; }

private:
    static uint64_t s_total_decompressed_bytes;
};

} // namespace quin::fs::decompression

#endif // QUIN_FS_DECOMPRESSION_KRAKEN_DECODER_HPP
