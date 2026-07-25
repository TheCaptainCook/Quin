#include "cpu/exception_handler.hpp"
#include "core/logging.hpp"
#include <mutex>
#include <sstream>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <signal.h>
#include <ucontext.h>
#include <cstring>
#endif

namespace quin::cpu {

namespace {

std::mutex g_crash_mutex;
bool g_has_crash{false};
CrashReport g_last_crash{};
void(*g_crash_callback)(const CrashReport&){nullptr};

#if defined(_WIN32)
PVOID g_veh_handle{nullptr};

LONG WINAPI vectored_exception_handler(PEXCEPTION_POINTERS exception_info) {
    if (!exception_info || !exception_info->ExceptionRecord) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    DWORD code = exception_info->ExceptionRecord->ExceptionCode;

    // Filter exceptions we care about
    if (code != EXCEPTION_ACCESS_VIOLATION &&
        code != EXCEPTION_ILLEGAL_INSTRUCTION &&
        code != EXCEPTION_STACK_OVERFLOW &&
        code != EXCEPTION_BREAKPOINT) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    std::lock_guard<std::mutex> lock(g_crash_mutex);
    g_has_crash = true;

    CrashReport report{};
    report.instruction_pointer = reinterpret_cast<uint64_t>(exception_info->ExceptionRecord->ExceptionAddress);

    if (code == EXCEPTION_ACCESS_VIOLATION) {
        report.type = ExceptionType::AccessViolation;
        report.fault_address = static_cast<uint64_t>(exception_info->ExceptionRecord->ExceptionInformation[1]);
        std::ostringstream ss;
        ss << "Access Violation (Memory Fault at 0x" << std::hex << report.fault_address << ")";
        report.description = ss.str();
    } else if (code == EXCEPTION_ILLEGAL_INSTRUCTION) {
        report.type = ExceptionType::IllegalInstruction;
        report.description = "Illegal Instruction Exception";
    } else if (code == EXCEPTION_STACK_OVERFLOW) {
        report.type = ExceptionType::StackOverflow;
        report.description = "Stack Overflow (Guard Page Violation)";
    } else if (code == EXCEPTION_BREAKPOINT) {
        report.type = ExceptionType::Breakpoint;
        report.description = "Breakpoint Trap";
    }

    report.call_stack.push_back(report.instruction_pointer);

    QUIN_LOG_ERROR("CRASH INTERCEPTED — Type: {} | RIP: 0x{:016X} | FaultAddr: 0x{:016X} | Description: {}",
                   static_cast<int>(report.type), report.instruction_pointer, report.fault_address, report.description);

    g_last_crash = report;
    if (g_crash_callback) {
        g_crash_callback(report);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

#else
// ===== POSIX Signal Handlers (Linux / macOS) =====

struct sigaction g_old_sigsegv{};
struct sigaction g_old_sigill{};
struct sigaction g_old_sigbus{};
struct sigaction g_old_sigtrap{};
bool g_signal_handlers_installed{false};

void posix_signal_handler(int sig, siginfo_t* info, void* ucontext_raw) {
    std::lock_guard<std::mutex> lock(g_crash_mutex);
    g_has_crash = true;

    CrashReport report{};

    // Extract instruction pointer from ucontext
    ucontext_t* uc = static_cast<ucontext_t*>(ucontext_raw);
    if (uc) {
#if defined(__linux__) && defined(__x86_64__)
        report.instruction_pointer = static_cast<uint64_t>(uc->uc_mcontext.gregs[REG_RIP]);
#elif defined(__APPLE__) && defined(__x86_64__)
        report.instruction_pointer = static_cast<uint64_t>(uc->uc_mcontext->__ss.__rip);
#elif defined(__APPLE__) && defined(__aarch64__)
        report.instruction_pointer = static_cast<uint64_t>(uc->uc_mcontext->__ss.__pc);
#elif defined(__linux__) && defined(__aarch64__)
        report.instruction_pointer = static_cast<uint64_t>(uc->uc_mcontext.pc);
#else
        report.instruction_pointer = 0;
#endif
    }

    // Extract fault address from siginfo
    if (info) {
        report.fault_address = reinterpret_cast<uint64_t>(info->si_addr);
    }

    switch (sig) {
        case SIGSEGV:
            report.type = ExceptionType::AccessViolation;
            {
                std::ostringstream ss;
                ss << "SIGSEGV — Segmentation Fault at 0x" << std::hex << report.fault_address;
                report.description = ss.str();
            }
            break;
        case SIGILL:
            report.type = ExceptionType::IllegalInstruction;
            report.description = "SIGILL — Illegal Instruction";
            break;
        case SIGBUS:
            report.type = ExceptionType::AccessViolation;
            {
                std::ostringstream ss;
                ss << "SIGBUS — Bus Error at 0x" << std::hex << report.fault_address;
                report.description = ss.str();
            }
            break;
        case SIGTRAP:
            report.type = ExceptionType::Breakpoint;
            report.description = "SIGTRAP — Breakpoint/Trap";
            break;
        default:
            report.type = ExceptionType::Unknown;
            report.description = "Unknown Signal";
            break;
    }

    report.call_stack.push_back(report.instruction_pointer);

    QUIN_LOG_ERROR("CRASH INTERCEPTED — Signal: {} | RIP: 0x{:016X} | FaultAddr: 0x{:016X} | Description: {}",
                   sig, report.instruction_pointer, report.fault_address, report.description);

    g_last_crash = report;
    if (g_crash_callback) {
        g_crash_callback(report);
    }

    // Re-raise the signal with the old handler so the process can terminate properly
    struct sigaction* old_action = nullptr;
    switch (sig) {
        case SIGSEGV: old_action = &g_old_sigsegv; break;
        case SIGILL:  old_action = &g_old_sigill; break;
        case SIGBUS:  old_action = &g_old_sigbus; break;
        case SIGTRAP: old_action = &g_old_sigtrap; break;
        default: break;
    }

    if (old_action && old_action->sa_handler != SIG_DFL && old_action->sa_handler != SIG_IGN) {
        old_action->sa_sigaction(sig, info, ucontext_raw);
    } else {
        // Restore default and re-raise
        signal(sig, SIG_DFL);
        raise(sig);
    }
}

#endif

} // anonymous namespace

void ExceptionHandler::initialize() {
#if defined(_WIN32)
    if (!g_veh_handle) {
        g_veh_handle = AddVectoredExceptionHandler(1, vectored_exception_handler);
        QUIN_LOG_INFO("Vectored Exception Handler registered successfully.");
    }
#else
    if (!g_signal_handlers_installed) {
        struct sigaction sa{};
        sa.sa_sigaction = posix_signal_handler;
        sa.sa_flags = SA_SIGINFO | SA_RESTART;
        sigemptyset(&sa.sa_mask);

        sigaction(SIGSEGV, &sa, &g_old_sigsegv);
        sigaction(SIGILL,  &sa, &g_old_sigill);
        sigaction(SIGBUS,  &sa, &g_old_sigbus);
        sigaction(SIGTRAP, &sa, &g_old_sigtrap);

        g_signal_handlers_installed = true;
        QUIN_LOG_INFO("POSIX Signal Handlers registered (SIGSEGV, SIGILL, SIGBUS, SIGTRAP).");
    }
#endif
}

void ExceptionHandler::shutdown() {
#if defined(_WIN32)
    if (g_veh_handle) {
        RemoveVectoredExceptionHandler(g_veh_handle);
        g_veh_handle = nullptr;
        QUIN_LOG_INFO("Vectored Exception Handler removed.");
    }
#else
    if (g_signal_handlers_installed) {
        sigaction(SIGSEGV, &g_old_sigsegv, nullptr);
        sigaction(SIGILL,  &g_old_sigill, nullptr);
        sigaction(SIGBUS,  &g_old_sigbus, nullptr);
        sigaction(SIGTRAP, &g_old_sigtrap, nullptr);
        g_signal_handlers_installed = false;
        QUIN_LOG_INFO("POSIX Signal Handlers restored to defaults.");
    }
#endif
}

bool ExceptionHandler::has_last_crash() {
    std::lock_guard<std::mutex> lock(g_crash_mutex);
    return g_has_crash;
}

CrashReport ExceptionHandler::get_last_crash() {
    std::lock_guard<std::mutex> lock(g_crash_mutex);
    return g_last_crash;
}

void ExceptionHandler::clear_last_crash() {
    std::lock_guard<std::mutex> lock(g_crash_mutex);
    g_has_crash = false;
    g_last_crash = CrashReport{};
}

void ExceptionHandler::set_crash_callback(void(*callback)(const CrashReport&)) {
    std::lock_guard<std::mutex> lock(g_crash_mutex);
    g_crash_callback = callback;
}

} // namespace quin::cpu
