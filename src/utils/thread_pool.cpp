#include "thread_pool.hpp"

#include <condition_variable>
#include <queue>
#include <thread>
#include <vector>

namespace concurrency {

    ThreadPool::ThreadPool(size_t num_threads) {
        threads_.reserve(num_threads);

        for (size_t i = 0; i < num_threads; ++i) {
            threads_.emplace_back([this] {
                while (true) {
                    std::function<void()> activate_task_from_queue;

                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);

                        condition_variable_.wait(lock, [this]() { return !tasks_.empty() || stop_; });

                        if (stop_ && tasks_.empty()) {
                            return;
                        }

                        activate_task_from_queue = std::move(tasks_.front());
                        tasks_.pop();

                        ++active_tasks_;
                    }

                    activate_task_from_queue();

                    --active_tasks_;
                    completion_cv_.notify_all();
                }
            });
        };
    }

    ThreadPool::~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }

        condition_variable_.notify_all();

        for (auto& thread : threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    void ThreadPool::enqueue(std::function<void()> next_task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            tasks_.emplace(std::move(next_task));
        }

        condition_variable_.notify_one();
    }

    void ThreadPool::wait_all() {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        completion_cv_.wait(lock, [this]() { return tasks_.empty() && active_tasks_ == 0; });
    }
};  // namespace concurrency
