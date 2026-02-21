// Token预算管理 - TokenBudget
// 管理token使用预算和优化建议

#ifndef ROBOCLAW_OPTIMIZATION_TOKEN_BUDGET_H
#define ROBOCLAW_OPTIMIZATION_TOKEN_BUDGET_H

#include "token_optimizer.h"
#include <string>
#include <functional>
#include <memory>
#include <mutex>

namespace roboclaw {

// Token预算管理器
class TokenBudget {
public:
    TokenBudget();
    ~TokenBudget() = default;

    // 设置预算
    void setBudget(int maxTokens);
    int getBudget() const { return max_tokens_; }

    // 设置优化器
    void setOptimizer(std::shared_ptr<TokenOptimizer> optimizer) {
        optimizer_ = optimizer;
    }

    // 检查预算
    bool checkBudget(const std::vector<ChatMessage>& messages);

    // 获取当前使用量
    int getCurrentUsage() const {
        std::lock_guard<std::mutex> lock(usage_mutex_);
        return current_usage_;
    }

    // 获取剩余预算
    int getRemainingBudget() const {
        std::lock_guard<std::mutex> lock(usage_mutex_);
        return std::max(0, max_tokens_ - current_usage_);
    }

    // 获取使用百分比
    double getUsagePercentage() const {
        std::lock_guard<std::mutex> lock(usage_mutex_);
        if (max_tokens_ == 0) return 0.0;
        return (static_cast<double>(current_usage_) / max_tokens_) * 100.0;
    }

    // 更新使用量
    void updateUsage(int tokens) {
        std::lock_guard<std::mutex> lock(usage_mutex_);
        current_usage_ += tokens;
    }

    // 重置使用量
    void resetUsage() {
        std::lock_guard<std::mutex> lock(usage_mutex_);
        current_usage_ = 0;
    }

    // 获取预算警告级别
    enum class WarningLevel {
        NONE,       // 0-50%
        LOW,        // 50-75%
        MEDIUM,     // 75-90%
        HIGH        // 90-100%
    };

    WarningLevel getWarningLevel() const;
    std::string getWarningLevelString() const;

    // 获取优化建议
    std::string getOptimizationSuggestion() const;

    // 设置警告回调
    void setWarningCallback(std::function<void(const std::string&)> callback) {
        warning_callback_ = callback;
    }

private:
    int max_tokens_;
    int current_usage_;
    std::shared_ptr<TokenOptimizer> optimizer_;

    std::function<void(const std::string&)> warning_callback_;

    // 互斥锁保证线程安全
    mutable std::mutex usage_mutex_;

    // 触发警告
    void triggerWarning(const std::string& message);
};

} // namespace roboclaw

#endif // ROBOCLAW_OPTIMIZATION_TOKEN_BUDGET_H
