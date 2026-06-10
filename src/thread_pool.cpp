#include <thread_pool.hpp>

#include <mutex>
#include <stdexcept>

ThreadPool::ThreadPool(size_t num_threads) : stop_(false)
{
    if (num_threads == 0)
    {
        throw std::invalid_argument("Thread pool need atleast one thread");
    }

    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i)
    {
        workers_.emplace_back(&ThreadPool::worker_loop, this);
    }
}

ThreadPool::~ThreadPool()
{
    stop_ = true;
    condition_.notify_all();

    for (auto &w : workers_)
    {
        if (w.joinable())
            w.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.push(std::move(task));
    }

    condition_.notify_one();
}

void ThreadPool::worker_loop()
{
    while (true)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this]
                            { return stop_.load() || !task_queue_.empty(); });

            if (stop_.load() && task_queue_.empty())
                return;

            task = std::move(task_queue_.front());
            task_queue_.pop();
        }

        task();
    }
}