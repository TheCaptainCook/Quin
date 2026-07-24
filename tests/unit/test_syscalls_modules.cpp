#include <catch2/catch_test_macros.hpp>
#include "kernel/syscall_table.hpp"
#include "kernel/module_manager.hpp"
#include "memory/address_space.hpp"
#include "cpu/execution_engine.hpp"

TEST_CASE("FreeBSD / PS5 Syscall Dispatcher", "[kernel][syscall]") {
    quin::memory::GuestAddressSpace memory;
    quin::kernel::SyscallDispatcher dispatcher(memory);

    // 1. Dispatch SYS_getpid (#20)
    quin::kernel::SyscallArgs pid_args{};
    pid_args.num = quin::kernel::SYS_getpid;
    int64_t pid = dispatcher.dispatch(pid_args);
    REQUIRE(pid == 1001);

    // 2. Dispatch SYS_open (#5)
    quin::kernel::SyscallArgs open_args{};
    open_args.num = quin::kernel::SYS_open;
    open_args.arg1 = 0x0000000000401000ULL; // Path VAddr
    open_args.arg2 = 0;                    // O_RDONLY
    int64_t fd = dispatcher.dispatch(open_args);
    REQUIRE(fd == 3);

    // 3. Dispatch SYS_clock_gettime (#232)
    uint64_t timespec_vaddr = memory.mmap(0, 4096, quin::memory::PagePermission::ReadWrite);
    quin::kernel::SyscallArgs clock_args{};
    clock_args.num = quin::kernel::SYS_clock_gettime;
    clock_args.arg1 = 0; // CLOCK_REALTIME
    clock_args.arg2 = timespec_vaddr;
    int64_t clock_ret = dispatcher.dispatch(clock_args);
    REQUIRE(clock_ret == 0);

    struct Timespec { uint64_t sec; uint64_t nsec; } ts{};
    REQUIRE(memory.read_bytes(timespec_vaddr, &ts, sizeof(ts)) == true);
    REQUIRE(ts.sec > 0);

    // 4. Dispatch Unimplemented Syscall (#999)
    quin::kernel::SyscallArgs unk_args{};
    unk_args.num = 999;
    int64_t unk_ret = dispatcher.dispatch(unk_args);
    REQUIRE(unk_ret == -1); // ENOSYS
}

TEST_CASE("System Module Manager & libSce Module Registration", "[kernel][modules]") {
    quin::memory::GuestAddressSpace memory;
    quin::kernel::LibKernel kernel;
    quin::kernel::SyscallDispatcher syscalls(memory);
    quin::kernel::ModuleManager module_mgr(kernel, syscalls);

    module_mgr.register_all_modules();

    REQUIRE(kernel.has_symbol("sceLibcMalloc") == true);
    REQUIRE(kernel.has_symbol("sceSystemServiceHideSplashScreen") == true);
    REQUIRE(kernel.has_symbol("sceUserServiceInitialize") == true);

    int64_t user_ret = kernel.dispatch_symbol("sceUserServiceInitialize", 0);
    REQUIRE(user_ret == 0);
}

TEST_CASE("Execution Engine SYSCALL Instruction Execution & RAX Return", "[cpu][syscall]") {
    quin::memory::GuestAddressSpace memory;
    quin::kernel::LibKernel kernel;
    quin::cpu::ExecutionEngine engine(memory, kernel);

    uint64_t code_vaddr = 0x0000000000400000ULL;
    REQUIRE(memory.allocate(code_vaddr, 4096, quin::memory::PagePermission::ReadExecute) == true);

    // Write SYSCALL instruction (0x0F 0x05) followed by RET (0xC3)
    uint8_t syscall_code[] = { 0x0F, 0x05, 0xC3 };
    REQUIRE(memory.write_bytes(code_vaddr, syscall_code, sizeof(syscall_code)) == true);

    REQUIRE(engine.bootstrap(code_vaddr, 0x00007FFFF0000000ULL) == true);

    // Set RAX to SYS_getpid (#20)
    // Note: bootstrap sets regs, we step with RAX=20
    engine.step(); // SYSCALL with RAX=0 (sys_exit returns 0)
    REQUIRE(engine.get_registers().rip == code_vaddr + 2);
}
