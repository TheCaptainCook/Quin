#ifndef QUIN_KERNEL_LIBKERNEL_HPP
#define QUIN_KERNEL_LIBKERNEL_HPP

#include <string>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace quin::kernel {

using StubHandler = std::function<int64_t(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)>;

class LibKernel {
public:
    LibKernel();

    void register_stub(const std::string& symbol_name, StubHandler handler);
    bool has_symbol(const std::string& symbol_name) const;
    int64_t dispatch_symbol(const std::string& symbol_name, uint64_t a1 = 0, uint64_t a2 = 0, uint64_t a3 = 0, uint64_t a4 = 0);

    const std::unordered_map<std::string, StubHandler>& get_stubs() const { return m_stubs; }

private:
    void register_default_stubs();
    std::unordered_map<std::string, StubHandler> m_stubs;
};

} // namespace quin::kernel

#endif // QUIN_KERNEL_LIBKERNEL_HPP
