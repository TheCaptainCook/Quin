#ifndef QUIN_CPU_EXCEPTION_HANDLER_HPP
#define QUIN_CPU_EXCEPTION_HANDLER_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace quin::cpu {

enum class ExceptionType {
    AccessViolation,
    IllegalInstruction,
    StackOverflow,
    Breakpoint,
    Unknown
};

struct CrashReport {
    ExceptionType type{ExceptionType::Unknown};
    uint64_t fault_address{0};
    uint64_t instruction_pointer{0};
    std::string description;
    std::vector<uint64_t> call_stack;
};

class ExceptionHandler {
public:
    static void initialize();
    static void shutdown();

    static bool has_last_crash();
    static CrashReport get_last_crash();
    static void clear_last_crash();

    static void set_crash_callback(void(*callback)(const CrashReport&));
};

} // namespace quin::cpu

#endif // QUIN_CPU_EXCEPTION_HANDLER_HPP
