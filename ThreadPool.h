//
// Created by ricardo on 4/2/23.
//

#ifndef RTSPSERVER_THREADPOOL_H
#define RTSPSERVER_THREADPOOL_H

#include <functional>
#include <queue>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    void enqueue(const std::function<void()>& task);

private:
    void worker();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex tasks_mutex_;
    std::condition_variable tasks_cv_;
    std::atomic<bool> stop_;
};

#endif //RTSPSERVER_THREADPOOL_H
