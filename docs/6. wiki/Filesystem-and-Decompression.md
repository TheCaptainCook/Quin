# 📁 Filesystem & Decompression — Quin PS5 Emulator

## 1. Virtual Filesystem (VFS) Layer (`src/fs/`)

### Architecture & Mount Table (`vfs.hpp`/`cpp`)
`VirtualFileSystem` maps PS5 guest virtual paths to host system directories:
- `/app0/` ➔ Local game application directory (`./app0/` or `./samples/`)
- `/data/` ➔ Guest system data assets (`./data/`)
- `/system/` ➔ System configuration files (`./system/`)
- `/savedata/` ➔ Isolated user save container directory (`./savedata/`)

### File Access Operations
- `open_file(guest_vpath, flags)`: Resolves host path, creates parent directories if needed, opens file stream.
- `read_file(handle, dest, size)`: Reads bytes from file stream into guest memory buffer.
- `write_file(handle, src, size)`: Writes bytes from guest memory to file stream.
- `seek_file(handle, offset, whence)`: Seeks file stream position (`SEEK_SET`, `SEEK_CUR`, `SEEK_END`).
- `close_file(handle)`: Flushes and closes file stream.
- `stat_file(guest_vpath, out_size)`: Queries file size on host filesystem.

---

## 2. SaveData Container Manager (`src/fs/savedata.hpp`/`cpp`)

### Savedata Directory Layout
`SaveDataManager` isolates save files per User ID and Title ID:
```
./savedata/
└── <user_id>/             (e.g., 1000)
    └── <title_id>/        (e.g., CUSA00001)
        ├── save_0.dat
        └── header.bin
```
- `mount_savedata(config)`: Mounts save directory for given `user_id`, `title_id`, and `quota_bytes` (default 32 MB).
- `get_save_directory(user_id, title_id)`: Resolves host directory path for save container.
- `has_savedata(user_id, title_id, filename)`: Checks file existence inside save container.

---

## 3. Clean-Room LZ77 Decompression (`src/fs/decompression/`)

### Decompression Engine (`kraken_decoder.hpp`/`cpp`)
`KrakenDecoder` provides clean-room LZ77 decompression for KRAK-magic asset chunks:
- Magic Constant: `0x4B52414B` (`KRAK`).
- Reads `KrakenChunkHeader` (magic, uncompressed_size, compressed_size, flags).
- **Uncompressed Flag (`0x01`)**: Raw payload copy.
- **LZ77 Bitstream Decoder**:
  - Control Byte: High nibble = literal length, Low nibble = match length base.
  - Literal Bytes: Copied directly from input payload.
  - 2-Byte Match Offsets: Little-endian relative offset reading from already-decompressed output buffer.
  - Extended Length Handling: Chained extra byte decoding for literal/match lengths exceeding 15.
