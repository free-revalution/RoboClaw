// ConversationTree实现

#include "conversation_tree.h"
#include <algorithm>

namespace roboclaw {

ConversationTree::ConversationTree()
    : conversation_id_(ConversationNode::generateId()) {
    // 创建根节点
    root_ = std::make_shared<ConversationNode>();
    nodes_[root_->getId()] = root_;
    current_node_id_ = root_->getId();
}

ConversationTree::ConversationTree(const std::string& conversationId)
    : conversation_id_(conversationId) {
    // 创建根节点
    root_ = std::make_shared<ConversationNode>();
    nodes_[root_->getId()] = root_;
    current_node_id_ = root_->getId();
}

std::shared_ptr<ConversationNode> ConversationTree::getCurrentNode() const {
    return getNode(current_node_id_);
}

void ConversationTree::setCurrentNode(const std::string& nodeId) {
    if (nodes_.find(nodeId) != nodes_.end()) {
        // 取消之前的活动状态
        if (nodes_.find(current_node_id_) != nodes_.end()) {
            nodes_[current_node_id_]->setActive(false);
        }
        // 设置新的活动状态
        current_node_id_ = nodeId;
        nodes_[nodeId]->setActive(true);
    }
}

std::shared_ptr<ConversationNode> ConversationTree::getNode(const std::string& nodeId) const {
    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<ConversationNode> ConversationTree::addNode(
        const std::string& parentId,
        const std::string& userMessage) {

    auto parent = getNode(parentId);
    if (!parent) {
        return nullptr;
    }

    // 创建新节点
    auto node = std::make_shared<ConversationNode>();
    node->setParentId(parentId);
    node->setUserMessage(userMessage);

    // 添加到父节点的子列表
    parent->addChild(node->getId());

    // 存储节点
    nodes_[node->getId()] = node;

    return node;
}

std::shared_ptr<ConversationNode> ConversationTree::createBranch(
        const std::string& parentId,
        const std::string& branchName) {

    auto parent = getNode(parentId);
    if (!parent) {
        return nullptr;
    }

    // 创建新节点作为分支起点
    auto node = std::make_shared<ConversationNode>();
    node->setParentId(parentId);
    node->setBranchName(branchName);

    // 添加到父节点的子列表
    parent->addChild(node->getId());

    // 存储节点
    nodes_[node->getId()] = node;

    return node;
}

bool ConversationTree::switchToNode(const std::string& nodeId) {
    if (nodes_.find(nodeId) == nodes_.end()) {
        return false;
    }

    setCurrentNode(nodeId);
    return true;
}

bool ConversationTree::switchToParent() {
    auto current = getCurrentNode();
    if (!current) {
        return false;
    }

    const std::string& parentId = current->getParentId();
    if (parentId.empty()) {
        return false;  // 已经是根节点
    }

    return switchToNode(parentId);
}

std::vector<std::string> ConversationTree::getBranchNames() const {
    std::vector<std::string> branches;

    for (const auto& pair : nodes_) {
        const std::string& branchName = pair.second->getBranchName();
        if (!branchName.empty()) {
            branches.push_back(branchName);
        }
    }

    return branches;
}

std::vector<std::string> ConversationTree::getPath() const {
    std::vector<std::string> path;
    getPathRecursive(current_node_id_, path);
    std::reverse(path.begin(), path.end());
    return path;
}

void ConversationTree::getPathRecursive(const std::string& nodeId,
                                         std::vector<std::string>& path) const {
    auto node = getNode(nodeId);
    if (!node) {
        return;
    }

    path.push_back(node->getId());

    const std::string& parentId = node->getParentId();
    if (!parentId.empty()) {
        getPathRecursive(parentId, path);
    }
}

json ConversationTree::toJson() const {
    json j;

    j["conversation_id"] = conversation_id_;
    j["current_node_id"] = current_node_id_;

    // 序列化所有节点
    json nodesJson = json::object();
    for (const auto& pair : nodes_) {
        nodesJson[pair.first] = pair.second->toJson();
    }
    j["nodes"] = nodesJson;

    return j;
}

bool ConversationTree::fromJson(const json& j) {
    try {
        conversation_id_ = j.value("conversation_id", ConversationNode::generateId());
        current_node_id_ = j.value("current_node_id", "");

        nodes_.clear();

        if (j.contains("nodes")) {
            for (const auto& pair : j["nodes"].items()) {
                auto node = std::make_shared<ConversationNode>(
                    ConversationNode::fromJson(pair.value())
                );
                nodes_[pair.key()] = node;

                // 设置根节点
                if (node->getParentId().empty()) {
                    root_ = node;
                }
            }
        }

        // 确保有根节点
        if (!root_) {
            root_ = std::make_shared<ConversationNode>();
            nodes_[root_->getId()] = root_;
        }

        // 确保当前节点存在
        if (nodes_.find(current_node_id_) == nodes_.end()) {
            current_node_id_ = root_->getId();
        }

        return true;

    } catch (const std::exception& e) {
        return false;
    }
}

std::vector<std::shared_ptr<ConversationNode>> ConversationTree::getAllNodes() const {
    std::vector<std::shared_ptr<ConversationNode>> nodes;
    getAllNodesRecursive(root_, nodes);
    return nodes;
}

void ConversationTree::getAllNodesRecursive(
        std::shared_ptr<ConversationNode> node,
        std::vector<std::shared_ptr<ConversationNode>>& nodes) const {

    if (!node) {
        return;
    }

    nodes.push_back(node);

    for (const auto& childId : node->getChildren()) {
        auto child = getNode(childId);
        if (child) {
            getAllNodesRecursive(child, nodes);
        }
    }
}

std::vector<std::string> ConversationTree::getConversationHistory() const {
    std::vector<std::string> history;

    std::vector<std::string> path = getPath();
    for (const auto& nodeId : path) {
        auto node = getNode(nodeId);
        if (node && !node->getUserMessage().empty()) {
            history.push_back(node->getUserMessage());
        }
    }

    return history;
}

} // namespace roboclaw
