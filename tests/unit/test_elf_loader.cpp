#include <catch2/catch_test_macros.hpp>
#include "loader/self_parser.hpp"
#include "loader/elf_loader.hpp"
#include "memory/address_space.hpp"
#include "kernel/libkernel.hpp"
#include "cpu/execution_engine.hpp"

#include <vector>
#include <cstring>

TEST_CASE("64-bit ELF Header Parsing and Segment Extraction", "[loader][elf]") {
    using namespace quin::loader;

    // Construct a minimal synthetic 64-bit ELF in memory (256 bytes)
    std::vector<uint8_t> elf_buffer(256, 0);

    auto* ehdr = reinterpret_cast<Elf64_Ehdr*>(elf_buffer.data());
    ehdr->e_ident[0] = ELF_MAG0;
    ehdr->e_ident[1] = ELF_MAG1;
    ehdr->e_ident[2] = ELF_MAG2;
    ehdr->e_ident[3] = ELF_MAG3;
    ehdr->e_ident[4] = ELFCLASS64;
    ehdr->e_ident[5] = ELFDATA2LSB;
    ehdr->e_type = ET_EXEC;
    ehdr->e_machine = EM_X86_64;
    ehdr->e_entry = 0x0000000000400080ULL; // Entry point at offset 0x80 (128)
    ehdr->e_phoff = sizeof(Elf64_Ehdr);
    ehdr->e_phnum = 1;

    auto* phdr = reinterpret_cast<Elf64_Phdr*>(elf_buffer.data() + sizeof(Elf64_Ehdr));
    phdr->p_type = PT_LOAD;
    phdr->p_flags = PF_R | PF_X; // Read + Execute
    phdr->p_offset = 0;
    phdr->p_vaddr = 0x0000000000400000ULL;
    phdr->p_filesz = elf_buffer.size();
    phdr->p_memsz = 4096;

    // Write payload (NOP NOP RET = 0x90 0x90 0xC3) at entry point offset 0x80
    uint8_t* payload = elf_buffer.data() + 0x80;
    payload[0] = 0x90; // NOP
    payload[1] = 0x90; // NOP
    payload[2] = 0xC3; // RET

    // 1. Parse ELF Header
    ParsedElf parsed = SelfParser::parse_buffer(elf_buffer);
    REQUIRE(parsed.valid == true);
    REQUIRE(parsed.is_self == false);
    REQUIRE(parsed.entry_point == 0x0000000000400080ULL);
    REQUIRE(parsed.program_headers.size() == 1);
    REQUIRE(parsed.program_headers[0].p_type == PT_LOAD);

    // 2. Load into Guest Address Space
    quin::memory::GuestAddressSpace address_space;
    ElfLoader loader(address_space);
    LoadResult result = loader.load(parsed);

    REQUIRE(result.success == true);
    REQUIRE(result.loaded_segments_count == 1);
    REQUIRE(result.entry_point == 0x0000000000400080ULL);
    REQUIRE(result.stack_top > 0);

    // 3. Verify Memory Mapping Payload
    uint8_t read_payload[3]{};
    bool read_success = address_space.read_bytes(0x0000000000400080ULL, read_payload, 3);
    REQUIRE(read_success == true);
    REQUIRE(read_payload[0] == 0x90);
    REQUIRE(read_payload[1] == 0x90);
    REQUIRE(read_payload[2] == 0xC3);

    // 4. Test LibKernel Symbol Dispatch & Harness Execution
    quin::kernel::LibKernel kernel;
    REQUIRE(kernel.has_symbol("sceKernelExitProcess") == true);
    int64_t ret_code = kernel.dispatch_symbol("sceKernelExitProcess", 0);
    REQUIRE(ret_code == 0);

    // 5. Test CPU Execution Harness Step
    quin::cpu::ExecutionEngine engine(address_space, kernel);
    bool boot_ok = engine.bootstrap(result.entry_point, result.stack_top);
    REQUIRE(boot_ok == true);
    REQUIRE(engine.get_state() == quin::cpu::CpuState::Ready);

    engine.step(); // Execute NOP (0x90)
    REQUIRE(engine.get_registers().rip == 0x0000000000400081ULL);

    engine.step(); // Execute NOP (0x90)
    REQUIRE(engine.get_registers().rip == 0x0000000000400082ULL);

    engine.step(); // Execute RET (0xC3)
    REQUIRE(engine.get_state() == quin::cpu::CpuState::Exited);
}
