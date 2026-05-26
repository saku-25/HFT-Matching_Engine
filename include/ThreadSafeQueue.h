#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional> // We can use this again!

template <typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue;
    std::mutex mtx;
    std::condition_variable cv;
    bool killSwitch = false; 

public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;

    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            queue.push(item);
        }
        cv.notify_one();
    }

    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mtx);
        
        cv.wait(lock, [this]() { 
            return !queue.empty() || killSwitch; 
        });

        if (killSwitch && queue.empty()) {
            return std::nullopt; // Safely return nothing
        }

        T item = queue.front();
        queue.pop();
        return item;
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            killSwitch = true;
        }
        cv.notify_all(); 
    }
};