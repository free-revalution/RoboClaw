// 线程池单元测试 / Thread Pool Unit Tests

#include <gtest/gtest.h>
#include "../../src/utils/thread_pool.h"
#include <atomic>
#include <chrono>
#include <vector>
#include <algorithm>

using namespace roboclaw;

class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool = std::make_unique<ThreadPool>(4);
    }

    std::unique_ptr<ThreadPool> pool;
};

// 测试基本任务提交 / Test basic task submission
TEST_F(ThreadPoolTest, BasicTaskSubmission) {
    std::atomic<int> counter{0};

    // 提交100个任务 / Submit 100 tasks
    for (int i = 0; i < 100; ++i) {
        pool->submit([&counter]() {
            counter.fetch_add(1);
        });
    }

    // 等待所有任务完成 / Wait for all tasks to complete
    pool->waitForAll();

    EXPECT_EQ(counter.load(), 100);
}

// 测试带返回值的任务 / Test task with return value
TEST_F(ThreadPoolTest, TaskWithResult) {
    auto future = pool->submitWithResult([](int x, int y) {
        return x + y;
    }, 10, 20);

    int result = future.get();
    EXPECT_EQ(result, 30);
}

// 测试并发性能 / Test concurrent performance
TEST_F(ThreadPoolTest, ConcurrentPerformance) {
    const int task_count = 1000;
    std::atomic<int> counter{0};

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < task_count; ++i) {
        pool->submit([&counter]() {
            // 模拟一些工作 / Simulate some work
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            counter.fetch_add(1);
        });
    }

    pool->waitForAll();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_EQ(counter.load(), task_count);

    // 并发执行应该比串行快 / Concurrent execution should be faster than serial
    // 串行执行时间: 1000 * 0.1ms = 100ms (实际会更多因为线程开销)
    // 并发应该显著减少时间
    std::cout << "Completed " << task_count << " tasks in " << duration.count() << "ms\n";
}

// 测试统计信息 / Test statistics
TEST_F(ThreadPoolTest, Statistics) {
    ThreadPoolStats stats_before = pool->getStats();
    EXPECT_EQ(stats_before.total_threads, 4);
    EXPECT_EQ(stats_before.active_threads, 0);

    // 提交一些任务 / Submit some tasks
    std::atomic<int> running{0};
    std::atomic<int> completed{0};

    for (int i = 0; i < 10; ++i) {
        pool->submit([&running, &completed]() {
            running.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            running.fetch_sub(1);
            completed.fetch_add(1);
        });
    }

    // 等待一下让一些任务开始 / Wait a bit for some tasks to start
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    pool->waitForAll();

    EXPECT_EQ(completed.load(), 10);
    EXPECT_EQ(running.load(), 0);
}

// 测试动态调整线程数 / Test dynamic thread resizing
TEST_F(ThreadPoolTest, DynamicResizing) {
    ThreadPoolConfig config;
    config.min_threads = 2;
    config.max_threads = 8;
    config.enable_dynamic_scaling = true;

    ThreadPool dynamicPool(config);

    auto stats = dynamicPool.getStats();
    EXPECT_GE(stats.total_threads, config.min_threads);
    EXPECT_LE(stats.total_threads, config.max_threads);
}

// 测试延迟任务 / Test delayed task
TEST_F(ThreadPoolTest, DelayedTask) {
    std::atomic<bool> executed{false};

    // 提交100ms后执行的任务 / Submit task to execute after 100ms
    pool->submitAfter([&executed]() {
        executed.store(true);
    }, std::chrono::milliseconds(100));

    // 立即检查应该还未执行 / Check immediately, should not be executed yet
    EXPECT_FALSE(executed.load());

    // 等待150ms后应该已执行 / Wait 150ms, should be executed
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_TRUE(executed.load());
}

// 测试停止线程池 / Test stopping thread pool
TEST_F(ThreadPoolTest, StopThreadPool) {
    std::atomic<int> counter{0};

    // 提交一些任务 / Submit some tasks
    for (int i = 0; i < 10; ++i) {
        pool->submit([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            counter.fetch_add(1);
        });
    }

    // 停止线程池（等待任务完成）/ Stop pool (wait for tasks)
    pool->stop();
    EXPECT_TRUE(pool->isStopped());
    EXPECT_EQ(counter.load(), 10);
}

// 测试立即停止 / Test immediate stop
TEST_F(ThreadPoolTest, StopNow) {
    std::atomic<int> counter{0};
    std::atomic<int> executed{0};

    // 提交许多长时间运行的任务 / Submit many long-running tasks
    for (int i = 0; i < 100; ++i) {
        pool->submit([&counter, &executed]() {
            counter.fetch_add(1);
            executed.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        });
    }

    // 立即停止 / Stop immediately
    pool->stopNow();
    EXPECT_TRUE(pool->isStopped());

    // 不是所有任务都能完成 / Not all tasks will complete
    EXPECT_LT(executed.load(), 100);
}

// 测试配置 / Test configuration
TEST_F(ThreadPoolTest, Configuration) {
    ThreadPoolConfig config;
    config.min_threads = 1;
    config.max_threads = 16;
    config.max_queue_size = 1000;
    config.enable_dynamic_scaling = true;

    ThreadPool configuredPool(config);

    auto retrievedConfig = configuredPool.getConfig();
    EXPECT_EQ(retrievedConfig.min_threads, 1);
    EXPECT_EQ(retrievedConfig.max_threads, 16);
    EXPECT_EQ(retrievedConfig.max_queue_size, 1000);
    EXPECT_TRUE(retrievedConfig.enable_dynamic_scaling);
}

// 测试队列满的情况 / Test queue full scenario
TEST_F(ThreadPoolTest, QueueFull) {
    ThreadPoolConfig config;
    config.min_threads = 1;
    config.max_threads = 1;
    config.max_queue_size = 5;

    ThreadPool smallPool(config);

    std::atomic<int> blocking_count{0};

    // 提交阻塞任务来填满队列 / Submit blocking tasks to fill queue
    std::vector<std::future<void>> futures;

    for (int i = 0; i < 10; ++i) {
        try {
            auto future = smallPool.submitWithResult([&blocking_count]() {
                blocking_count.fetch_add(1);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            });
            futures.push_back(std::move(future));
        } catch (const std::runtime_error& e) {
            // 队列满时会抛出异常 / Exception when queue is full
            EXPECT_NE(std::string(e.what()).find("任务队列已满"), std::string::npos);
            break;
        }
    }

    // 等待所有任务完成 / Wait for all tasks to complete
    for (auto& future : futures) {
        future.get();
    }
}

// 测试全局线程池单例 / Test global thread pool singleton
TEST_F(ThreadPoolTest, GlobalThreadPoolSingleton) {
    auto& pool1 = GlobalThreadPool::instance();
    auto& pool2 = GlobalThreadPool::instance();

    // 应该是同一个实例 / Should be the same instance
    EXPECT_EQ(&pool1, &pool2);
}

// 测试任务异常处理 / Test task exception handling
TEST_F(ThreadPoolTest, TaskExceptionHandling) {
    auto future = pool->submitWithResult([]() {
        throw std::runtime_error("Test exception");
        return 42;
    });

    EXPECT_THROW(future.get(), std::runtime_error);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
