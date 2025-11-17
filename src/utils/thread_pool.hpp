#ifndef QUANT_FORGE_THREAD_POOL_HPP
#define QUANT_FORGE_THREAD_POOL_HPP

#pragma once

#include <atomic>
#include <condition_variable>
#include <queue>
#include <thread>
#include <vector>

namespace concurrency {
    class ThreadPool {
       public:
        ThreadPool(size_t num_threads);

        ~ThreadPool();
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;

        void enqueue(std::function<void()> next_task);
        void wait_all();

       private:
        std::vector<std::thread> threads_;  // reserve
        std::queue<std::function<void()> > tasks_;
        std::mutex queue_mutex_;
        std::condition_variable condition_variable_;
        std::atomic<bool> stop_ = false;
        std::atomic<size_t> active_tasks_ = 0;
        std::condition_variable completion_cv_;
    };
}  // namespace concurrency

#endif
