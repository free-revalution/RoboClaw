// ThreadPool实现

#include "thread_pool.h"
#include "../utils/logger.h"
#include <sstream>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <mutex>

namespace roboclaw {

std::unique_ptr<ThreadPool> GlobalThreadPool::pool_ = nullptr;
std::once_flag GlobalThreadPool::init_flag_;

// ==================== ThreadPool ====================

ThreadPool::ThreadPool(size_t numThreads)
    : stopped_(false)
    , max_queue_size_(0)
    , active_threads_(0)
    , completed_tasks_(0) {

    config_.min_threads = numThreads > 0 ? numThreads : 1;
    config_.max_threads = numThreads;

    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back(&ThreadPool::worker, this);
    }

    std::stringstream ss;
    ss << "线程池已创建，线程数: " << numThreads;
    LOG_INFO(ss.str());
}

ThreadPool::ThreadPool(const ThreadPoolConfig& config)
    : config_(config)
    , stopped_(false)
    , max_queue_size_(config.max_queue_size)
    , active_threads_(0)
    , completed_tasks_(0) {

    size_t numThreads = config.min_threads;
    if (numThreads == 0) {
        numThreads = 1;
    }

    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back(&ThreadPool::worker, this);
    }

    std::stringstream ss;
    ss << "线程池已创建，最小线程数: " << numThreads << ", 最大线程数: " << config.max_threads;
    LOG_INFO(ss.str());
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::worker() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            // 等待任务或停止信号
            condition_.wait(lock, [this] {
                return stopped_ || !tasks_.empty();
            });

            // 如果已停止且没有任务，退出
            if (stopped_ && tasks_.empty()) {
                return;
            }

            // 获取任务
            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }

        if (task) {
            active_threads_++;
            try {
                task();
                completed_tasks_++;
            } catch (const std::exception& e) {
                LOG_ERROR("线程池任务执行异常: " + std::string(e.what()));
            } catch (...) {
                LOG_ERROR("线程池任务执行未知异常");
            }
            active_threads_--;
        }
    }
}

ThreadPoolStats ThreadPool::getStats() const {
    ThreadPoolStats stats;
    stats.total_threads = workers_.size();
    stats.active_threads = active_threads_.load();
    stats.completed_tasks = completed_tasks_.load();

    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stats.pending_tasks = tasks_.size();
    }

    return stats;
}

void ThreadPool::setConfig(const ThreadPoolConfig& config) {
    config_ = config;
    max_queue_size_ = config.max_queue_size;

    if (config.enable_dynamic_scaling) {
        adjustThreads();
    }
}

void ThreadPool::resize(size_t numThreads) {
    if (numThreads == 0 || numThreads == workers_.size()) {
        return;
    }

    // 增加线程
    if (numThreads > workers_.size()) {
        size_t add = numThreads - workers_.size();
        for (size_t i = 0; i < add; ++i) {
            workers_.emplace_back(&ThreadPool::worker, this);
        }
    }
    // 减少线程（通过设置停止信号）
    else {
        // 注意：这里简单实现，实际可能需要更复杂的逻辑
        std::stringstream ss;
        ss << "减少线程池大小从 " << workers_.size() << " 到 " << numThreads;
        LOG_INFO(ss.str());
        // 线程会在下次唤醒时自动退出
    }
}

void ThreadPool::adjustThreads() {
    size_t current = workers_.size();
    size_t active = active_threads_.load();
    size_t pending = 0;

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        pending = tasks_.size();
    }

    // 如果所有线程都忙碌且有大量待处理任务，增加线程
    if (active >= current && pending > current / 2 &&
        current < config_.max_threads) {
        size_t add = std::min({pending / 2, config_.max_threads - current, size_t(4)});
        for (size_t i = 0; i < add; ++i) {
            workers_.emplace_back(&ThreadPool::worker, this);
        }
        LOG_INFO("线程池自动扩展，当前线程数: " + std::to_string(workers_.size()));
    }
    // 如果线程利用率低且超过最小线程数，可以考虑减少
    else if (active < current / 4 && current > config_.min_threads) {
        // 简化实现：暂不减少线程
    }
}

void ThreadPool::stop() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stopped_ = true;
    }
    condition_.notify_all();

    // 等待所有线程完成
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();

    LOG_INFO("线程池已停止");
}

void ThreadPool::stopNow() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        // 清空任务队列
        while (!tasks_.empty()) {
            tasks_.pop();
        }
        stopped_ = true;
    }
    condition_.notify_all();

    // 等待所有线程完成
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();

    LOG_INFO("线程池已立即停止");
}

void ThreadPool::waitForAll() {
    // 等待任务队列为空且没有活跃线程
    while (true) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (tasks_.empty() && active_threads_ == 0) {
                return;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// ==================== GlobalThreadPool ====================

GlobalThreadPool::GlobalThreadPool() {
    // 私有构造函数
}

ThreadPool& GlobalThreadPool::instance() {
    std::call_once(init_flag_, []() {
        ThreadPoolConfig config;
        config.min_threads = 4;
        config.max_threads = std::thread::hardware_concurrency();
        pool_ = std::unique_ptr<ThreadPool>(new ThreadPool(config));
    });
    return *pool_;
}

} // namespace roboclaw
