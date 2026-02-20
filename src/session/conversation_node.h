// 对话节点 - ConversationNode
// 对话树中的单个节点

#ifndef ROBOCLAW_SESSION_CONVERSATION_NODE_H
#define ROBOCLAW_SESSION_CONVERSATION_NODE_H

#include <string>
#include <vector>
#include <memory>
#include <chrono>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace roboclaw {

// 对话节点
class ConversationNode {
public:
    ConversationNode();
    ConversationNode(const std::string& id, const std::string& parentId = "");

    // 节点ID
    std::string getId() const { return id_; }
    void setId(const std::string& id) { id_ = id; }

    // 父节点ID
    std::string getParentId() const { return parent_id_; }
    void setParentId(const std::string& parentId) { parent_id_ = parentId; }

    // 子节点ID列表
    std::vector<std::string> getChildren() const { return children_; }
    void addChild(const std::string& childId) { children_.push_back(childId); }
    void removeChild(const std::string& childId);
    bool hasChild(const std::string& childId) const;

    // 用户消息
    std::string getUserMessage() const { return user_message_; }
    void setUserMessage(const std::string& message) { user_message_ = message; }

    // 时间戳
    std::chrono::system_clock::time_point getTimestamp() const { return timestamp_; }
    void setTimestamp(const std::chrono::system_clock::time_point& time) { timestamp_ = time; }

    // AI回复
    struct AssistantMessage {
        std::string content;
        std::vector<std::string> tool_calls;  // 工具调用的JSON字符串

        json toJson() const {
            json j;
            j["content"] = content;
            j["tool_calls"] = tool_calls;
            return j;
        }

        static AssistantMessage fromJson(const json& j) {
            AssistantMessage msg;
            msg.content = j.value("content", "");
            if (j.contains("tool_calls")) {
                for (const auto& call : j["tool_calls"]) {
                    msg.tool_calls.push_back(call.get<std::string>());
                }
            }
            return msg;
        }
    };

    AssistantMessage getAssistantMessage() const { return assistant_message_; }
    void setAssistantMessage(const AssistantMessage& msg) { assistant_message_ = msg; }

    // 分支信息
    std::string getBranchName() const { return branch_name_; }
    void setBranchName(const std::string& name) { branch_name_ = name; }

    bool isActive() const { return is_active_; }
    void setActive(bool active) { is_active_ = active; }

    // 序列化
    json toJson() const;
    static ConversationNode fromJson(const json& j);

    // 生成唯一ID
    static std::string generateId();

private:
    std::string id_;
    std::string parent_id_;
    std::vector<std::string> children_;

    std::string user_message_;
    AssistantMessage assistant_message_;
    std::chrono::system_clock::time_point timestamp_;

    std::string branch_name_;
    bool is_active_;
};

} // namespace roboclaw

#endif // ROBOCLAW_SESSION_CONVERSATION_NODE_H
