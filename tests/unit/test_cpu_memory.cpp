#include <catch2/catch_test_macros.hpp>
#include "memory/address_space.hpp"
#include "cpu/thread_manager.hpp"
#include "cpu/execution_engine.hpp"
#include "cpu/exception_handler.hpp"
#include "kernel/libkernel.hpp"

#include <vector>

TEST_CASE("Guest Memory Manager Enhancements (mmap, mprotect, Guard Page)", "[memory]") {
    quin::memory::GuestAddressSpace memory;

    // 1. Dynamic mmap allocation
    uint64_t vaddr = memory.mmap(0, 8192, quin::memory::PagePermission::ReadWrite);
    REQUIRE(vaddr > 0);
    REQUIRE(memory.get_total_allocated_bytes() >= 8192);

    // 2. Write data and verify read
    uint32_t magic = 0xDEADBEEF;
    REQUIRE(memory.write_bytes(vaddr, &magic, sizeof(magic)) == true);

    uint32_t read_magic = 0;
    REQUIRE(memory.read_bytes(vaddr, &read_magic, sizeof(read_magic)) == true);
    REQUIRE(read_magic == magic);

    // 3. mprotect permission modification to Read-Only
    REQUIRE(memory.mprotect(vaddr, 8192, quin::memory::PagePermission::Read) == true);

    // 4. Guard Page Allocation
    uint64_t guard_vaddr = 0x00007FFF10000000ULL;
    REQUIRE(memory.allocate_guard_page(guard_vaddr) == true);

    const auto* guard_block = memory.get_block_at(guard_vaddr);
    REQUIRE(guard_block != nullptr);
    REQUIRE(guard_block->permissions == quin::memory::PagePermission::None);

    // 5. Cleanup with munmap
    REQUIRE(memory.munmap(vaddr, 8192) == true);
}

TEST_CASE("Multi-Threaded Guest Thread Manager & TLS Isolation", "[cpu][thread]") {
    quin::memory::GuestAddressSpace memory;
    quin::cpu::ThreadManager thread_mgr(memory);

    // Create Guest Thread
    quin::cpu::GuestThreadId tid1 = thread_mgr.create_thread("worker_1", 0x0000000000400080ULL, 0x100);
    REQUIRE(tid1 == 1);

    quin::cpu::ThreadContext* ctx1 = thread_mgr.get_thread(tid1);
    REQUIRE(ctx1 != nullptr);
    REQUIRE(ctx1->name == "worker_1");
    REQUIRE(ctx1->tls_base > 0);
    REQUIRE(ctx1->guard_page_addr > 0);

    // Verify TLS ABI self-pointer at offset 0
    uint64_t tls_self_ptr = 0;
    REQUIRE(memory.read_bytes(ctx1->tls_base, &tls_self_ptr, sizeof(tls_self_ptr)) == true);
    REQUIRE(tls_self_ptr == ctx1->tls_base);

    // Verify TLS Thread ID at offset 8
    uint64_t tls_tid = 0;
    REQUIRE(memory.read_bytes(ctx1->tls_base + 8, &tls_tid, sizeof(tls_tid)) == true);
    REQUIRE(tls_tid == tid1);

    // Join Thread
    REQUIRE(thread_mgr.join_thread(tid1) == true);
}

TEST_CASE("Execution Engine SYSCALL Trap Dispatch", "[cpu][engine]") {
    quin::memory::GuestAddressSpace memory;
    quin::kernel::LibKernel kernel;
    quin::cpu::ExecutionEngine engine(memory, kernel);

    // Allocate guest code segment
    uint64_t code_vaddr = 0x0000000000400000ULL;
    REQUIRE(memory.allocate(code_vaddr, 4096, quin::memory::PagePermission::ReadExecute) == true);

    // Write SYSCALL instruction (0x0F 0x05) followed by RET (0xC3)
    uint8_t syscall_code[] = { 0x0F, 0x05, 0xC3 };
    REQUIRE(memory.write_bytes(code_vaddr, syscall_code, sizeof(syscall_code)) == true);

    REQUIRE(engine.bootstrap(code_vaddr, 0x00007FFFF0000000ULL) == true);

    // Step 1: SYSCALL (0x0F 0x05)
    engine.step();
    REQUIRE(engine.get_registers().rip == code_vaddr + 2);

    // Step 2: RET (0xC3)
    engine.step();
    REQUIRE(engine.get_state() == quin::cpu::CpuState::Exited);
}

TEST_CASE("Native Exception Handler Initialization", "[cpu][exception]") {
    quin::cpu::ExceptionHandler::initialize();
    REQUIRE(quin::cpu::ExceptionHandler::has_last_crash() == false);
    quin::cpu::ExceptionHandler::shutdown();
}
