#ifndef QUIN_COMPAT_COMPAT_TRIAGE_HPP
#define QUIN_COMPAT_COMPAT_TRIAGE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace quin::compat {

struct MissingSymbolInfo {
    std::string symbol_name;
    std::string module_name;
    uint64_t call_count{0};
};

struct RegressionTestResult {
    bool all_passed{true};
    size_t tests_run{0};
    size_t tests_passed{0};
    size_t tests_failed{0};
    std::vector<std::string> log_output;
};

class CompatTriage {
public:
    CompatTriage() = default;

    void log_missing_symbol(const std::string& symbol, const std::string& module = "unknown");
    void log_missing_syscall(uint64_t sys_num);

    std::vector<MissingSymbolInfo> get_top_missing_symbols(size_t limit = 10) const;
    RegressionTestResult run_regression_suite();

    uint64_t get_total_unimplemented_calls() const { return m_total_unimplemented; }

private:
    std::unordered_map<std::string, MissingSymbolInfo> m_missing_symbols;
    std::unordered_map<uint64_t, uint64_t> m_missing_syscalls;
    uint64_t m_total_unimplemented{0};
    mutable std::mutex m_mutex;
};

} // namespace quin::compat

#endif // QUIN_COMPAT_COMPAT_TRIAGE_HPP
