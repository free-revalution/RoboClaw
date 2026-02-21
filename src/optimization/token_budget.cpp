// TokenBudget实现

#include "token_budget.h"
#include "token_constants.h"
#include "../utils/logger.h"
#include <sstream>
#include <mutex>

namespace roboclaw {

TokenBudget::TokenBudget()
    : max_tokens_(TokenConstants::DEFAULT_TOKEN_BUDGET)
    , current_usage_(0)
    , optimizer_(nullptr) {
}

void TokenBudget::setBudget(int maxTokens) {
    std::lock_guard<std::mutex> lock(usage_mutex_);
    max_tokens_ = maxTokens;
}

bool TokenBudget::checkBudget(const std::vector<ChatMessage>& messages) {
    if (!optimizer_) {
        return true;  // 没有优化器，无法检查
    }

    int estimated = optimizer_->estimateNextRequest(messages, {});

    if (estimated > getRemainingBudget()) {
        triggerWarning("请求的token数(" + std::to_string(estimated) +
                       ")超过剩余预算(" + std::to_string(getRemainingBudget()) + ")");
        return false;
    }

    return true;
}

TokenBudget::WarningLevel TokenBudget::getWarningLevel() const {
    double percentage = getUsagePercentage();

    if (percentage < TokenConstants::WARNING_THRESHOLD_LOW) return WarningLevel::NONE;
    if (percentage < TokenConstants::WARNING_THRESHOLD_MEDIUM) return WarningLevel::LOW;
    if (percentage < TokenConstants::WARNING_THRESHOLD_HIGH) return WarningLevel::MEDIUM;
    return WarningLevel::HIGH;
}

std::string TokenBudget::getWarningLevelString() const {
    switch (getWarningLevel()) {
        case WarningLevel::NONE:   return "正常";
        case WarningLevel::LOW:    return "低";
        case WarningLevel::MEDIUM: return "中";
        case WarningLevel::HIGH:   return "高";
        default:                    return "未知";
    }
}

std::string TokenBudget::getOptimizationSuggestion() const {
    double percentage = getUsagePercentage();

    if (percentage >= TokenConstants::WARNING_THRESHOLD_CRITICAL) {
        return "警告：已达到token预算上限！请立即开启对话压缩或开始新对话。";
    }

    if (percentage >= TokenConstants::WARNING_THRESHOLD_HIGH) {
        return "警告：token使用量接近上限。建议启用对话压缩或清理历史记录。";
    }

    if (percentage >= TokenConstants::WARNING_THRESHOLD_MEDIUM) {
        return "提示：token使用量较高。建议启用对话压缩以节省成本。";
    }

    if (percentage >= TokenConstants::WARNING_THRESHOLD_LOW) {
        return "token使用量适中。可继续使用。";
    }

    return "token使用量良好。";
}

void TokenBudget::triggerWarning(const std::string& message) {
    LOG_WARNING("Token预算警告: " + message);

    if (warning_callback_) {
        warning_callback_(message);
    }
}

} // namespace roboclaw
