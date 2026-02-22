#pragma once

#include "social_adapter.h"
#include "social_message.h"
#include "../agent/task_coordinator.h"
#include <memory>
#include <map>
#include <string>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>

namespace roboclaw::social {

/**
 * @brief SocialManager coordinates social platform adapters and routes messages
 *        to the TaskCoordinator for analysis and potential delegation.
 *
 * This class manages multiple social platform adapters, handles their lifecycle,
 * and provides a message processing pipeline that integrates with the task
 * coordination system.
 */
class SocialManager {
public:
    SocialManager();
    ~SocialManager();

    // Disable copy, enable move
    SocialManager(const SocialManager&) = delete;
    SocialManager& operator=(const SocialManager&) = delete;
    SocialManager(SocialManager&&) noexcept = default;
    SocialManager& operator=(SocialManager&&) noexcept = default;

    /**
     * @brief Register a social platform adapter
     * @param platform_id Unique identifier for the platform (e.g., "telegram", "wechat")
     * @param adapter Shared pointer to the adapter instance
     */
    void registerAdapter(const std::string& platform_id,
                        std::shared_ptr<ISocialAdapter> adapter);

    /**
     * @brief Connect to a social platform
     * @param platform_id Platform identifier
     * @param config Configuration JSON for the platform connection
     * @return true if connection successful, false otherwise
     */
    bool connectPlatform(const std::string& platform_id,
                        const nlohmann::json& config);

    /**
     * @brief Disconnect from a social platform
     * @param platform_id Platform identifier
     */
    void disconnectPlatform(const std::string& platform_id);

    /**
     * @brief Process a social message through the task coordinator
     * @param message The social message to process
     * @return true if message was processed successfully, false otherwise
     */
    bool processMessage(const SocialMessage& message);

    /**
     * @brief Start the message receiving loop
     *        This continuously polls all connected adapters for new messages
     */
    void startMessageLoop();

    /**
     * @brief Stop the message receiving loop
     */
    void stopMessageLoop();

    /**
     * @brief Check if message loop is running
     * @return true if loop is active, false otherwise
     */
    bool isMessageLoopRunning() const { return message_loop_running_; }

    /**
     * @brief Get number of registered adapters
     * @return Count of registered adapters
     */
    size_t getAdapterCount() const { return adapters_.size(); }

    /**
     * @brief Check if a platform is connected
     * @param platform_id Platform identifier
     * @return true if platform is registered and connected, false otherwise
     */
    bool isPlatformConnected(const std::string& platform_id) const;

    /**
     * @brief Send a message to a specific chat on a platform
     * @param platform_id Platform identifier
     * @param chat_id Chat/session identifier
     * @param content Message content to send
     * @return true if message sent successfully, false otherwise
     */
    bool sendMessage(const std::string& platform_id,
                    const std::string& chat_id,
                    const std::string& content);

private:
    /**
     * @brief Internal message loop implementation
     */
    void messageLoop();

    /**
     * @brief Process messages from a specific adapter
     * @param platform_id Platform identifier
     * @param adapter The adapter to process messages from
     */
    void processMessagesFromAdapter(const std::string& platform_id,
                                    std::shared_ptr<ISocialAdapter> adapter);

    /**
     * @brief Create a task description from a social message
     * @param message The social message
     * @return JSON task description for the coordinator
     */
    nlohmann::json createTaskDescription(const SocialMessage& message) const;

    /**
     * @brief Send a response back through the appropriate adapter
     * @param original_message The original message being responded to
     * @param response_content The response content
     */
    void sendResponse(const SocialMessage& original_message,
                      const std::string& response_content);

    std::map<std::string, std::shared_ptr<ISocialAdapter>> adapters_;
    std::unique_ptr<roboclaw::agent::TaskCoordinator> coordinator_;
    std::atomic<bool> message_loop_running_;
    std::thread message_loop_thread_;
    mutable std::mutex adapters_mutex_;
};

} // namespace roboclaw::social
