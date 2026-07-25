#include "fs/decompression/kraken_decoder.hpp"
#include "core/logging.hpp"
#include <cstring>
#include <algorithm>

namespace quin::fs::decompression {

uint64_t KrakenDecoder::s_total_decompressed_bytes = 0;

bool KrakenDecoder::is_kraken_chunk(const void* src, size_t src_size) {
    if (!src || src_size < sizeof(KrakenChunkHeader)) {
        return false;
    }
    const auto* hdr = static_cast<const KrakenChunkHeader*>(src);
    return hdr->magic == KRAKEN_MAGIC;
}

DecompressionResult KrakenDecoder::decompress_chunk(const void* src, size_t src_size, void* dest, size_t dest_capacity) {
    DecompressionResult result{};
    if (!src || !dest || src_size < sizeof(KrakenChunkHeader) || dest_capacity == 0) {
        return result;
    }

    const auto* hdr = static_cast<const KrakenChunkHeader*>(src);
    if (hdr->magic != KRAKEN_MAGIC) {
        QUIN_LOG_WARN("KrakenDecoder: Invalid magic 0x{:08X} (expected 0x{:08X})", hdr->magic, KRAKEN_MAGIC);
        return result;
    }

    uint32_t uncompressed_size = hdr->uncompressed_size;
    uint32_t compressed_size = hdr->compressed_size;
    uint32_t flags = hdr->flags;

    if (uncompressed_size > dest_capacity) {
        QUIN_LOG_WARN("KrakenDecoder: Output buffer too small ({} < {})", dest_capacity, uncompressed_size);
        return result;
    }

    const uint8_t* payload = static_cast<const uint8_t*>(src) + sizeof(KrakenChunkHeader);
    size_t payload_size = src_size - sizeof(KrakenChunkHeader);

    if (compressed_size > payload_size) {
        QUIN_LOG_WARN("KrakenDecoder: Compressed payload truncated ({} > {})", compressed_size, payload_size);
        return result;
    }

    // Flag 0x01 = uncompressed (raw copy)
    if (flags & 0x01) {
        size_t copy_size = std::min(static_cast<size_t>(uncompressed_size), payload_size);
        std::memcpy(dest, payload, copy_size);
        result.success = true;
        result.decompressed_bytes = copy_size;
        result.compressed_bytes_read = sizeof(KrakenChunkHeader) + copy_size;
        s_total_decompressed_bytes += copy_size;

        QUIN_LOG_INFO("KrakenDecoder: Raw copy — {} bytes (uncompressed flag)", copy_size);
        return result;
    }

    // =========================================================================
    // LZ77 Decompression (Clean-Room Implementation)
    //
    // Byte-stream format:
    //   Control byte: high nibble = literal length, low nibble = match length base
    //   Literal bytes follow the control byte
    //   If match length base > 0: 2-byte little-endian offset follows
    //
    // Match length = base + 4 (minimum match = 4 bytes)
    // Literal length: if 0xF, read additional bytes until < 0xFF, then sum
    // Match length:   if 0xF, read additional bytes until < 0xFF, then sum + 4
    // =========================================================================
    uint8_t* out = static_cast<uint8_t*>(dest);
    size_t out_pos = 0;
    size_t in_pos = 0;

    while (in_pos < compressed_size && out_pos < uncompressed_size) {
        uint8_t ctrl = payload[in_pos++];
        uint8_t lit_len = (ctrl >> 4) & 0x0F;
        uint8_t match_len_base = ctrl & 0x0F;

        // Extended literal length
        size_t literal_length = lit_len;
        if (lit_len == 0x0F) {
            while (in_pos < compressed_size) {
                uint8_t extra = payload[in_pos++];
                literal_length += extra;
                if (extra < 0xFF) break;
            }
        }

        // Copy literal bytes
        if (literal_length > 0) {
            size_t copy_len = std::min(literal_length, static_cast<size_t>(compressed_size - in_pos));
            copy_len = std::min(copy_len, static_cast<size_t>(uncompressed_size - out_pos));
            std::memcpy(out + out_pos, payload + in_pos, copy_len);
            in_pos += copy_len;
            out_pos += copy_len;
        }

        // If we've filled the output, done
        if (out_pos >= uncompressed_size) break;

        // Match copy — only if there's a match component
        if (match_len_base > 0 || (ctrl & 0x0F) == 0x0F) {
            // Read 2-byte little-endian match offset
            if (in_pos + 1 >= compressed_size) break;
            uint16_t match_offset = static_cast<uint16_t>(payload[in_pos]) |
                                    (static_cast<uint16_t>(payload[in_pos + 1]) << 8);
            in_pos += 2;

            if (match_offset == 0) break; // Invalid offset

            // Extended match length
            size_t match_length = static_cast<size_t>(match_len_base) + 4;
            if (match_len_base == 0x0F) {
                while (in_pos < compressed_size) {
                    uint8_t extra = payload[in_pos++];
                    match_length += extra;
                    if (extra < 0xFF) break;
                }
            }

            // Copy from already-decompressed output (byte-by-byte for overlapping)
            if (match_offset > out_pos) {
                // Offset points before the start of output — fill with zeros
                size_t fill_len = std::min(match_length, static_cast<size_t>(uncompressed_size - out_pos));
                std::memset(out + out_pos, 0, fill_len);
                out_pos += fill_len;
            } else {
                size_t src_pos = out_pos - match_offset;
                size_t copy_len = std::min(match_length, static_cast<size_t>(uncompressed_size - out_pos));
                for (size_t j = 0; j < copy_len; ++j) {
                    out[out_pos++] = out[src_pos + (j % match_offset)];
                }
            }
        } else if (match_len_base == 0 && out_pos < uncompressed_size) {
            // No match component and not end-of-stream — check if we have more data
            if (in_pos >= compressed_size) break;
        }
    }

    result.success = (out_pos > 0);
    result.decompressed_bytes = out_pos;
    result.compressed_bytes_read = sizeof(KrakenChunkHeader) + in_pos;
    s_total_decompressed_bytes += out_pos;

    QUIN_LOG_INFO("KrakenDecoder: LZ77 decompressed {} → {} bytes (ratio: {:.1f}x)",
                  in_pos, out_pos,
                  in_pos > 0 ? static_cast<double>(out_pos) / in_pos : 0.0);

    return result;
}

} // namespace quin::fs::decompression
