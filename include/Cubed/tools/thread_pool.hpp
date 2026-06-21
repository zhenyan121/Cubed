#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
namespace Cubed {
class ThreadPool {
private:
    std::vector<std::jthread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mtx;
    std::condition_variable_any m_cv;
    std::atomic<bool> m_stopping{false};
    std::atomic<size_t> m_thread_sum{0};

public:
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;
    explicit ThreadPool(size_t thread_sum) : m_thread_sum(thread_sum) {
        for (size_t i = 0; i < thread_sum; i++) {
            m_workers.emplace_back([this](std::stop_token stoken) {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(m_mtx);
                        m_cv.wait(lock, stoken,
                                  [this, stoken] { return !m_tasks.empty(); });
                        if (stoken.stop_requested() && m_tasks.empty()) {
                            return;
                        }
                        task = std::move(m_tasks.front());
                        m_tasks.pop();
                    }
                    task();
                }
            });
        }
    }
    ~ThreadPool() {
        m_stopping = true;
        for (auto& w : m_workers) {
            w.request_stop();
        }

        m_cv.notify_all();

        for (auto& w : m_workers) {
            if (w.joinable()) {
                w.join();
            }
        }
    }
    template <typename F> auto enqueue(F&& f) {

        using R = std::invoke_result_t<F>;

        auto task =
            std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
        auto fut = task->get_future();

        {
            std::lock_guard lock(m_mtx);
            if (m_stopping)
                throw std::runtime_error("thread pool stopped");
            m_tasks.emplace([task] { (*task)(); });
        }
        m_cv.notify_one();
        return fut;
    }
    size_t thread_sum() const { return m_thread_sum.load(); }
};

template <std::random_access_iterator Iter, typename F>
void parallel_do(ThreadPool& pool, Iter first, Iter last, size_t max_threads,
                 F&& f) {
    max_threads = std::max<size_t>(1, max_threads);
    max_threads = std::min(max_threads, pool.thread_sum());
    std::decay_t<F> fn(std::forward<F>(f));
    size_t length = std::distance(first, last);
    if (!length) {
        return;
    }

    constexpr size_t MIN_PER_THREAD = 25;
    size_t num_blocks =
        std::min(max_threads, (length + MIN_PER_THREAD - 1) / MIN_PER_THREAD);
    num_blocks = std::max<size_t>(1, num_blocks);
    size_t block_size = (length + num_blocks - 1) / num_blocks;

    std::vector<std::future<void>> futures;
    futures.reserve(num_blocks - 1);
    Iter block_start = first;
    for (size_t i = 0; i < num_blocks - 1; ++i) {
        Iter block_end = block_start;
        auto remain = std::distance(block_start, last);
        std::advance(block_end, std::min<size_t>(block_size, remain));

        futures.emplace_back(pool.enqueue([block_start, block_end, &fn]() {
            for (auto it = block_start; it != block_end; ++it) {
                fn(*it);
            }
        }));

        block_start = block_end;
    }
    for (auto it = block_start; it != last; ++it) {
        fn(*it);
    }

    for (auto& fut : futures) {
        fut.get();
    }
};

} // namespace Cubed
