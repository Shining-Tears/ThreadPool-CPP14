#include "CThreadPool.h"
#include <iostream>

CThreadPool::CThreadPool(size_t min_threads_num, size_t max_threads_num) : m_min_threadnum_(min_threads_num),
m_max_threadnum_(max_threads_num), m_stop_(false), m_exit_threadnum_(0), m_idle_threadnum_(min_threads_num), 
m_curr_threadnum_(min_threads_num) {
    m_manager_ = std::thread(&CThreadPool::manager, this);
    for (int i = 0; i < m_curr_threadnum_; i++) {
        std::thread t(&CThreadPool::worker, this);
        m_workers_.insert(std::make_pair<std::thread::id, std::thread>(t.get_id(), std::move(t)));
    }
}

CThreadPool::~CThreadPool() {
    m_stop_ = true;
    m_condition_.notify_all();
    for (auto& it: m_workers_) {
        std::thread& t = it.second;
        if (t.joinable()) t.join();
    }

    if (m_manager_.joinable()) m_manager_.join();
}

void CThreadPool::manager() {
    while (!m_stop_.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (m_idle_threadnum_.load() > m_curr_threadnum_.load() / 2 && 
        m_curr_threadnum_ > m_min_threadnum_) {
            m_exit_threadnum_.store(1);
            m_condition_.notify_one();
            std::unique_lock<std::mutex> lock_(m_ids_mutex_);
            for (const auto& exit_id : m_exit_ids_) {
                auto it = m_workers_.find(exit_id);
                if (it != m_workers_.end() && it->second.joinable()) {
                    it->second.join();
                    m_workers_.erase(it);
                }
            }
            m_exit_ids_.clear();
        } else if (m_idle_threadnum_.load() == 0 && m_curr_threadnum_.load() < m_max_threadnum_) {
            std::thread t(&CThreadPool::worker, this);
            m_workers_.insert(std::make_pair<std::thread::id, std::thread>(t.get_id(), std::move(t)));
            m_curr_threadnum_++;
            m_idle_threadnum_++;
        }
    }
}

void CThreadPool::worker() {
    while (!m_stop_.load()) {
        std::function<void()> task = nullptr;
        {
        std::unique_lock<std::mutex> lock_(m_queue_mutex_);
        while (!m_stop_.load() && m_tasks_.empty()){
            m_condition_.wait(lock_);
            if (m_exit_threadnum_.load() > 0) {
                std::unique_lock<std::mutex> lock_ids_(m_ids_mutex_);
                m_exit_ids_.emplace_back(std::this_thread::get_id());
                m_exit_threadnum_--;
                m_curr_threadnum_--;
                m_idle_threadnum_--;
            }
        }

        if (!m_tasks_.empty()) {
            task = std::move(m_tasks_.front());
            m_tasks_.pop();
        }
        }
        
        if (task) {
            m_idle_threadnum_--;
            task();
            m_idle_threadnum_++;
        }
    }
}

int calc(int x, int y)
{
    int res = x + y;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return res;
}

int main() {
    {
    int task_num = 100;
    std::cout << "initialize thread pool" << std::endl;

    CThreadPool thread_pool(4, 16);
    std::vector<std::future<int>> results;

    TICK(Runtime)
    for (int i = 0; i < task_num; i++) {
        results.emplace_back(thread_pool.addTask(calc, i, i));
    }

    for (auto&& res: results) {
        std::cout << res.get() << std::endl;
    }
    TOCK(Runtime)
    }
}