// 通用线程池实现
// Generic thread pool implementation

#ifndef ROBOCLAW_UTILS_THREAD_POOL_H
#define ROBOCLAW_UTILS_THREAD_POOL_H

#include <functional>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <memory>
#include <atomic>
#include <chrono>
#include <type_traits>

namespace roboclaw {

// 线程池配置
struct ThreadPoolConfig {
    size_t min_threads;           // 最小线程数
    size_t max_threads;           // 最大线程数
    size_t max_queue_size;        // 最大任务队列大小（0表示无限制）
    bool enable_dynamic_scaling;  // 启用动态线程调整

    ThreadPoolConfig()
        : min_threads(2)
        , max_threads(std::thread::hardware_concurrency())
        , max_queue_size(0)
        , enable_dynamic_scaling(true) {}
};

// 线程池统计信息
struct ThreadPoolStats {
    size_t total_threads;         // 总线程数
    size_t active_threads;        // 活跃线程数
    size_t pending_tasks;         // 等待中的任务数
    size_t completed_tasks;       // 已完成任务数
};

// 线程池类
class ThreadPool {
public:
    // 创建指定大小的线程池
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency());

    // 使用配置创建线程池
    explicit ThreadPool(const ThreadPoolConfig& config);

    ~ThreadPool();

    // 禁止拷贝和移动
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    // 提交任务（无返回值）
    template<typename F>
    void submit(F&& task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (max_queue_size_ > 0 && tasks_.size() >= max_queue_size_) {
                throw std::runtime_error("任务队列已满");
            }
            tasks_.emplace(std::forward<F>(task));
        }
        condition_.notify_one();
    }

    // 提交任务（有返回值）
    template<typename F, typename... Args>
    auto submitWithResult(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result_t<F, Args...>> {
        using return_type = typename std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (max_queue_size_ > 0 && tasks_.size() >= max_queue_size_) {
                throw std::runtime_error("任务队列已满");
            }
            tasks_.emplace([task]() { (*task)(); });
        }
        condition_.notify_one();
        return result;
    }

    // 提交延迟任务
    template<typename F, typename Rep, typename Period>
    void submitAfter(F&& task, std::chrono::duration<Rep, Period> delay) {
        std::thread([this, task = std::forward<F>(task), delay]() {
            std::this_thread::sleep_for(delay);
            this->submit(task);
        }).detach();
    }

    // 获取统计信息
    ThreadPoolStats getStats() const;

    // 获取配置
    ThreadPoolConfig getConfig() const { return config_; }

    // 设置配置
    void setConfig(const ThreadPoolConfig& config);

    // 调整线程数量
    void resize(size_t numThreads);

    // 停止线程池（等待当前任务完成）
    void stop();

    // 立即停止线程池（不等待任务完成）
    void stopNow();

    // 检查是否已停止
    bool isStopped() const { return stopped_; }

    // 等待所有任务完成
    void waitForAll();

private:
    // 工作线程函数
    void worker();

    // 动态调整线程数
    void adjustThreads();

    // 成员变量
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stopped_;

    ThreadPoolConfig config_;
    size_t max_queue_size_;

    // 统计信息
    std::atomic<size_t> active_threads_;
    std::atomic<size_t> completed_tasks_;
};

// 全局线程池单例
class GlobalThreadPool {
public:
    static ThreadPool& instance();

    GlobalThreadPool(const GlobalThreadPool&) = delete;
    GlobalThreadPool& operator=(const GlobalThreadPool&) = delete;

private:
    GlobalThreadPool();
    ~GlobalThreadPool() = default;

    static std::unique_ptr<ThreadPool> pool_;
    static std::once_flag init_flag_;
};

} // namespace roboclaw

#endif // ROBOCLAW_UTILS_THREAD_POOL_H
