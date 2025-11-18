#pragma once

#include <functional>
#include <mutex>
#include <queue>

template <typename T>
struct ThreadTask
{
    std::function<void(T &)> callback;
    T result;
};

template <typename T>
class ThreadSafeQueue
{
public:
    void push(const T &value)
    {
        std::lock_guard<std::mutex> lock(mtx);
        q.push(value);
    }

    bool pop(T &out)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (q.empty())
            return false;
        out = std::move(q.front());
        q.pop();
        return true;
    }

private:
    std::queue<T> q;
    std::mutex mtx;
};
