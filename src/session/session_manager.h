// 会话管理器 - SessionManager
// 管理所有对话会话

#ifndef ROBOCLAW_SESSION_SESSION_MANAGER_H
#define ROBOCLAW_SESSION_SESSION_MANAGER_H

#include "conversation_tree.h"
#include <string>
#include <map>
#include <memory>
#include <filesystem>

namespace roboclaw {

// 会话元数据
struct SessionMetadata {
    std::string id;
    std::string title;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    int message_count;

    SessionMetadata()
        : message_count(0) {
        created_at = updated_at = std::chrono::system_clock::now();
    }

    json toJson() const {
        json j;
        j["id"] = id;
        j["title"] = title;
        j["created_at"] = std::chrono::system_clock::to_time_t(created_at);
        j["updated_at"] = std::chrono::system_clock::to_time_t(updated_at);
        j["message_count"] = message_count;
        return j;
    }

    static SessionMetadata fromJson(const json& j) {
        SessionMetadata meta;
        meta.id = j.value("id", "");
        meta.title = j.value("title", "");

        if (j.contains("created_at")) {
            std::time_t time = j["created_at"];
            meta.created_at = std::chrono::system_clock::from_time_t(time);
        }

        if (j.contains("updated_at")) {
            std::time_t time = j["updated_at"];
            meta.updated_at = std::chrono::system_clock::from_time_t(time);
        }

        meta.message_count = j.value("message_count", 0);
        return meta;
    }
};

// 会话管理器
class SessionManager {
public:
    SessionManager();
    ~SessionManager() = default;

    // 设置会话存储目录
    void setSessionsDir(const std::string& dir);
    std::string getSessionsDir() const { return sessions_dir_; }

    // 创建新会话
    std::shared_ptr<ConversationTree> createSession(const std::string& title = "");

    // 加载会话
    std::shared_ptr<ConversationTree> loadSession(const std::string& sessionId);

    // 保存会话
    bool saveSession(std::shared_ptr<ConversationTree> session);

    // 删除会话
    bool deleteSession(const std::string& sessionId);

    // 获取当前会话
    std::shared_ptr<ConversationTree> getCurrentSession() const { return current_session_; }

    // 设置当前会话
    void setCurrentSession(std::shared_ptr<ConversationTree> session) {
        current_session_ = session;
    }

    // 列出所有会话
    std::vector<SessionMetadata> listSessions() const;

    // 获取会话元数据
    SessionMetadata getSessionMetadata(const std::string& sessionId) const;

    // 获取或创建最新会话
    std::shared_ptr<ConversationTree> getOrCreateLatestSession();

    // 清理空会话
    void cleanupEmptySessions();

private:
    std::string sessions_dir_;
    std::shared_ptr<ConversationTree> current_session_;
    std::map<std::string, SessionMetadata> sessions_cache_;

    // 获取会话文件路径
    std::string getSessionFilePath(const std::string& sessionId) const;

    // 获取元数据文件路径
    std::string getMetadataFilePath(const std::string& sessionId) const;

    // 获取会话目录路径
    std::string getSessionDirPath(const std::string& sessionId) const;

    // 加载元数据
    bool loadMetadata(const std::string& sessionId, SessionMetadata& metadata) const;

    // 保存元数据
    bool saveMetadata(const SessionMetadata& metadata) const;

    // 扫描会话目录
    void scanSessionsDir();
};

} // namespace roboclaw

#endif // ROBOCLAW_SESSION_SESSION_MANAGER_H
