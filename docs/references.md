# Primary Technical References & Specifications

This document catalogs the authoritative, publicly accessible technical references used in the design and engineering of the **Quin** emulator.

---

## 1. Processor & GPU Hardware Architecture

### AMD RDNA2 GPU Instruction Set Architecture (ISA)
- **Document**: AMD RDNA 2 Shader Instruction Set Architecture (ISA) Reference Manual
- **Publisher**: Advanced Micro Devices, Inc. (AMD)
- **Scope**: Instruction encoding, vector ALU execution, scalar registers, resource descriptors, and wave execution model for RDNA2 (GFX10.3) family GPUs.
- **Link**: [AMD RDNA2 ISA Reference Guide](https://gpuopen.com/)

### x86-64 Architecture Software Developer Manuals
- **Document**: Intel 64 and IA-32 Architectures Software Developer's Manuals / AMD64 Architecture Programmer's Manual
- **Scope**: General-purpose instructions, AVX/AVX2 SIMD extensions, page table structures, memory ordering, and thread context switching.

---

## 2. Graphics API Specifications

### Khronos Vulkan 1.3 Specification
- **Document**: Vulkan 1.3 Core API Specification
- **Publisher**: Khronos Group
- **Scope**: Render passes, pipeline layout mapping, descriptor set management, SPIR-V intermediate representation, dynamic state, and synchronization primitives.
- **Link**: [Khronos Vulkan Registry](https://registry.khronos.org/vulkan/)

---

## 3. Operating System & Binary Format Specifications

### FreeBSD Kernel & System Call Architecture
- **Document**: FreeBSD 12 / 13 / 14 Syscall Reference & Kernel Source (`sys/sys/syscall.h`)
- **Scope**: Posix memory mapping (`mmap`, `mprotect`, `munmap`), thread creation (`thr_new`, `thr_exit`), IPC, file descriptors, and signal handling.
- **Link**: [FreeBSD Documentation Project](https://www.freebsd.org/doc/)

### Executable and Linkable Format (ELF) Standard
- **Document**: System V Application Binary Interface - DRAFT / Executable and Linking Format (ELF) Specification
- **Scope**: ELF64 headers, program headers (`PT_LOAD`, `PT_DYNAMIC`, `PT_TLS`), section headers, dynamic relocation tables (`RELA`), symbol resolution, and Procedure Linkage Table (PLT) mechanics.

---

## 4. Package & Container Formats

### Published Public Format Analysis
- **SELF / EBOOT Structure**: Signed Executable and Linkable Format header wrapping standard 64-bit ELF segments with metadata headers.
- **PKG Format**: Encapsulated package structure containing PFS (PlayStation Filesystem) volume headers, asset files, and metadata.
