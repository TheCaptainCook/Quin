# Phase 4 — Filesystem, Storage & Decompression Implementation Plan

This plan details the technical architecture and implementation strategy for **Phase 4 — Filesystem, Storage & Decompression** of the **Quin** PS5 emulator.

## User Review Required

> [!IMPORTANT]
> - **Virtual Filesystem (VFS) Mounts**: Maps guest mount points (`/app0/`, `/data/`, `/system/`, `/savedata/`) to host workspace directories.
> - **Kraken / Oodle Decompression**: Implements a clean-room chunked byte-stream decoder interface handling compressed game asset streams and uncompressed fallback blocks.
> - **Savedata Management**: Isolated savedata storage per user and title ID in host storage (`savedata/<user_id>/<title_id>/`).

## Proposed Changes

---

### Virtual Filesystem Subsystem (`src/fs/`)

#### [NEW] [vfs.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/fs/vfs.hpp) & [vfs.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/fs/vfs.cpp)
- `VirtualFileSystem` manager mapping guest virtual paths (`/app0/`, `/data/`, `/savedata/`) to host local directories.
- File descriptor table and path resolution (`open`, `read`, `write`, `close`, `stat`, `seek`).

#### [NEW] [savedata.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/fs/savedata.hpp) & [savedata.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/fs/savedata.cpp)
- `SaveDataManager` handling savedata directory initialization, quota verification, and savedata container mounts.

---

### Decompression Pipeline (`src/fs/decompression/`)

#### [NEW] [kraken_decoder.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/fs/decompression/kraken_decoder.hpp) & [kraken_decoder.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/fs/decompression/kraken_decoder.cpp)
- Clean-room Kraken chunk decompression engine parsing compressed asset streams and decompressing payloads into guest buffer memory.

---

### Kernel & Debug UI Integration (`src/kernel/`, `src/gui/`)

#### [MODIFY] [syscall_table.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/kernel/syscall_table.cpp)
- Connect `SYS_open` (#5), `SYS_read` (#3), `SYS_write` (#4), `SYS_close` (#6) syscalls directly to `VirtualFileSystem`.

#### [MODIFY] [debug_shell.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.hpp) & [debug_shell.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.cpp)
- Add "Virtual Filesystem & Storage" ImGui panel displaying active mount points, open file handles, I/O bandwidth, and decompression metrics.

---

### Build System & Unit Tests (`CMakeLists.txt`, `tests/unit/`)

#### [MODIFY] [CMakeLists.txt](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/CMakeLists.txt)
- Include new filesystem and decompression source files in `quin-core`.
- Add `tests/unit/test_vfs_decompression.cpp` target to `quin-tests`.

#### [NEW] [test_vfs_decompression.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/tests/unit/test_vfs_decompression.cpp)
- Catch2 unit tests for VFS mount path resolution, file I/O, savedata creation, and Kraken chunk decompression.

---

### Documentation (`README.md`)

#### [MODIFY] [README.md](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/README.md)
- Update status table marking Phase 4 as ✅ **Complete** and Phase 5 as 🟡 **Next** (before git push).

---

## Verification Plan

### Automated Tests
```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
```

### Manual Verification
- Launch `quin.exe`, verify VFS mount points (`/app0/`, `/savedata/`), open file handles, and decompression metrics in the ImGui debug shell.
