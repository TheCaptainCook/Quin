# PS5 Emulator — Detailed Development Roadmap
*Companion to `ps5-emulator-independent-build-brief.md`. Effort ratings assume a small, part-time/hobbyist team — the same scale the current public projects (SharpEmu, KytyPS5, RPCSX) are built at.*

## Overview

| # | Phase | Goal | Effort |
|---|---|---|---|
| 0 | Foundations & Tooling | Environment ready to write emulator code | Small |
| 1 | Executable Loading | A homebrew ELF's entry point executes | Medium |
| 2 | CPU Execution & Memory Model | Guest code runs safely, multi-threaded | Medium–Large |
| 3 | Syscalls & System Libraries | Real games get past initialization | Large (never fully "done") |
| 4 | Filesystem & Decompression | Games can find and load their own data | Medium |
| 5 | GPU Command Processing → Vulkan | Something *correct* appears on screen | Large |
| 6 | Shader Recompilation | 3D titles render | Large |
| 7 | Audio | Sound plays in sync | Small–Medium |
| 8 | Input | Player can control the game | Small |
| 9 | Compatibility Expansion | A few titles → a real library | Ongoing |
| 10 | Performance / the 60fps pass | Playable → smooth, per-title | Ongoing |

---

## Phase 0 — Foundations & Tooling
**Goal:** an environment where you can actually start writing emulator code.
- Set up the repo, pick a license deliberately (MIT/BSD/GPL — this decides how others can build on *you* later), and write a short clean-room policy doc: no Sony code/keys, no borrowed code from other GPL emulator projects.
- Build system: CMake + vcpkg or Conan; CI (GitHub Actions) building Windows/Linux/macOS from day one.
- Pull in core third-party libs: Vulkan-Headers/Loader, SDL3, Dear ImGui, spdlog, Catch2 or GoogleTest.
- Stand up a minimal debug shell (ImGui window, log console, "load ELF" button) — you'll live in this for years, so it's worth doing properly now.
- Gather primary references: AMD's public RDNA2 ISA manual, the Khronos Vulkan spec, published FreeBSD syscall tables, public technical write-ups on the self/eboot/PKG formats.

**Exit criteria:** empty emulator shell builds in CI on all three target OSes, opens a window, has a working log pane.

---

## Phase 1 — Executable Loading & Process Bootstrap
**Goal:** get a homebrew ELF's entry point to execute at all.
- Implement a self/eboot/PKG parser — header, segment table, entry-point extraction — built from published format documentation, not from another project's parser code.
- Guest virtual address space allocator matching the PS5's expected memory layout.
- ELF segment loader: map `PT_LOAD` segments into guest memory with correct permissions.
- Minimal libkernel stub: enough exported symbols (even if most just return "unsupported") that the loader's PLT/GOT resolves without crashing.
- Basic guest→host trap/exit path so a crash produces a readable log instead of a silent hang.

**Exit criteria:** a simple homebrew "hello world" ELF reaches its entry point and executes under your control. *(This is roughly where the public field is as of mid-2026.)*

---

## Phase 2 — CPU Execution & Memory Model
**Goal:** guest code runs correctly and safely, including multi-threaded code.
- PS5 and host share x86-64, so most instructions execute natively — but you still need a thin harness to trap syscalls/interrupts (e.g. a hooked `syscall` instruction or JIT-inserted trampolines) rather than "just running it" unsupervised.
- Guest memory manager: mmap/mprotect/munmap emulation matching PS5 address-space conventions, guard pages, per-thread stack setup.
- Thread model: map guest thread creation onto host threads; implement TLS matching the guest ABI.
- Signal/exception translation: host SIGSEGV/illegal-instruction → meaningful guest-side diagnostics. Invest here — every later phase leans on this for debugging.
- Confirm whether any instructions genuinely need software emulation (PS5-specific ISA extensions, if any) — for the large majority of code this shouldn't be necessary given the shared ISA.

**Exit criteria:** multi-threaded homebrew samples run correctly and repeatably; a guest crash produces a real stack trace instead of taking down the whole emulator.

---

## Phase 3 — Syscalls & System Libraries
**Goal:** real games stop failing immediately on startup.
- Build and document your own syscall table — numbers, argument conventions, return semantics — derived from published FreeBSD syscall references.
- Implement core syscalls first: file open/read/write/close, memory ops, thread ops, time/clock functions.
- Implement the "Sce*" system modules games actually link against (libSceLibcInternal, libSceSystemService, libSceUserService, etc.). This surface is huge and open-ended — prioritize by logging every unimplemented-function hit across your test titles and working top-down by frequency, not by guesswork.
- Sandbox/process-model behavior only to the extent games actually probe for it; full sandbox fidelity isn't the goal, behavioral compatibility is.

