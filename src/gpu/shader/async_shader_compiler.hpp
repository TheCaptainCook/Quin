#ifndef QUIN_GPU_SHADER_ASYNC_SHADER_COMPILER_HPP
#define QUIN_GPU_SHADER_ASYNC_SHADER_COMPILER_HPP

#include "gpu/shader/shader_recompiler.hpp"
#include "gpu/shader/shader_cache.hpp"
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace quin::gpu::shader {

struct AsyncCompileJob {
    std::vector<uint8_t> rdna2_bytecode;
    ShaderType stage;
    ShaderHash hash;
};

class AsyncShaderCompiler {
public:
    AsyncShaderCompiler(ShaderRecompiler& recompiler, ShaderCache& cache, size_t num_threads = 2);
    ~AsyncShaderCompiler();

    void start();
    void stop();

    void queue_job(const void* rdna2_bytes, size_t size, ShaderType stage);

    size_t get_pending_jobs_count() const;
    uint64_t get_completed_jobs_count() const { return m_completed_jobs; }
    size_t get_worker_threads_count() const { return m_worker_threads.size(); }

private:
    void worker_loop();

    ShaderRecompiler& m_recompiler;
    ShaderCache& m_cache;
    size_t m_num_threads;
    std::vector<std::thread> m_worker_threads;
    std::queue<AsyncCompileJob> m_job_queue;

    std::atomic<bool> m_running{false};
    std::atomic<uint64_t> m_completed_jobs{0};
    mutable std::mutex m_queue_mutex;
    std::condition_variable m_cv;
};

} // namespace quin::gpu::shader

#endif // QUIN_GPU_SHADER_ASYNC_SHADER_COMPILER_HPP
