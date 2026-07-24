# Clean-Room Implementation & Ethical Development Policy

## 1. Objective
Quin is an open-source research and educational project developing a high-performance x86-64 translation layer and system emulator for the PlayStation 5 console architecture. To safeguard the project's legal integrity and ensure independent clean-room engineering, all contributors must strictly adhere to the policies set forth in this document.

---

## 2. Fundamental Rules

### 2.1 Zero Proprietary Code or SDK Usage
- **No Sony Interactive Entertainment (SIE) source code**, headers, private SDKs, leaked documentation, or proprietary tools may be used, referenced, or checked into this repository.
- Any contribution containing or derived from proprietary SDK headers or reverse-engineered confidential source code will be immediately rejected and removed.

### 2.2 No Cryptographic Key Materials or Firmware Dumps
- **No static keys, system keys, SAMU keys, HDCP keys, or console firmware binaries** may be distributed within this repository.
- All hardware key derivations or decryption keys must be provided by the end-user locally.

### 2.3 Independent Clean-Room Specification & Research
- All hardware and software behaviors implemented in Quin must be derived solely from:
  1. Public hardware vendor documentation (e.g., AMD RDNA2 ISA manuals, x86-64 SDM).
  2. Open-source specifications (e.g., Khronos Vulkan specs, FreeBSD system call interfaces).
  3. Independent black-box reverse engineering and behavior analysis of public homebrew binaries.

### 2.4 Respect for Third-Party Open Source Licensing
- Quin is licensed under the **BSD 3-Clause License**.
- Code borrowed from or inspired by GPL/AGPL-licensed emulators (or any non-compatible licenses) **must NOT** be copy-pasted or directly adapted into Quin to prevent license infection.
- Third-party libraries included in Quin (e.g., `spdlog`, `SDL3`, `Dear ImGui`, `Catch2`) must have permissive licenses (MIT, BSD, Apache 2.0, Zlib) and must retain their original copyright notices.

---

## 3. Contribution Verification
All pull requests must confirm:
- [x] Contributed code was authored independently by the contributor.
- [x] No copyrighted Sony SDKs, decompiled binary leaks, or non-permissive emulator code was consulted or copied during implementation.
