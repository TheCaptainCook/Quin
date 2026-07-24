#ifndef QUIN_CPU_THREAD_MANAGER_HPP
#define QUIN_CPU_THREAD_MANAGER_HPP

#include "cpu/thread_context.hpp"
#include "memory/address_space.hpp"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace quin::cpu {

struct ThreadInfo {
    GuestThreadId id;
    std::string name;
    ThreadState state;
    uint64_t rip;
    uint64_t rsp;
    uint64_t stack_base;
    uint64_t tls_base;
};

class ThreadManager {
public:
    explicit ThreadManager(quin::memory::GuestAddressSpace& memory);
    ~ThreadManager();

    // Prevent copying
    ThreadManager(const ThreadManager&) = delete;
    ThreadManager& operator=(const ThreadManager&) = delete;

    GuestThreadId create_thread(const std::string& name, uint64_t entry_point, uint64_t arg, size_t stack_size = 2 * 1024 * 1024);
    bool join_thread(GuestThreadId id);
    bool terminate_thread(GuestThreadId id);

    ThreadContext* get_thread(GuestThreadId id);
    std::vector<ThreadInfo> get_active_threads_info() const;
    size_t get_active_thread_count() const;

private:
    uint64_t allocate_tls_block(GuestThreadId id);

    quin::memory::GuestAddressSpace& m_memory;
    std::unordered_map<GuestThreadId, std::unique_ptr<ThreadContext>> m_threads;
    std::atomic<GuestThreadId> m_next_thread_id{1};
    mutable std::mutex m_threads_mutex;
    uint64_t m_next_stack_vaddr{0x00007FFF00000000ULL};
    uint64_t m_next_tls_vaddr{0x00007FFE00000000ULL};
};

} // namespace quin::cpu

#endif // QUIN_CPU_THREAD_MANAGER_HPP
