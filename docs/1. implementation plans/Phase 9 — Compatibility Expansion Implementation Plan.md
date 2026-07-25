# Phase 9 — Compatibility Expansion Implementation Plan

This plan details the technical architecture and implementation strategy for **Phase 9 — Compatibility Expansion** of the **Quin** PS5 emulator.

## User Review Required

> [!IMPORTANT]
> - **Per-Title Compatibility Database**: Tracks title compatibility states (`Boots`, `Menu`, `Ingame`, `Playable`, `Perfect`) by Title ID (`CUSAXXXXX`).
> - **Stub Triage & Symbol Logger**: Records frequency of unimplemented `libSce*` symbols and FreeBSD syscalls during runtime execution.
> - **Automated Regression Test Suite**: Validates that new system module additions do not break previously functional titles.

## Proposed Changes

---

### Compatibility Subsystem (`src/compat/`)

#### [NEW] [title_db.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/compat/title_db.hpp) & [title_db.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/compat/title_db.cpp)
- `TitleDatabase` managing per-title status entries, region, target framerate, and export to `compatibility.md`.

#### [NEW] [compat_triage.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/compat/compat_triage.hpp) & [compat_triage.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/compat/compat_triage.cpp)
- `CompatTriage` logging frequency of missing symbols/syscalls and running regression tests on registered test binaries.

---

### Debug UI & Integration (`src/gui/`)

#### [MODIFY] [debug_shell.hpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.hpp) & [debug_shell.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/src/gui/debug_shell.cpp)
- Add "Compatibility Tracker & Stub Triage" ImGui panel displaying title database entries, status breakdown charts, missing symbol frequencies, and regression test runner.

---

### Build System & Unit Tests (`CMakeLists.txt`, `tests/unit/`)

#### [MODIFY] [CMakeLists.txt](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/CMakeLists.txt)
- Include compatibility sources in `quin-core`.
- Add `tests/unit/test_compatibility_db.cpp` target to `quin-tests`.

#### [NEW] [test_compatibility_db.cpp](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/tests/unit/test_compatibility_db.cpp)
- Catch2 unit tests for title database lookup, status transitions, symbol triage frequency counting, and regression runner.

---

### Documentation (`README.md`, `compatibility.md`)

#### [NEW] [compatibility.md](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/compatibility.md)
- Markdown compatibility status matrix listing test titles and compatibility ratings.

#### [MODIFY] [README.md](file:///c:/Users/Masem/Downloads/0.%20old/Claude%20Work/Quin%20Mains/Quin/README.md)
- Update status table marking Phase 9 as ✅ **Complete** and Phase 10 as 🟡 **Next** (before git push).

---

## Verification Plan

### Automated Tests
```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
```

### Manual Verification
- Launch `quin.exe`, view compatibility database, search titles, check symbol triage logs, and execute regression test suite in the ImGui debug shell.
