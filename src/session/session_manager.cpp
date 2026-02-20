// SessionManager实现

#include "session_manager.h"
#include "../utils/logger.h"
#include <fstream>
#include <algorithm>

namespace roboclaw {

SessionManager::SessionManager() {
    // 默认会话目录：.roboclaw/conversations/
    sessions_dir_ = ".roboclaw/conversations";
}

void SessionManager::setSessionsDir(const std::string& dir) {
    sessions_dir_ = dir;

    // 创建目录
    try {
        std::filesystem::create_directories(sessions_dir_);
    } catch (...) {
        LOG_ERROR("无法创建会话目录: " + sessions_dir_);
    }

    // 扫描现有会话
    scanSessionsDir();
}

std::shared_ptr<ConversationTree> SessionManager::createSession(const std::string& title) {
    // 创建新会话
    auto session = std::make_shared<ConversationTree>();

    // 创建元数据
    SessionMetadata metadata;
    metadata.id = session->getConversationId();
    metadata.title = title.empty() ? "新对话" : title;
    metadata.created_at = std::chrono::system_clock::now();
    metadata.updated_at = std::chrono::system_clock::now();
    metadata.message_count = 0;

    // 保存元数据
    saveMetadata(metadata);
    sessions_cache_[metadata.id] = metadata;

    // 保存会话
    saveSession(session);

    // 设置为当前会话
    current_session_ = session;

    LOG_INFO("创建新会话: " + metadata.id + " (" + metadata.title + ")");

    return session;
}

std::shared_ptr<ConversationTree> SessionManager::loadSession(const std::string& sessionId) {
    // 检查缓存
    auto cachedIt = sessions_cache_.find(sessionId);
    if (cachedIt != sessions_cache_.end()) {
        // 检查是否当前已加载
        if (current_session_ && current_session_->getConversationId() == sessionId) {
            return current_session_;
        }
    }

    // 从文件加载
    std::string sessionPath = getSessionFilePath(sessionId);
    std::ifstream file(sessionPath);

    if (!file.is_open()) {
        LOG_ERROR("无法打开会话文件: " + sessionPath);
        return nullptr;
    }

    try {
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        json sessionJson = json::parse(content);
        auto session = std::make_shared<ConversationTree>();

        if (session->fromJson(sessionJson)) {
            current_session_ = session;
            LOG_INFO("加载会话: " + sessionId);
            return session;
        }

    } catch (const std::exception& e) {
        LOG_ERROR("解析会话文件失败: " + std::string(e.what()));
    }

    return nullptr;
}

bool SessionManager::saveSession(std::shared_ptr<ConversationTree> session) {
    if (!session) {
        return false;
    }

    try {
        // 创建会话目录
        std::string sessionDir = getSessionDirPath(session->getConversationId());
        std::filesystem::create_directories(sessionDir);

        // 保存会话树
        std::string sessionPath = getSessionFilePath(session->getConversationId());
        std::ofstream file(sessionPath);

        if (!file.is_open()) {
            LOG_ERROR("无法创建会话文件: " + sessionPath);
            return false;
        }

        json sessionJson = session->toJson();
        file << sessionJson.dump(2);
        file.close();

        // 更新元数据
        SessionMetadata metadata = getSessionMetadata(session->getConversationId());
        if (metadata.id.empty()) {
            metadata.id = session->getConversationId();
        }
        metadata.updated_at = std::chrono::system_clock::now();
        metadata.message_count = session->getAllNodes().size();
        saveMetadata(metadata);

        LOG_DEBUG("保存会话: " + session->getConversationId());
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("保存会话失败: " + std::string(e.what()));
        return false;
    }
}

bool SessionManager::deleteSession(const std::string& sessionId) {
    try {
        std::string sessionDir = getSessionDirPath(sessionId);

        if (std::filesystem::exists(sessionDir)) {
            std::filesystem::remove_all(sessionDir);
        }

        sessions_cache_.erase(sessionId);

        if (current_session_ && current_session_->getConversationId() == sessionId) {
            current_session_ = nullptr;
        }

        LOG_INFO("删除会话: " + sessionId);
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("删除会话失败: " + std::string(e.what()));
        return false;
    }
}

std::vector<SessionMetadata> SessionManager::listSessions() const {
    std::vector<SessionMetadata> sessions;

    for (const auto& pair : sessions_cache_) {
        sessions.push_back(pair.second);
    }

    // 按更新时间排序
    std::sort(sessions.begin(), sessions.end(),
        [](const SessionMetadata& a, const SessionMetadata& b) {
            return a.updated_at > b.updated_at;
        });

    return sessions;
}

SessionMetadata SessionManager::getSessionMetadata(const std::string& sessionId) const {
    auto it = sessions_cache_.find(sessionId);
    if (it != sessions_cache_.end()) {
        return it->second;
    }

    // 尝试从文件加载
    SessionMetadata metadata;
    if (const_cast<SessionManager*>(this)->loadMetadata(sessionId, metadata)) {
        return metadata;
    }

    return SessionMetadata();
}

std::shared_ptr<ConversationTree> SessionManager::getOrCreateLatestSession() {
    // 如果有当前会话，返回它
    if (current_session_) {
        return current_session_;
    }

    // 获取会话列表
    auto sessions = listSessions();

    if (!sessions.empty()) {
        // 加载最新会话
        return loadSession(sessions[0].id);
    }

    // 创建新会话
    return createSession();
}

void SessionManager::cleanupEmptySessions() {
    for (const auto& pair : sessions_cache_) {
        if (pair.second.message_count == 0) {
            deleteSession(pair.first);
        }
    }
}

std::string SessionManager::getSessionFilePath(const std::string& sessionId) const {
    return getSessionDirPath(sessionId) + "/tree.json";
}

std::string SessionManager::getMetadataFilePath(const std::string& sessionId) const {
    return getSessionDirPath(sessionId) + "/metadata.json";
}

std::string SessionManager::getSessionDirPath(const std::string& sessionId) const {
    return sessions_dir_ + "/" + sessionId;
}

bool SessionManager::loadMetadata(const std::string& sessionId, SessionMetadata& metadata) const {
    std::string metadataPath = getMetadataFilePath(sessionId);
    std::ifstream file(metadataPath);

    if (!file.is_open()) {
        return false;
    }

    try {
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        json metadataJson = json::parse(content);
        metadata = SessionMetadata::fromJson(metadataJson);

        return true;

    } catch (...) {
        return false;
    }
}

bool SessionManager::saveMetadata(const SessionMetadata& metadata) const {
    try {
        // 创建会话目录
        std::string sessionDir = getSessionDirPath(metadata.id);
        std::filesystem::create_directories(sessionDir);

        // 保存元数据
        std::string metadataPath = getMetadataFilePath(metadata.id);
        std::ofstream file(metadataPath);

        if (!file.is_open()) {
            return false;
        }

        json metadataJson = metadata.toJson();
        file << metadataJson.dump(2);
        file.close();

        return true;

    } catch (...) {
        return false;
    }
}

void SessionManager::scanSessionsDir() {
    sessions_cache_.clear();

    try {
        if (!std::filesystem::exists(sessions_dir_)) {
            return;
        }

        for (const auto& entry : std::filesystem::directory_iterator(sessions_dir_)) {
            if (entry.is_directory()) {
                std::string sessionId = entry.path().filename().string();
                SessionMetadata metadata;
                if (loadMetadata(sessionId, metadata)) {
                    sessions_cache_[sessionId] = metadata;
                }
            }
        }

    } catch (const std::exception& e) {
        LOG_ERROR("扫描会话目录失败: " + std::string(e.what()));
    }
}

} // namespace roboclaw
