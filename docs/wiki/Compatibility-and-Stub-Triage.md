# 🎯 Compatibility & Stub Triage — Quin PS5 Emulator

## 1. Title Compatibility Database (`src/compat/`)

### Compatibility Ratings (`title_db.hpp`/`cpp`)
`TitleDatabase` tracks title compatibility states:
- **Perfect**: Flawless gameplay, full target framerate, graphics and audio perfectly in sync.
- **Playable**: Game can be played from start to finish with minor visual/audio glitches.
- **In-game**: Reaches gameplay, but crashes or major visual glitches prevent further progress.
- **Menu**: Boots to main menu or title screen, but fails when starting gameplay.
- **Boots**: Displays intro splash screen or early debug logs before freezing.
- **Nothing**: Fails to boot or crashes immediately on launch.

### Export to Markdown
`TitleDatabase::export_to_markdown()` exports the live compatibility status matrix to `compatibility.md` in the project root.

---

## 2. Automated Stub Triage Engine (`compat_triage.hpp`/`cpp`)

`CompatTriage` logs missing system functions during runtime execution:
- **Symbol Logger**: Records unimplemented `libSce*` system function calls and unhandled FreeBSD syscalls.
- **Frequency Counter**: Counts call counts per missing symbol (`log_missing_symbol()`).
- **Triage Priority Ranking**: `get_top_missing_symbols()` ranks missing symbols by call frequency so developers prioritize high-impact stubs.

---

## 3. Automated Regression Test Runner

- `CompatTriage::run_regression_suite()` executes automated health checks across homebrew binaries, thread isolation, VFS mounts, and DualSense inputs to ensure new additions do not cause regression failures.
