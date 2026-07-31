#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount = 2) {
        m_workers.reserve(threadCount);
        for (size_t i = 0; i < threadCount; ++i) {
            m_workers.emplace_back([this] {
                while (true) {
                    std::function<void()> job;
                    {
                        std::unique_lock lock(m_mutex);
                        m_cv.wait(lock, [this] { return m_stop || !m_jobs.empty(); });
                        if (m_stop && m_jobs.empty()) return;
                        job = std::move(m_jobs.front());
                        m_jobs.pop();
                    }
                    job();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard lock(m_mutex);
            m_stop = true;
        }
        m_cv.notify_all();
        for (auto& t : m_workers) t.join();
    }

    // Returns a std::future so callers can optionally block/inspect the result.
    template<typename Fn, typename... Args>
    auto Submit(Fn&& fn, Args&&... args)
        -> std::future<std::invoke_result_t<Fn, Args...>>
    {
        using RetT = std::invoke_result_t<Fn, Args...>;
        auto task  = std::make_shared<std::packaged_task<RetT()>>(
            [f = std::forward<Fn>(fn), ...a = std::forward<Args>(args)]() mutable {
                return f(std::move(a)...);
            }
        );
        std::future<RetT> future = task->get_future();
        {
            std::lock_guard lock(m_mutex);
            m_jobs.emplace([task] { (*task)(); });
        }
        m_cv.notify_one();
        return future;
    }

private:
    std::vector<std::thread>          m_workers;
    std::queue<std::function<void()>> m_jobs;
    std::mutex                        m_mutex;
    std::condition_variable           m_cv;
    std::atomic<bool>                 m_stop{false};
};
