#include "Base/ThreadPool/ThreadPool.h"

#include <algorithm>
#include <cstdlib>

namespace base {

namespace {

int requestedWorkers()
{
    if (const char *single = std::getenv("NEON_SINGLE_THREAD")) {
        (void)single;
        return 0;
    }
    if (const char *value = std::getenv("NEON_RENDER_THREADS")) {
        const int requested = std::atoi(value);
        return std::clamp(requested, 0, 8);
    }
    const unsigned int hardware = std::thread::hardware_concurrency();
    const int usable = hardware > 1 ? int(hardware - 1) : 1;
    return std::clamp(usable, 1, 3);
}

} // namespace

ThreadPool::ThreadPool() : workerTarget_(requestedWorkers()) {}

ThreadPool::~ThreadPool()
{
    shutdown();
}

void ThreadPool::start()
{
    if (started_ || stopping_ || workerTarget_ <= 0) {
        return;
    }
    started_ = true;
    workers_.reserve(size_t(workerTarget_));
    for (int i = 0; i < workerTarget_; ++i) {
        workers_.emplace_back([this] { workerLoop(0); });
    }
}

void ThreadPool::parallelFor(int taskCount, const std::function<void(int)> &task)
{
    if (taskCount <= 0) {
        return;
    }
    if (taskCount == 1 || workerTarget_ <= 0) {
        for (int i = 0; i < taskCount; ++i) {
            task(i);
        }
        return;
    }

    start();
    if (workers_.empty()) {
        for (int i = 0; i < taskCount; ++i) {
            task(i);
        }
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        job_ = &task;
        jobCount_ = taskCount;
        nextTask_.store(0, std::memory_order_relaxed);
        completed_.store(0, std::memory_order_relaxed);
        ++generation_;
    }
    wake_.notify_all();

    // 主线程也参与，避免只等不干。
    runTasks(task, taskCount);

    std::unique_lock<std::mutex> lock(mutex_);
    finished_.wait(lock, [this, taskCount] {
        return completed_.load(std::memory_order_acquire) >= taskCount;
    });
    job_ = nullptr;
    jobCount_ = 0;
}

void ThreadPool::runTasks(const std::function<void(int)> &task, int taskCount)
{
    for (;;) {
        const int index = nextTask_.fetch_add(1, std::memory_order_relaxed);
        if (index >= taskCount) {
            return;
        }
        task(index);
        if (completed_.fetch_add(1, std::memory_order_acq_rel) + 1 == taskCount) {
            std::lock_guard<std::mutex> lock(mutex_);
            finished_.notify_all();
        }
    }
}

void ThreadPool::workerLoop(unsigned int initialGeneration)
{
    unsigned int seen = initialGeneration;
    for (;;) {
        const std::function<void(int)> *job = nullptr;
        int count = 0;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            wake_.wait(lock, [this, &seen] {
                return stopping_ || generation_ != seen;
            });
            if (stopping_) {
                return;
            }
            seen = generation_;
            job = job_;
            count = jobCount_;
        }
        if (job != nullptr) {
            runTasks(*job, count);
        }
    }
}

void ThreadPool::shutdown()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    wake_.notify_all();
    for (std::thread &worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
    started_ = false;
}

} // namespace base
