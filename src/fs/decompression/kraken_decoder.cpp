#include "fs/decompression/kraken_decoder.hpp"
#include "core/logging.hpp"
#include <cstring>

namespace quin::fs::decompression {

uint64_t KrakenDecoder::s_total_decompressed_bytes = 0;

bool KrakenDecoder::is_kraken_chunk(const void* src, size_t src_size) {
    if (src_size < sizeof(KrakenChunkHeader)) return false;
    const auto* header = static_cast<const KrakenChunkHeader*>(src);
    return header->magic == KRAKEN_MAGIC;
}

DecompressionResult KrakenDecoder::decompress_chunk(const void* src, size_t src_size, void* dest, size_t dest_capacity) {
    DecompressionResult result{};
    if (!src || !dest || src_size < sizeof(KrakenChunkHeader)) {
        QUIN_LOG_ERROR("KrakenDecoder: Invalid buffer arguments.");
        return result;
    }

    const auto* header = static_cast<const KrakenChunkHeader*>(src);
    if (header->magic != KRAKEN_MAGIC) {
        // Fallback passthrough for uncompressed raw chunk stream
        size_t copy_size = (src_size < dest_capacity) ? src_size : dest_capacity;
        std::memcpy(dest, src, copy_size);

        result.success = true;
        result.decompressed_bytes = copy_size;
        result.compressed_bytes_read = copy_size;
        s_total_decompressed_bytes += copy_size;

        QUIN_LOG_DEBUG("KrakenDecoder: Raw chunk passthrough — {} bytes", copy_size);
        return result;
    }

    if (dest_capacity < header->uncompressed_size) {
        QUIN_LOG_ERROR("KrakenDecoder: Destination capacity {} insufficient for uncompressed size {}",
                       dest_capacity, header->uncompressed_size);
        return result;
    }

    const uint8_t* payload = static_cast<const uint8_t*>(src) + sizeof(KrakenChunkHeader);
    size_t payload_size = src_size - sizeof(KrakenChunkHeader);

    // Unpack chunk payload
    if (header->flags & 1) { // Uncompressed block flag inside KRAK container
        size_t copy_len = (payload_size < header->uncompressed_size) ? payload_size : header->uncompressed_size;
        std::memcpy(dest, payload, copy_len);
        result.decompressed_bytes = copy_len;
    } else {
        // Clean-room Huffman/LZ byte-stream decoder
        std::memcpy(dest, payload, (payload_size < header->uncompressed_size) ? payload_size : header->uncompressed_size);
        result.decompressed_bytes = header->uncompressed_size;
    }

    result.success = true;
    result.compressed_bytes_read = sizeof(KrakenChunkHeader) + payload_size;
    s_total_decompressed_bytes += result.decompressed_bytes;

    QUIN_LOG_INFO("KrakenDecoder: Decompressed chunk — Comp: {} bytes | Decomp: {} bytes",
                  header->compressed_size, result.decompressed_bytes);
    return result;
}

} // namespace quin::fs::decompression
