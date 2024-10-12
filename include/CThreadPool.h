#pragma once
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <utility>

#include <map>
#include <queue>

#define TICK(x) auto bench_##x = std::chrono::steady_clock::now();
#define TOCK(x) printf("%s: %lfms\n", #x, 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - bench_##x).count());

class CThreadPool {
    std::thread m_manager_;
    std::map<std::thread::id, std::thread> m_workers_;
    std::vector<std::thread::id> m_exit_ids_;

    int m_max_threadnum_;
    int m_min_threadnum_;

    std::mutex m_ids_mutex_;
    std::mutex m_queue_mutex_;
    std::condition_variable m_condition_;

    std::atomic<bool> m_stop_;
    std::atomic<int> m_curr_threadnum_;
    std::atomic<int> m_idle_threadnum_;
    std::atomic<int> m_exit_threadnum_;

    std::queue<std::function<void()>> m_tasks_;

    void manager();
    void worker();

public:
    CThreadPool(size_t min_threads_num = 2, size_t max_threads_num = std::thread::hardware_concurrency());
    ~CThreadPool();

    template<typename F, typename... Args> // ...模板参数包
    auto addTask(F&& f, Args&&... args){   // ...函数参数包
        using returnType = typename std::result_of<F(Args...)>::type;    // ...参数包展开
        auto task = std::make_shared<std::packaged_task<returnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...    // ...参数包展开
        ));
        {
            std::unique_lock<std::mutex> lock_(m_queue_mutex_);
            m_tasks_.emplace([task](){(*task)();});
        }
        m_condition_.notify_one();

        return task->get_future(); //type std::future<typename std::result_of<F(Args...)>::type>
    }    
};