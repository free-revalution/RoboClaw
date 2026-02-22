#include "social_manager.h"
#include "../utils/logger.h"
#include <algorithm>
#include <sstream>

namespace roboclaw::social {

SocialManager::SocialManager()
    : coordinator_(std::make_unique<roboclaw::agent::TaskCoordinator>()),
      message_loop_running_(false) {
    LOG_INFO("SocialManager initialized");
}

SocialManager::~SocialManager() {
    stopMessageLoop();

    // Disconnect all platforms
    std::lock_guard<std::mutex> lock(adapters_mutex_);
    for (auto& [platform_id, adapter] : adapters_) {
        try {
            if (adapter->isConnected()) {
                adapter->disconnect();
                LOG_INFO("Disconnected platform: " + platform_id);
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Error disconnecting platform " + platform_id + ": " + e.what());
        }
    }
    adapters_.clear();

    LOG_INFO("SocialManager destroyed");
}

void SocialManager::registerAdapter(const std::string& platform_id,
                                    std::shared_ptr<ISocialAdapter> adapter) {
    if (!adapter) {
        LOG_ERROR("Cannot register null adapter for platform: " + platform_id);
        return;
    }

    std::lock_guard<std::mutex> lock(adapters_mutex_);
    adapters_[platform_id] = adapter;
    LOG_INFO("Registered adapter for platform: " + platform_id);
}

bool SocialManager::connectPlatform(const std::string& platform_id,
                                    const nlohmann::json& config) {
    std::shared_ptr<ISocialAdapter> adapter;
    {
        std::lock_guard<std::mutex> lock(adapters_mutex_);
        auto it = adapters_.find(platform_id);
        if (it == adapters_.end()) {
            LOG_ERROR("No adapter registered for platform: " + platform_id);
            return false;
        }
        adapter = it->second;
    }

    try {
        if (adapter->connect(config)) {
            LOG_INFO("Successfully connected to platform: " + platform_id);
            return true;
        } else {
            LOG_ERROR("Failed to connect to platform: " + platform_id);
            return false;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Exception connecting to platform " + platform_id + ": " + e.what());
        return false;
    }
}

void SocialManager::disconnectPlatform(const std::string& platform_id) {
    std::lock_guard<std::mutex> lock(adapters_mutex_);
    auto it = adapters_.find(platform_id);
    if (it != adapters_.end()) {
        try {
            it->second->disconnect();
            LOG_INFO("Disconnected platform: " + platform_id);
        } catch (const std::exception& e) {
            LOG_ERROR("Error disconnecting platform " + platform_id + ": " + e.what());
        }
    } else {
        LOG_WARN("Cannot disconnect unknown platform: " + platform_id);
    }
}

bool SocialManager::isPlatformConnected(const std::string& platform_id) const {
    std::lock_guard<std::mutex> lock(adapters_mutex_);
    auto it = adapters_.find(platform_id);
    return it != adapters_.end() && it->second->isConnected();
}

bool SocialManager::processMessage(const SocialMessage& message) {
    LOG_INFO("Processing message from " + message.platform_id +
             ", user: " + message.user_id);

    try {
        // Create task description from the social message
        nlohmann::json task_desc = createTaskDescription(message);

        // Analyze the task
        roboclaw::agent::TaskAnalysis analysis = coordinator_->analyzeTask(task_desc);

        LOG_INFO("Task analysis - category: " + analysis.category +
                 ", language: " + analysis.language +
                 ", complexity: " + analysis.complexity);

        // Determine if we should delegate to an external agent
        std::string best_agent = coordinator_->selectBestAgent(analysis);

        if (!best_agent.empty() && coordinator_->shouldDelegate(analysis, best_agent)) {
            LOG_INFO("Delegating task to agent: " + best_agent);

            // Build context with message metadata
            nlohmann::json context;
            context["platform_id"] = message.platform_id;
            context["chat_id"] = message.chat_id;
            context["user_id"] = message.user_id;
            context["message_id"] = message.message_id;
            context["metadata"] = message.metadata;

            bool delegated = coordinator_->delegateToAgent(
                best_agent,
                message.content,
                context
            );

            if (delegated) {
                LOG_INFO("Task successfully delegated to agent: " + best_agent);
                return true;
            } else {
                LOG_WARN("Delegation to agent " + best_agent + " failed, processing locally");
                // Fall through to local processing
            }
        }

        // Local processing: send acknowledgment
        std::ostringstream response;
        response << "Message received. Task analysis:\n"
                 << "  Category: " << analysis.category << "\n"
                 << "  Language: " << analysis.language << "\n"
                 << "  Complexity: " << analysis.complexity;

        sendResponse(message, response.str());

        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Error processing message: " + std::string(e.what()));
        sendResponse(message, "Error processing your message. Please try again.");
        return false;
    }
}

void SocialManager::startMessageLoop() {
    if (message_loop_running_) {
        LOG_WARN("Message loop is already running");
        return;
    }

    message_loop_running_ = true;
    message_loop_thread_ = std::thread(&SocialManager::messageLoop, this);

    LOG_INFO("Message loop started");
}

void SocialManager::stopMessageLoop() {
    if (!message_loop_running_) {
        return;
    }

    message_loop_running_ = false;

    if (message_loop_thread_.joinable()) {
        message_loop_thread_.join();
    }

    LOG_INFO("Message loop stopped");
}

bool SocialManager::sendMessage(const std::string& platform_id,
                                const std::string& chat_id,
                                const std::string& content) {
    std::lock_guard<std::mutex> lock(adapters_mutex_);
    auto it = adapters_.find(platform_id);

    if (it == adapters_.end()) {
        LOG_ERROR("No adapter found for platform: " + platform_id);
        return false;
    }

    if (!it->second->isConnected()) {
        LOG_ERROR("Platform not connected: " + platform_id);
        return false;
    }

    try {
        return it->second->sendMessage(chat_id, content);
    } catch (const std::exception& e) {
        LOG_ERROR("Error sending message on " + platform_id + ": " + e.what());
        return false;
    }
}

void SocialManager::messageLoop() {
    LOG_INFO("Message loop thread started");

    // Create a copy of adapters to avoid holding the lock during processing
    std::vector<std::pair<std::string, std::shared_ptr<ISocialAdapter>>> connected_adapters;

    while (message_loop_running_) {
        connected_adapters.clear();

        {
            std::lock_guard<std::mutex> lock(adapters_mutex_);
            for (const auto& [platform_id, adapter] : adapters_) {
                if (adapter->isConnected()) {
                    connected_adapters.push_back({platform_id, adapter});
                }
            }
        }

        // Process messages from each connected adapter
        for (const auto& [platform_id, adapter] : connected_adapters) {
            if (!message_loop_running_) {
                break;
            }

            processMessagesFromAdapter(platform_id, adapter);
        }

        // Sleep before next poll to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    LOG_INFO("Message loop thread ended");
}

void SocialManager::processMessagesFromAdapter(
    const std::string& platform_id,
    std::shared_ptr<ISocialAdapter> adapter) {

    try {
        std::vector<SocialMessage> messages = adapter->receiveMessages();

        for (const auto& message : messages) {
            if (!message_loop_running_) {
                break;
            }

            LOG_DEBUG("Received message from " + platform_id +
                     " in chat " + message.chat_id);

            processMessage(message);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error receiving messages from " + platform_id + ": " + e.what());
    }
}

nlohmann::json SocialManager::createTaskDescription(const SocialMessage& message) const {
    nlohmann::json task_desc;
    task_desc["type"] = "social_message";
    task_desc["platform"] = message.platform_id;
    task_desc["content"] = message.content;
    task_desc["user_id"] = message.user_id;
    task_desc["chat_id"] = message.chat_id;
    task_desc["timestamp"] = message.timestamp;
    task_desc["metadata"] = message.metadata;

    // Try to detect command prefix if available
    std::lock_guard<std::mutex> lock(adapters_mutex_);
    auto it = adapters_.find(message.platform_id);
    if (it != adapters_.end()) {
        std::string prefix = it->second->getCommandPrefix();
        if (!prefix.empty() && message.content.find(prefix) == 0) {
            task_desc["is_command"] = true;
            task_desc["command"] = message.content.substr(prefix.length());
        } else {
            task_desc["is_command"] = false;
        }
    }

    return task_desc;
}

void SocialManager::sendResponse(const SocialMessage& original_message,
                                  const std::string& response_content) {
    std::lock_guard<std::mutex> lock(adapters_mutex_);
    auto it = adapters_.find(original_message.platform_id);

    if (it != adapters_.end()) {
        try {
            it->second->sendMessage(original_message.chat_id, response_content);
            LOG_DEBUG("Sent response to chat " + original_message.chat_id);
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to send response: " + std::string(e.what()));
        }
    }
}

} // namespace roboclaw::social
