# 📁 Filesystem & Storage — Quin PS5 Emulator

## 1. Virtual Filesystem (VFS) (`src/fs/`)

### VFS Mount Architecture (`vfs.hpp`/`cpp`)
`VirtualFilesystem` maps guest PS5 POSIX path prefixes to local host directory structures:

| Guest Virtual Prefix | Host Local Target Directory | Description |
| :--- | :--- | :--- |
| `/app0/` | `./app0/` (or `./samples/`) | Executable game installation data & assets |
| `/data/` | `./data/` | System configuration & dynamic title data |
| `/system/` | `./system/` | System libraries & runtime assets |
| `/savedata/` | `./savedata/` | Isolated user title save containers |

### VFS Syscall File Operations
- `open_file(virtual_path, mode)`: Resolves host disk path and opens `std::fstream` handle. Automatically creates missing parent directories using `std::filesystem::create_directories`.
- `read_file(handle, buffer, bytes)`: Reads byte stream from disk.
- `write_file(handle, buffer, bytes)`: Writes byte stream to disk.
- `close_file(handle)`: Closes file handle.

---

## 2. SaveData Container Manager (`src/fs/savedata.hpp`/`cpp`)

`SaveDataManager` handles isolated savedata storage:
- **Path Resolution**: `/savedata/<user_id>/<title_id>/` (e.g. `./savedata/1000/CUSA00001/`).
- **Quota Enforcer**: Verifies container size limit allocations (`max_bytes`).
- **Mount Management**: `mount_savedata()` initializes save containers on launch.

---

## 3. Kraken / Oodle Asset Decompressor (`src/fs/decompression/`)

### Kraken Decoder Pipeline (`kraken_decoder.hpp`/`cpp`)
- `KrakenDecoder` provides clean-room chunked byte-stream decompression for PS5 compressed game asset streams.
- **Decompression Algorithm**: Reads compressed chunk headers, processes byte streams, and outputs uncompressed asset payloads into guest memory.
- **Telemetry**: Tracks total decompressed bytes (`get_total_decompressed_bytes()`).