**Exit criteria:** simple 2D/indie titles get past initialization into their actual main loop, not just to a splash screen.
**Note:** this phase's long tail runs alongside Phase 9 for the rest of the project's life.

---

## Phase 4 — Filesystem, Storage & Decompression
**Goal:** games can find and read their own data.
- Virtual filesystem layer mapping a dumped game's directory structure to the mount points guest code expects (`app0/`, `data/`, etc.).
- Savedata mount emulation; trophy calls can be stubbed to "always succeeds" initially.
- The SSD I/O pipeline relies on Kraken-family decompression, which is RAD Game Tools' proprietary Oodle codec — check their current licensing terms (these have varied by use case over the years) before deciding whether to license it directly or invest in an independent decoder.

**Exit criteria:** a real game's assets load without file-not-found or decompression errors.

---

## Phase 5 — GPU Command Processing → Vulkan Translation
**Goal:** something appears on screen that isn't garbage.
- Intercept and parse GNM command buffers: draw calls, state-object updates, resource bindings.
- Build the Vulkan backend: translate GNM pipeline/render state into Vulkan pipeline state objects, with caching from the start — you'll otherwise rebuild these every frame.
- Resource binding translation: map GNM's descriptor/texture/buffer model onto Vulkan descriptor sets and formats.
- Sequence your test targets deliberately: a single triangle, then a 2D sprite-based title, before attempting anything 3D.

**Exit criteria:** a simple 2D title renders correctly, in real time.

---

## Phase 6 — Shader Recompilation
**Goal:** 3D titles render with correct-looking geometry and materials.
- Build a recompiler translating PS5 shader binaries (RDNA2 ISA) into SPIR-V, using SPIRV-Cross/SPIRV-Tools on the IR side.
- Handle common stages first — vertex and pixel shaders — before compute shaders.
- Shader cache keyed by binary hash so you're not re-translating on every launch.

**Exit criteria:** a simple 3D title reaches its actual in-game rendering state, even if visually imperfect.

---

## Phase 7 — Audio
**Goal:** sound comes out in sync with the game.
- Stub/implement `libSceAudioOut` and the Tempest 3D AudioTech entry points, routing PCM buffers to a host backend (SDL_audio, miniaudio, or PortAudio).
- Basic mixing/format conversion first; spatial audio is a later refinement, not a blocker.

**Exit criteria:** audio plays back without desync or crashes during Phase 5/6 testing.

---

## Phase 8 — Input
**Goal:** the player can actually control the game.
- Map DualSense HID reports onto your host controller layer (SDL already has DualSense support you can learn the *shape* of the problem from, without copying its implementation).
- Rumble/adaptive-trigger passthrough is a nice-to-have, not a blocker.

**Exit criteria:** menu navigation and in-game control both work on your test titles.

---

## Phase 9 — Compatibility Expansion (the long grind)
**Goal:** go from a handful of working titles to a real library.
- Stand up a public, per-title compatibility tracker: Boots → Menu → In-game → Playable → Full-speed. This becomes your project's actual progress signal, the way SharpEmu's public tracker works today.
- Triage unimplemented-function logs across a growing test library; fix by real-world frequency, not by whichever game you personally want to play.
- Build a regression suite from your already-working titles so new syscall/GPU work doesn't silently break old progress.

**Exit criteria:** there isn't a clean one — this is the ongoing state of the project. A reasonable interim goal: a growing, double-digit list of titles reaching "playable."
**Effort:** ongoing and effectively unbounded — every real console emulator spends years here.

---

## Phase 10 — Performance & the 60fps Pass
**Goal:** playable titles become smooth titles.
- Persist compiled Vulkan pipelines to disk to kill first-launch/traversal stutter.
- Async shader compilation off the render thread.
- Multi-threaded command-buffer translation and submission.
- Dynamic resolution scaling and open-source FSR2/3 integration for upscaling headroom.
- Frame-pacing and present-mode tuning.
- Only optimize titles that are already functionally correct, and profile each one individually — bottlenecks differ wildly game to game.

**Exit criteria:** your best-supported titles hold a stable, correct framerate — tracked per title, never claimed for the whole library at once.

---

## How to sequence this
- **Phases 0 → 3 are strictly sequential** — each one blocks the next.
- Once Phase 3 is minimally stable, **Phases 4, 7, and 8 can run in parallel with 5 and 6** — different subsystems, good split points if you gain contributors.
- **Phase 9 starts the moment you have your first working title, and never really stops.**
- **Phase 10 only starts once a title has cleared Phase 9's "in-game" tier** — chasing speed before correctness is how emulator projects stall out.

**Bottom line:** phases 0–8 are the part you can realistically plan and schedule. Phases 9–10 are the part that actually determines whether this becomes a real emulator — and across this entire genre, those are measured in years, not sprints.