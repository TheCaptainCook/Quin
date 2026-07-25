# 🎯 Compatibility & Triage — Quin PS5 Emulator

## 1. Compatibility Database (`src/compat/title_db.hpp`/`cpp`)

### Title Status Ratings
`TitleDatabase` tracks compatibility ratings across 5 status levels:
1. `Nothing`: Title does not load or parse.
2. `Boots`: Binary loads and starts execution.
3. `Menu`: Title reaches intro logos or main menu screen.
4. `Ingame`: Title enters active gameplay with graphics/audio rendering.
5. `Playable`: Title completes from start to finish without major glitches or crashes.
6. `Perfect`: Flawless performance and visual fidelity.

### Title Database Exports
- `get_title_info(title_id)`: Retrieves status info, target FPS, and notes for a Title ID (`CUSAXXXXX`).
- `export_compatibility_markdown()`: Generates `compatibility.md` report matrix.

---

## 2. Automated Stub Triage Logger (`src/compat/compat_triage.hpp`/`cpp`)

### Missing Symbol & Syscall Logger
`CompatTriage` records missing system symbols and FreeBSD syscalls during runtime execution:
- `log_missing_symbol(symbol, module)`: Increments missing call count for a given symbol and module (fed automatically by `DynamicLinker`).
- `log_missing_syscall(sys_num)`: Tracks missing syscall frequency.
- `get_top_missing_symbols(limit)`: Returns top N missing symbols sorted by call frequency.

---

## 3. Functional Regression Suite (`src/compat/compat_triage.cpp`)

### `run_regression_suite()`
Executes 6 real functional regression test cases across core emulator subsystems:
1. **Memory Address Space**: Tests guest memory `allocate`, `write_bytes`, `read_bytes`, data verification, and `munmap`.
2. **VFS File Operations**: Tests VFS `mount`, `open_file`, `write_file`, `seek_file` (SEEK_SET), `read_file`, and data integrity.
3. **Thread Manager & Stack**: Tests guest thread creation, stack allocation with guard pages, active thread info queries, and thread joining.
4. **Syscall Dispatcher**: Tests dispatching known syscalls (`SYS_getpid` ➔ 1001) and unknown syscalls (returns `-1` / ENOSYS).
5. **Shader Cache Lookup**: Tests shader cache put, get, hash lookup, and stage matching.
6. **ELF Parser Safety**: Verifies safety when parsing invalid buffers.
