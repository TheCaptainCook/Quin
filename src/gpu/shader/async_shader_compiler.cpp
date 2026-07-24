#include "gpu/shader/async_shader_compiler.hpp"
#include "core/logging.hpp"

namespace quin::gpu::shader {

AsyncShaderCompiler::AsyncShaderCompiler(ShaderRecompiler& recompiler, ShaderCache& cache, size_t num_threads)
    : m_recompiler(recompiler), m_cache(cache), m_num_threads(num_threads) {}

AsyncShaderCompiler::~AsyncShaderCompiler() {
    stop();
}

void AsyncShaderCompiler::start() {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    if (m_running) return;

    m_running = true;
    for (size_t i = 0; i < m_num_threads; ++i) {
        m_worker_threads.emplace_back(&AsyncShaderCompiler::worker_loop, this);
    }
    QUIN_LOG_INFO("AsyncShaderCompiler: Started {} background worker threads.", m_num_threads);
}

void AsyncShaderCompiler::stop() {
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        if (!m_running) return;
        m_running = false;
    }
    m_cv.notify_all();

    for (auto& t : m_worker_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    m_worker_threads.clear();
    QUIN_LOG_INFO("AsyncShaderCompiler: Stopped all worker threads cleanly.");
}

void AsyncShaderCompiler::queue_job(const void* rdna2_bytes, size_t size, ShaderType stage) {
    if (!rdna2_bytes || size == 0) return;

    ShaderHash hash = ShaderRecompiler::compute_hash(rdna2_bytes, size);
    if (m_cache.contains(hash)) {
        return; // Already cached
    }

    AsyncCompileJob job{};
    const uint8_t* ptr = static_cast<const uint8_t*>(rdna2_bytes);
    job.rdna2_bytecode.assign(ptr, ptr + size);
    job.stage = stage;
    job.hash = hash;

    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_job_queue.push(job);
    }
    m_cv.notify_one();
}

void AsyncShaderCompiler::worker_loop() {
    while (m_running) {
        AsyncCompileJob job;
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_cv.wait(lock, [this]() {
                return !m_running || !m_job_queue.empty();
            });

            if (!m_running && m_job_queue.empty()) {
                break;
            }

            job = std::move(m_job_queue.front());
            m_job_queue.pop();
        }

        auto res = m_recompiler.recompile(job.rdna2_bytecode.data(), job.rdna2_bytecode.size(), job.stage);
        if (res.success) {
            m_cache.put(res.shader);
            m_completed_jobs++;
        }
    }
}

size_t AsyncShaderCompiler::get_pending_jobs_count() const {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    return m_job_queue.size();
}

} // namespace quin::gpu::shader
