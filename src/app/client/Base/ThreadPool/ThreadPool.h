#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace base {

// 帧内并行任务池：主线程自己也参与执行，所有任务完成后 parallelFor 才返回。
//
// 约束：
// - 只允许同一个线程（主线程）调用 parallelFor；不支持嵌套。
// - 任务之间不得写同一份数据；每个任务只应操作自己独占的缓冲/对象。
// - 线程数由硬件并发决定（默认 min(核数-1, 3)），可用环境变量覆盖：
//     NEON_RENDER_THREADS=N   工作线程数（0 = 全部顺序执行）
//     NEON_SINGLE_THREAD=1    强制单线程（排查/对比用）
class ThreadPool {
public:
    ThreadPool();
    ~ThreadPool();

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    int workerCount() const { return int(workers_.size()); }
    bool enabled() const { return workerTarget_ > 0; }

    // 把 [0, taskCount) 分给主线程 + 工作线程；阻塞到全部任务完成。
    // taskCount <= 1 或池被禁用时直接顺序执行。
    void parallelFor(int taskCount, const std::function<void(int)> &task);

    // 启动工作线程（幂等）。Base::initialize() 在 QApplication 就绪后调用。
    void start();

    // 停止并回收线程；可重复调用（之后池不再启动）。
    void shutdown();

private:
    void runTasks(const std::function<void(int)> &task, int taskCount);
    void workerLoop(unsigned int initialGeneration);

    std::vector<std::thread> workers_;
    std::mutex mutex_;
    std::condition_variable wake_;
    std::condition_variable finished_;
    const std::function<void(int)> *job_ = nullptr;
    int jobCount_ = 0;
    std::atomic<int> nextTask_{0};
    std::atomic<int> completed_{0};
    unsigned int generation_ = 0;
    int workerTarget_ = 0;
    bool started_ = false;
    bool stopping_ = false;
};

} // namespace base
