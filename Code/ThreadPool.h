#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class ThreadPool
{
public:
    ThreadPool(size_t threads = std::thread::hardware_concurrency())
    {
        stopFlag = false;
        for (size_t i = 0; i < threads; ++i)
        {
            workers.emplace_back([this]()
                                 { workerLoop(); });
        }
    }

    ~ThreadPool()
    {
        stop();
    }

    // Schedule a job
    template <typename F>
    void post(F &&func)
    {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            jobs.emplace(std::forward<F>(func));
        }
        cv.notify_one();
    }

    // Graceful shutdown
    void stop()
    {
        if (stopFlag)
            return;

        stopFlag = true;
        cv.notify_all();
        for (auto &t : workers)
            if (t.joinable())
                t.join();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> jobs;

    std::mutex queueMutex;
    std::condition_variable cv;
    std::atomic<bool> stopFlag = false;

    void workerLoop()
    {
        while (true)
        {
            std::function<void()> job;

            {
                std::unique_lock<std::mutex> lock(queueMutex);
                cv.wait(lock, [this]()
                        { return stopFlag || !jobs.empty(); });

                if (stopFlag && jobs.empty())
                    return;

                job = std::move(jobs.front());
                jobs.pop();
            }

            job();
        }
    }
};
