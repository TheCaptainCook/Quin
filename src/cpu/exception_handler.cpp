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
#endif

} // anonymous namespace

void ExceptionHandler::initialize() {
#if defined(_WIN32)
    if (!g_veh_handle) {
        g_veh_handle = AddVectoredExceptionHandler(1, vectored_exception_handler);
        QUIN_LOG_INFO("Vectored Exception Handler registered successfully.");
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
