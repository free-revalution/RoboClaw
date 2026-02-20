// ConversationNode实现

#include "conversation_node.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace roboclaw {

ConversationNode::ConversationNode()
    : is_active_(false) {
    id_ = generateId();
    timestamp_ = std::chrono::system_clock::now();
}

ConversationNode::ConversationNode(const std::string& id, const std::string& parentId)
    : id_(id)
    , parent_id_(parentId)
    , is_active_(false) {
    timestamp_ = std::chrono::system_clock::now();
}

void ConversationNode::removeChild(const std::string& childId) {
    children_.erase(
        std::remove(children_.begin(), children_.end(), childId),
        children_.end()
    );
}

bool ConversationNode::hasChild(const std::string& childId) const {
    return std::find(children_.begin(), children_.end(), childId) != children_.end();
}

json ConversationNode::toJson() const {
    json j;

    j["id"] = id_;
    j["parent_id"] = parent_id_;

    json childrenArray = json::array();
    for (const auto& child : children_) {
        childrenArray.push_back(child);
    }
    j["children"] = childrenArray;

    j["user_message"] = user_message_;
    j["assistant_message"] = assistant_message_.toJson();

    // 转换时间戳
    auto time_t_value = std::chrono::system_clock::to_time_t(timestamp_);
    j["timestamp"] = time_t_value;

    j["branch_name"] = branch_name_;
    j["is_active"] = is_active_;

    return j;
}

ConversationNode ConversationNode::fromJson(const json& j) {
    ConversationNode node;

    node.id_ = j.value("id", generateId());
    node.parent_id_ = j.value("parent_id", "");

    if (j.contains("children")) {
        for (const auto& child : j["children"]) {
            node.children_.push_back(child.get<std::string>());
        }
    }

    node.user_message_ = j.value("user_message", "");
    if (j.contains("assistant_message")) {
        node.assistant_message_ = AssistantMessage::fromJson(j["assistant_message"]);
    }

    if (j.contains("timestamp")) {
        std::time_t time_t_value = j["timestamp"];
        node.timestamp_ = std::chrono::system_clock::from_time_t(time_t_value);
    } else {
        node.timestamp_ = std::chrono::system_clock::now();
    }

    node.branch_name_ = j.value("branch_name", "");
    node.is_active_ = j.value("is_active", false);

    return node;
}

std::string ConversationNode::generateId() {
    // 生成格式: node_随机字符串
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << "node_";

    for (int i = 0; i < 16; ++i) {
        ss << std::hex << dis(gen);
    }

    return ss.str();
}

} // namespace roboclaw
