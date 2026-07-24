#include <catch2/catch_test_macros.hpp>
#include "fs/vfs.hpp"
#include "fs/savedata.hpp"
#include "fs/decompression/kraken_decoder.hpp"
#include <vector>
#include <string>

TEST_CASE("Virtual Filesystem (VFS) Mounts & File Operations", "[fs][vfs]") {
    quin::fs::VirtualFileSystem vfs;

    // 1. Register Mount Point
    REQUIRE(vfs.mount("/test_app/", "./test_vfs_dir/") == true);

    // 2. Resolve Path
    std::string resolved = vfs.resolve_path("/test_app/content/data.bin");
    REQUIRE(resolved.find("test_vfs_dir") != std::string::npos);

    // 3. Open & Write File
    quin::fs::VfsFileHandle handle = vfs.open_file("/test_app/content/data.bin");
    REQUIRE(handle != quin::fs::INVALID_VFS_HANDLE);

    std::string payload = "Hello Quin PS5 Emulator VFS!";
    int64_t written = vfs.write_file(handle, payload.data(), payload.size());
    REQUIRE(written == static_cast<int64_t>(payload.size()));

    // 4. Close & Re-open Read File
    REQUIRE(vfs.close_file(handle) == true);

    quin::fs::VfsFileHandle read_handle = vfs.open_file("/test_app/content/data.bin");
    REQUIRE(read_handle != quin::fs::INVALID_VFS_HANDLE);

    std::vector<char> read_buf(payload.size() + 1, 0);
    int64_t bytes_read = vfs.read_file(read_handle, read_buf.data(), payload.size());
    REQUIRE(bytes_read == static_cast<int64_t>(payload.size()));
    REQUIRE(std::string(read_buf.data()) == payload);

    REQUIRE(vfs.close_file(read_handle) == true);

    // 5. Stat File
    uint64_t size = 0;
    REQUIRE(vfs.stat_file("/test_app/content/data.bin", size) == true);
    REQUIRE(size == payload.size());
}

TEST_CASE("SaveData Container Manager", "[fs][savedata]") {
    quin::fs::VirtualFileSystem vfs;
    quin::fs::SaveDataManager savedata_mgr(vfs);

    quin::fs::SaveDataConfig config{1000, "CUSA99999", 16 * 1024 * 1024};
    REQUIRE(savedata_mgr.mount_savedata(config) == true);

    std::string save_dir = savedata_mgr.get_save_directory(1000, "CUSA99999");
    REQUIRE(save_dir.find("1000/CUSA99999") != std::string::npos);
}

TEST_CASE("Kraken / Oodle Chunk Decompression Pipeline", "[fs][decompression]") {
    // 1. Raw Chunk Passthrough
    std::string raw_data = "Uncompressed raw asset stream content";
    std::vector<uint8_t> dest(128, 0);

    auto res1 = quin::fs::decompression::KrakenDecoder::decompress_chunk(
        raw_data.data(), raw_data.size(), dest.data(), dest.size()
    );
    REQUIRE(res1.success == true);
    REQUIRE(res1.decompressed_bytes == raw_data.size());

    // 2. Synthetic Kraken Chunk
    quin::fs::decompression::KrakenChunkHeader header;
    header.magic = quin::fs::decompression::KRAKEN_MAGIC;
    header.uncompressed_size = 16;
    header.compressed_size = 16;
    header.flags = 1; // Uncompressed container flag

    std::vector<uint8_t> chunk_buffer(sizeof(header) + 16, 0);
    std::memcpy(chunk_buffer.data(), &header, sizeof(header));
    std::memcpy(chunk_buffer.data() + sizeof(header), "0123456789ABCDEF", 16);

    std::vector<uint8_t> decomp_dest(32, 0);
    auto res2 = quin::fs::decompression::KrakenDecoder::decompress_chunk(
        chunk_buffer.data(), chunk_buffer.size(), decomp_dest.data(), decomp_dest.size()
    );

    REQUIRE(res2.success == true);
    REQUIRE(res2.decompressed_bytes == 16);
    REQUIRE(std::string(reinterpret_cast<char*>(decomp_dest.data()), 16) == "0123456789ABCDEF");
}
