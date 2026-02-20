// 对话树 - ConversationTree
// 管理对话的树状结构

#ifndef ROBOCLAW_SESSION_CONVERSATION_TREE_H
#define ROBOCLAW_SESSION_CONVERSATION_TREE_H

#include "conversation_node.h"
#include <string>
#include <map>
#include <memory>
#include <vector>

namespace roboclaw {

// 对话树
class ConversationTree {
public:
    ConversationTree();
    explicit ConversationTree(const std::string& conversationId);

    // 获取对话ID
    std::string getConversationId() const { return conversation_id_; }
    void setConversationId(const std::string& id) { conversation_id_ = id; }

    // 获取根节点
    std::shared_ptr<ConversationNode> getRoot() const { return root_; }

    // 获取当前活动节点
    std::shared_ptr<ConversationNode> getCurrentNode() const;
    std::string getCurrentNodeId() const { return current_node_id_; }

    // 设置当前节点
    void setCurrentNode(const std::string& nodeId);

    // 获取节点
    std::shared_ptr<ConversationNode> getNode(const std::string& nodeId) const;

    // 添加节点
    std::shared_ptr<ConversationNode> addNode(const std::string& parentId,
                                               const std::string& userMessage);

    // 创建分支
    std::shared_ptr<ConversationNode> createBranch(const std::string& parentId,
                                                   const std::string& branchName);

    // 切换到指定节点
    bool switchToNode(const std::string& nodeId);

    // 切换到父节点
    bool switchToParent();

    // 获取分支列表
    std::vector<std::string> getBranchNames() const;

    // 获取路径（从根到当前节点）
    std::vector<std::string> getPath() const;

    // 序列化
    json toJson() const;
    bool fromJson(const json& j);

    // 获取所有节点
    std::vector<std::shared_ptr<ConversationNode>> getAllNodes() const;

    // 获取对话历史（从根到当前节点）
    std::vector<std::string> getConversationHistory() const;

private:
    std::string conversation_id_;
    std::shared_ptr<ConversationNode> root_;
    std::string current_node_id_;
    std::map<std::string, std::shared_ptr<ConversationNode>> nodes_;

    // 递归获取路径
    void getPathRecursive(const std::string& nodeId, std::vector<std::string>& path) const;

    // 递归获取所有节点
    void getAllNodesRecursive(std::shared_ptr<ConversationNode> node,
                               std::vector<std::shared_ptr<ConversationNode>>& nodes) const;
};

} // namespace roboclaw

#endif // ROBOCLAW_SESSION_CONVERSATION_TREE_H
