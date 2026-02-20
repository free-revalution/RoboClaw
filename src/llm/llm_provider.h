// LLM提供商抽象接口 - LLMProvider
// 统一的LLM API接口

#ifndef ROBOCLAW_LLM_LLM_PROVIDER_H
#define ROBOCLAW_LLM_LLM_PROVIDER_H

#include "http_client.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace roboclaw {

// 消息角色
enum class MessageRole {
    SYSTEM,
    USER,
    ASSISTANT,
    TOOL
};

// 聊天消息
struct ChatMessage {
    MessageRole role;
    std::string content;

    // 工具调用相关（仅ASSISTANT角色使用）
    struct ToolCall {
        std::string id;
        std::string name;
        json arguments;  // JSON对象
    };
    std::vector<ToolCall> tool_calls;

    // 工具返回结果（仅TOOL角色使用）
    std::string tool_call_id;
    bool is_error = false;

    ChatMessage() : role(MessageRole::USER) {}
    ChatMessage(MessageRole r, const std::string& c)
        : role(r), content(c) {}

    json toJson() const {
        json j;
        j["content"] = content;

        switch (role) {
            case MessageRole::SYSTEM:
                j["role"] = "system";
                break;
            case MessageRole::USER:
                j["role"] = "user";
                break;
            case MessageRole::ASSISTANT:
                j["role"] = "assistant";
                if (!tool_calls.empty()) {
                    json calls = json::array();
                    for (const auto& call : tool_calls) {
                        json callJson;
                        callJson["id"] = call.id;
                        callJson["type"] = "function";
                        callJson["function"]["name"] = call.name;
                        callJson["function"]["arguments"] = call.arguments.dump();
                        calls.push_back(callJson);
                    }
                    j["tool_calls"] = calls;
                }
                break;
            case MessageRole::TOOL:
                j["role"] = "tool";
                j["tool_call_id"] = tool_call_id;
                if (is_error) {
                    j["content"] = "Error: " + content;
                }
                break;
        }

        return j;
    }

    static ChatMessage fromJson(const json& j) {
        ChatMessage msg;

        std::string roleStr = j.value("role", "user");
        if (roleStr == "system") msg.role = MessageRole::SYSTEM;
        else if (roleStr == "assistant") msg.role = MessageRole::ASSISTANT;
        else if (roleStr == "tool") msg.role = MessageRole::TOOL;
        else msg.role = MessageRole::USER;

        msg.content = j.value("content", "");

        if (j.contains("tool_calls")) {
            for (const auto& callJson : j["tool_calls"]) {
                ToolCall call;
                call.id = callJson.value("id", "");
                if (callJson.contains("function")) {
                    call.name = callJson["function"].value("name", "");
                    std::string argsStr = callJson["function"].value("arguments", "{}");
                    try {
                        call.arguments = json::parse(argsStr);
                    } catch (...) {
                        call.arguments = json::object();
                    }
                }
                msg.tool_calls.push_back(call);
            }
        }

        if (j.contains("tool_call_id")) {
            msg.tool_call_id = j["tool_call_id"];
        }

        return msg;
    }
};

// 工具定义（用于发送给LLM）
struct ToolDefinition {
    std::string name;
    std::string description;
    json input_schema;  // JSON Schema格式的参数定义

    json toJson() const {
        json j;
        j["name"] = name;
        j["description"] = description;

        // 转换为OpenAI格式
        json function;
        function["name"] = name;
        function["description"] = description;
        function["parameters"] = input_schema;

        j["type"] = "function";
        j["function"] = function;

        return j;
    }
};

// LLM响应
struct LLMResponse {
    std::string content;
    std::vector<ChatMessage::ToolCall> tool_calls;
    bool success;
    std::string error;
    json raw_response;

    // 使用量统计
    int input_tokens = 0;
    int output_tokens = 0;

    LLMResponse() : success(false) {}
};

// 流式响应回调
using StreamCallback = std::function<void(const std::string& delta)>;

// LLM提供商抽象基类
class LLMProvider {
public:
    LLMProvider(const std::string& apiKey, const std::string& baseUrl = "")
        : api_key_(apiKey), base_url_(baseUrl) {}

    virtual ~LLMProvider() = default;

    // 发送消息，获取响应
    virtual LLMResponse chat(const std::vector<ChatMessage>& messages,
                            const std::vector<ToolDefinition>& tools = {}) = 0;

    // 流式响应
    virtual bool chatStream(const std::vector<ChatMessage>& messages,
                           const std::vector<ToolDefinition>& tools,
                           StreamCallback callback) = 0;

    // 获取模型名称
    virtual std::string getModelName() const = 0;

    // 设置模型
    virtual void setModel(const std::string& model) {
        model_ = model;
    }

    // 获取最大token数
    virtual int getMaxTokens() const {
        return max_tokens_;
    }

    // 设置最大token数
    void setMaxTokens(int maxTokens) {
        max_tokens_ = maxTokens;
    }

    // 获取API密钥
    std::string getApiKey() const { return api_key_; }

    // 获取基础URL
    std::string getBaseUrl() const { return base_url_; }

protected:
    std::string api_key_;
    std::string base_url_;
    std::string model_;
    int max_tokens_ = 4096;

    // HTTP客户端
    HttpClient http_client_;
};

// 提供商工厂
class LLMProviderFactory {
public:
    static std::unique_ptr<LLMProvider> create(const std::string& providerType,
                                               const std::string& apiKey,
                                               const std::string& baseUrl = "",
                                               const std::string& model = "");
};

} // namespace roboclaw

#endif // ROBOCLAW_LLM_LLM_PROVIDER_H
