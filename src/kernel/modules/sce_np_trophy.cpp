#include "kernel/modules/sce_np_trophy.hpp"
#include "core/logging.hpp"

namespace quin::kernel::modules {

void register_sce_np_trophy(LibKernel& kernel) {
    // sceNpTrophyCreateContext — Creates a trophy context for the application
    kernel.register_stub("sceNpTrophyCreateContext", [](uint64_t ctx_out, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("[libSceNpTrophy] sceNpTrophyCreateContext — Stub returning success (context=0x{:X})", ctx_out);
        return 0; // SCE_OK
    });

    // sceNpTrophyCreateHandle — Creates a trophy handle for API operations
    kernel.register_stub("sceNpTrophyCreateHandle", [](uint64_t handle_out, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("[libSceNpTrophy] sceNpTrophyCreateHandle — Stub returning success (handle=0x{:X})", handle_out);
        return 0; // SCE_OK
    });

    // sceNpTrophyDestroyContext — Destroys a trophy context
    kernel.register_stub("sceNpTrophyDestroyContext", [](uint64_t ctx, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("[libSceNpTrophy] sceNpTrophyDestroyContext — Stub (ctx=0x{:X})", ctx);
        return 0;
    });

    // sceNpTrophyDestroyHandle — Destroys a trophy handle
    kernel.register_stub("sceNpTrophyDestroyHandle", [](uint64_t handle, uint64_t, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("[libSceNpTrophy] sceNpTrophyDestroyHandle — Stub (handle=0x{:X})", handle);
        return 0;
    });

    // sceNpTrophyRegisterContext — Registers a trophy configuration
    kernel.register_stub("sceNpTrophyRegisterContext", [](uint64_t ctx, uint64_t handle, uint64_t, uint64_t) -> int64_t {
        QUIN_LOG_INFO("[libSceNpTrophy] sceNpTrophyRegisterContext — Stub (ctx=0x{:X}, handle=0x{:X})", ctx, handle);
        return 0;
    });

    // sceNpTrophyUnlockTrophy — Unlocks a trophy by ID (always succeeds)
    kernel.register_stub("sceNpTrophyUnlockTrophy", [](uint64_t ctx, uint64_t handle, uint64_t trophy_id, uint64_t) -> int64_t {
        QUIN_LOG_INFO("[libSceNpTrophy] sceNpTrophyUnlockTrophy — Trophy #{} UNLOCKED (stub)", trophy_id);
        return 0;
    });

    // sceNpTrophyGetTrophyUnlockState — Query unlock state of all trophies
    kernel.register_stub("sceNpTrophyGetTrophyUnlockState", [](uint64_t ctx, uint64_t handle, uint64_t state_out, uint64_t count_out) -> int64_t {
        QUIN_LOG_INFO("[libSceNpTrophy] sceNpTrophyGetTrophyUnlockState — Stub (all locked)");
        (void)ctx; (void)handle; (void)state_out; (void)count_out;
        return 0;
    });

    // sceNpTrophyGetTrophyInfo — Get info for a specific trophy
    kernel.register_stub("sceNpTrophyGetTrophyInfo", [](uint64_t ctx, uint64_t handle, uint64_t trophy_id, uint64_t info_out) -> int64_t {
        QUIN_LOG_INFO("[libSceNpTrophy] sceNpTrophyGetTrophyInfo — Stub (trophy #{})", trophy_id);
        (void)info_out;
        return 0;
    });

    // sceNpTrophyGetGameInfo — Get game-level trophy information
    kernel.register_stub("sceNpTrophyGetGameInfo", [](uint64_t ctx, uint64_t handle, uint64_t info_out, uint64_t) -> int64_t {
        QUIN_LOG_INFO("[libSceNpTrophy] sceNpTrophyGetGameInfo — Stub");
        (void)info_out;
        return 0;
    });

    QUIN_LOG_INFO("[ModuleManager] Registered module: libSceNpTrophy (9 stubs)");
}

} // namespace quin::kernel::modules
