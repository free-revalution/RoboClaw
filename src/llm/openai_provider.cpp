// OpenAIProvider实现

#include "openai_provider.h"
#include <random>
#include <sstream>
#include <iomanip>

namespace roboclaw {

OpenAIProvider::OpenAIProvider(const std::string& apiKey,
                               const std::string& model,
                               const std::string& baseUrl)
    : LLMProvider(apiKey, baseUrl) {
    model_ = model;
    if (base_url_.empty()) {
        base_url_ = "https://api.openai.com/v1";
    }

    // 设置默认头部
    http_client_.setDefaultHeader("Authorization", "Bearer " + api_key_);
    http_client_.setDefaultHeader("Content-Type", "application/json");
}

LLMResponse OpenAIProvider::chat(const std::vector<ChatMessage>& messages,
                                 const std::vector<ToolDefinition>& tools) {
    LLMResponse response;

    try {
        // 构建请求体
        json requestBody = buildRequestBody(messages, tools);

        // 发送请求
        std::string url = base_url_ + "/chat/completions";
        std::map<std::string, std::string> headers;
        headers["Authorization"] = "Bearer " + api_key_;

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

bool OpenAIProvider::chatStream(const std::vector<ChatMessage>& messages,
                                const std::vector<ToolDefinition>& tools,
                                StreamCallback callback) {
    try {
        // 构建请求体
        json requestBody = buildRequestBody(messages, tools);
        requestBody["stream"] = true;

        // 发送流式请求
        std::string url = base_url_ + "/chat/completions";
        std::map<std::string, std::string> headers;
        headers["Authorization"] = "Bearer " + api_key_;

        return http_client_.postStream(url, requestBody, headers, callback);

    } catch (const std::exception& e) {
        callback("error: " + std::string(e.what()));
        return false;
    }
}

json OpenAIProvider::buildRequestBody(const std::vector<ChatMessage>& messages,
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

json OpenAIProvider::convertMessage(const ChatMessage& msg) const {
    json openaiMsg;

    switch (msg.role) {
        case MessageRole::SYSTEM:
            openaiMsg["role"] = "system";
            openaiMsg["content"] = msg.content;
            break;

        case MessageRole::USER:
            openaiMsg["role"] = "user";
            openaiMsg["content"] = msg.content;
            break;

        case MessageRole::ASSISTANT:
            openaiMsg["role"] = "assistant";
            if (!msg.tool_calls.empty()) {
                // 有工具调用
                json calls = json::array();
                for (const auto& call : msg.tool_calls) {
                    json callJson;
                    callJson["id"] = call.id;
                    callJson["type"] = "function";
                    callJson["function"]["name"] = call.name;
                    callJson["function"]["arguments"] = call.arguments.dump();
                    calls.push_back(callJson);
                }
                openaiMsg["tool_calls"] = calls;
                // OpenAI允许同时有内容和工具调用
                if (!msg.content.empty()) {
                    openaiMsg["content"] = msg.content;
                } else {
                    openaiMsg["content"] = "";  // 必须有content字段
                }
            } else {
                openaiMsg["content"] = msg.content;
            }
            break;

        case MessageRole::TOOL:
            openaiMsg["role"] = "tool";
            openaiMsg["tool_call_id"] = msg.tool_call_id;
            openaiMsg["content"] = msg.content;
            break;
    }

    return openaiMsg;
}

json OpenAIProvider::convertTool(const ToolDefinition& tool) const {
    json openaiTool;
    openaiTool["type"] = "function";

    json function;
    function["name"] = tool.name;
    function["description"] = tool.description;
    function["parameters"] = tool.input_schema;

    openaiTool["function"] = function;

    return openaiTool;
}

LLMResponse OpenAIProvider::parseResponse(const std::string& body) const {
    LLMResponse response;

    try {
        json jsonResponse = json::parse(body);

        // 检查错误
        if (jsonResponse.contains("error")) {
            response.error = jsonResponse["error"]["message"];
            return response;
        }

        // 获取选择
        if (jsonResponse.contains("choices") && !jsonResponse["choices"].empty()) {
            auto choice = jsonResponse["choices"][0];

            // 获取消息
            if (choice.contains("message")) {
                auto message = choice["message"];

                response.content = message.value("content", "");

                // 获取工具调用
                if (message.contains("tool_calls")) {
                    for (const auto& callJson : message["tool_calls"]) {
                        ChatMessage::ToolCall call;
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

                        response.tool_calls.push_back(call);
                    }
                }
            }

            // 获取完成原因
            response.success = (choice.value("finish_reason", "") == "stop" ||
                               choice.value("finish_reason", "") == "tool_calls" ||
                               choice.value("finish_reason", "") == "length");
        }

        // 获取使用量
        if (jsonResponse.contains("usage")) {
            auto usage = jsonResponse["usage"];
            response.input_tokens = usage.value("prompt_tokens", 0);
            response.output_tokens = usage.value("completion_tokens", 0);
        }

    } catch (const std::exception& e) {
        response.error = std::string("解析响应失败: ") + e.what();
    }

    return response;
}

std::string OpenAIProvider::generateToolCallId() const {
    // 生成格式: call_随机字符串
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << "call_";

    for (int i = 0; i < 24; ++i) {
        ss << std::hex << dis(gen);
    }

    return ss.str();
}

} // namespace roboclaw
