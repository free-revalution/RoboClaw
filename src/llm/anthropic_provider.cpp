// AnthropicProvider实现

#include "anthropic_provider.h"
#include <random>
#include <iomanip>
#include <sstream>

namespace roboclaw {

AnthropicProvider::AnthropicProvider(const std::string& apiKey,
                                     const std::string& model,
                                     const std::string& baseUrl)
    : LLMProvider(apiKey, baseUrl) {
    model_ = model;
    if (base_url_.empty()) {
        base_url_ = "https://api.anthropic.com";
    }

    // 设置默认头部
    http_client_.setDefaultHeader("x-api-key", api_key);
    http_client_.setDefaultHeader("anthropic-version", "2023-06-01");
    http_client_.setDefaultHeader("Content-Type", "application/json");
}

LLMResponse AnthropicProvider::chat(const std::vector<ChatMessage>& messages,
                                    const std::vector<ToolDefinition>& tools) {
    LLMResponse response;

    try {
        // 构建请求体
        json requestBody = buildRequestBody(messages, tools);

        // 发送请求
        std::string url = base_url_ + "/v1/messages";
        std::map<std::string, std::string> headers;
        headers["x-api-key"] = api_key_;

        HttpResponse httpResponse = http_client_.postJson(url, requestBody, headers);

        if (!httpResponse.success) {
            response.error = "HTTP请求失败: " + std::to_string(httpResponse.status_code);
            return response;
        }

        // 解析响应
        response = parseResponse(httpResponse.body);
        response.raw_response = json::parse(httpResponse.body);

    } catch (const std::exception& e) {
        response.error = std::string("请求异常: ") + e.what();
    }

    return response;
}

bool AnthropicProvider::chatStream(const std::vector<ChatMessage>& messages,
                                   const std::vector<ToolDefinition>& tools,
                                   StreamCallback callback) {
    try {
        // 构建请求体
        json requestBody = buildRequestBody(messages, tools);
        requestBody["stream"] = true;

        // 发送流式请求
        std::string url = base_url_ + "/v1/messages";
        std::map<std::string, std::string> headers;
        headers["x-api-key"] = api_key;

        return http_client_.postStream(url, requestBody, headers, callback);

    } catch (const std::exception& e) {
        callback("error: " + std::string(e.what()));
        return false;
    }
}

json AnthropicProvider::buildRequestBody(const std::vector<ChatMessage>& messages,
                                         const std::vector<ToolDefinition>& tools) const {
    json request;

    request["model"] = model_;
    request["max_tokens"] = max_tokens_;

    // 转换消息
    json convertedMessages = json::array();
    for (const auto& msg : messages) {
        convertedMessages.push_back(convertMessage(msg));
    }
    request["messages"] = convertedMessages;

    // 添加系统消息（如果有）
    for (const auto& msg : messages) {
        if (msg.role == MessageRole::SYSTEM) {
            request["system"] = msg.content;
            break;
        }
    }

    // 添加工具定义
    if (!tools.empty()) {
        json toolsArray = json::array();
        for (const auto& tool : tools) {
            toolsArray.push_back(convertTool(tool));
        }
        request["tools"] = toolsArray;
    }

    return request;
}

json AnthropicProvider::convertMessage(const ChatMessage& msg) const {
    json anthropicMsg;

    switch (msg.role) {
        case MessageRole::SYSTEM:
            // 系统消息在Anthropic中是单独的字段
            anthropicMsg["role"] = "user";
            anthropicMsg["content"] = msg.content;
            break;

        case MessageRole::USER:
            anthropicMsg["role"] = "user";
            if (msg.role == MessageRole::TOOL) {
                // 工具响应作为用户内容
                json content;
                content["type"] = "tool_result";
                content["tool_use_id"] = msg.tool_call_id;
                content["content"] = msg.content;
                if (msg.is_error) {
                    content["is_error"] = true;
                }
                anthropicMsg["content"] = json::array({content});
            } else {
                anthropicMsg["content"] = msg.content;
            }
            break;

        case MessageRole::ASSISTANT:
            anthropicMsg["role"] = "assistant";
            if (msg.tool_calls.empty()) {
                anthropicMsg["content"] = msg.content;
            } else {
                // 有工具调用
                json contentArray = json::array();

                // 添加文本内容（如果有）
                if (!msg.content.empty()) {
                    json textContent;
                    textContent["type"] = "text";
                    textContent["text"] = msg.content;
                    contentArray.push_back(textContent);
                }

                // 添加工具调用
                for (const auto& call : msg.tool_calls) {
                    json toolContent;
                    toolContent["type"] = "tool_use";
                    toolContent["id"] = call.id;
                    toolContent["name"] = call.name;
                    toolContent["input"] = call.arguments;
                    contentArray.push_back(toolContent);
                }

                anthropicMsg["content"] = contentArray;
            }
            break;

        case MessageRole::TOOL:
            anthropicMsg["role"] = "user";
            json content;
            content["type"] = "tool_result";
            content["tool_use_id"] = msg.tool_call_id;
            content["content"] = msg.content;
            if (msg.is_error) {
                content["is_error"] = true;
            }
            anthropicMsg["content"] = json::array({content});
            break;
    }

    return anthropicMsg;
}

json AnthropicProvider::convertTool(const ToolDefinition& tool) const {
    json anthropicTool;
    anthropicTool["name"] = tool.name;
    anthropicTool["description"] = tool.description;

    // 转换input_schema
    if (tool.input_schema.contains("properties")) {
        anthropicTool["input_schema"] = tool.input_schema;
    }

    return anthropicTool;
}

LLMResponse AnthropicProvider::parseResponse(const std::string& body) const {
    LLMResponse response;

    try {
        json jsonResponse = json::parse(body);

        // 检查错误
        if (jsonResponse.contains("error")) {
            response.error = jsonResponse["error"]["message"];
            return response;
        }

        // 获取内容
        if (jsonResponse.contains("content")) {
            auto content = jsonResponse["content"];

            std::string textContent;
            std::vector<ChatMessage::ToolCall> toolCalls;

            if (content.is_array()) {
                for (const auto& item : content) {
                    std::string type = item.value("type", "");

                    if (type == "text") {
                        textContent += item.value("text", "");
                    } else if (type == "tool_use") {
                        ChatMessage::ToolCall call;
                        call.id = item.value("id", "");
                        call.name = item.value("name", "");
                        call.arguments = item.value("input", json::object());
                        toolCalls.push_back(call);
                    }
                }
            }

            response.content = textContent;
            response.tool_calls = toolCalls;
        }

        // 获取使用量
        if (jsonResponse.contains("usage")) {
            auto usage = jsonResponse["usage"];
            response.input_tokens = usage.value("input_tokens", 0);
            response.output_tokens = usage.value("output_tokens", 0);
        }

        response.success = true;

    } catch (const std::exception& e) {
        response.error = std::string("解析响应失败: ") + e.what();
    }

    return response;
}

std::vector<ChatMessage::ToolCall> AnthropicProvider::parseToolCalls(const json& content) const {
    std::vector<ChatMessage::ToolCall> toolCalls;

    if (content.is_array()) {
        for (const auto& item : content) {
            std::string type = item.value("type", "");
            if (type == "tool_use") {
                ChatMessage::ToolCall call;
                call.id = item.value("id", generateToolCallId());
                call.name = item.value("name", "");
                call.arguments = item.value("input", json::object());
                toolCalls.push_back(call);
            }
        }
    }

    return toolCalls;
}

std::string AnthropicProvider::generateToolCallId() const {
    // 生成格式: toolu_随机字符串
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << "toolu_";

    for (int i = 0; i < 24; ++i) {
        ss << std::hex << dis(gen);
    }

    return ss.str();
}

} // namespace roboclaw
