// ConfigManager实现

#include "config_manager.h"
#include "../utils/logger.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>

// 为了简化，这里先不使用cpptoml，改用简单的键值对解析
// 后续可以升级为使用完整的TOML库

namespace roboclaw {

// 字符串工具
namespace {
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, last - first + 1);
    }

    std::string unquote(const std::string& str) {
        if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
            return str.substr(1, str.size() - 2);
        }
        if (str.size() >= 2 && str.front() == '\'' && str.back() == '\'') {
            return str.substr(1, str.size() - 2);
        }
        return str;
    }

    bool parseBool(const std::string& str) {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower == "true" || lower == "yes" || lower == "1";
    }

    int parseInt(const std::string& str, int defaultVal = 0) {
        try {
            return std::stoi(str);
        } catch (...) {
            return defaultVal;
        }
    }
}

// ProviderInfo实现
ProviderInfo* Config::getCurrentProvider() {
    auto it = providers.find(default_config.provider);
    if (it != providers.end()) {
        return &it->second;
    }
    return nullptr;
}

const ProviderInfo* Config::getCurrentProvider() const {
    auto it = providers.find(default_config.provider);
    if (it != providers.end()) {
        return &it->second;
    }
    return nullptr;
}

bool Config::validate() const {
    if (providers.empty()) return false;
    auto it = providers.find(default_config.provider);
    if (it == providers.end()) return false;

    const auto& provider = it->second;
    if (provider.api_key.empty()) return false;
    if (provider.models.empty()) return false;

    return true;
}

std::string Config::getValidationError() const {
    if (providers.empty()) {
        return "没有配置任何LLM提供商";
    }
    auto it = providers.find(default_config.provider);
    if (it == providers.end()) {
        return "默认提供商未配置";
    }
    const auto& provider = it->second;
    if (provider.api_key.empty()) {
        return "API密钥未设置";
    }
    return "";
}

// ConfigManager实现
ConfigManager::ConfigManager() {
    initializeDefaults();
}

std::string ConfigManager::getConfigPath() {
    return getConfigDir() + "/config.toml";
}

std::string ConfigManager::getConfigDir() {
#ifdef PLATFORM_WINDOWS
    const char* home = getenv("USERPROFILE");
    if (home) return std::string(home) + "\\.roboclaw";
    const char* drive = getenv("HOMEDRIVE");
    const char* path = getenv("HOMEPATH");
    if (drive && path) return std::string(drive) + std::string(path) + "\\.roboclaw";
    return "C:\\.roboclaw";
#else
    const char* home = getenv("HOME");
    if (home) return std::string(home) + "/.roboclaw";
    return "/tmp/.roboclaw";
#endif
}

bool ConfigManager::configExists() {
    std::string path = getConfigPath();
    std::ifstream file(path);
    return file.good();
}

void ConfigManager::initializeDefaults() {
    // 默认配置
    config_.default_config.provider = ProviderType::ANTHROPIC;
    config_.default_config.model = "claude-sonnet-4-20250514";

    // 行为设置
    config_.behavior.max_retries = 3;
    config_.behavior.timeout = 60;
    config_.behavior.verbose = true;
    config_.behavior.stream_delay = 10;

    // 工具设置
    config_.tools.bash_timeout = 30;
    config_.tools.forbidden_commands = {"rm -rf /", "rm -rf /*", "mkfs", "dd if=/dev/zero"};
    config_.tools.max_read_size = 10;

    // 初始化所有提供商
    for (int i = 0; i <= 5; ++i) {
        initializeProviderInfo(static_cast<ProviderType>(i));
    }
}

void ConfigManager::initializeProviderInfo(ProviderType type) {
    ProviderInfo info;
    info.type = type;
    info.name = providerToString(type);

    switch (type) {
        case ProviderType::ANTHROPIC:
            info.base_url = "https://api.anthropic.com";
            info.models = {"claude-sonnet-4-20250514", "claude-opus-4-20250514", "claude-3-5-sonnet-20241022"};
            break;
        case ProviderType::OPENAI:
            info.base_url = "https://api.openai.com/v1";
            info.models = {"gpt-4o", "gpt-4o-mini", "gpt-4-turbo", "gpt-3.5-turbo"};
            break;
        case ProviderType::GEMINI:
            info.base_url = "https://generativelanguage.googleapis.com/v1";
            info.models = {"gemini-2.0-flash", "gemini-1.5-pro"};
            break;
        case ProviderType::DEEPSEEK:
            info.base_url = "https://api.deepseek.com";
            info.models = {"deepseek-chat", "deepseek-coder"};
            break;
        case ProviderType::DOUBAO:
            info.base_url = "https://ark.cn-beijing.volces.com/api/v3";
            info.models = {"doubao-pro-32k", "doubao-lite-32k"};
            break;
        case ProviderType::QWEN:
            info.base_url = "https://dashscope.aliyuncs.com/compatible-mode/v1";
            info.models = {"qwen-max", "qwen-plus", "qwen-turbo"};
            break;
    }

    config_.providers[type] = info;
}

bool ConfigManager::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("无法打开配置文件: " + path);
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    return parseToml(content);
}

bool ConfigManager::load() {
    return load(getConfigPath());
}

bool ConfigManager::parseToml(const std::string& content) {
    std::istringstream stream(content);
    std::string line;
    std::string currentSection;

    while (std::getline(stream, line)) {
        line = trim(line);

        // 跳过空行和注释
        if (line.empty() || line[0] == '#') continue;

        // 处理节（section）
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }

        // 处理键值对
        size_t equalPos = line.find('=');
        if (equalPos != std::string::npos) {
            std::string key = trim(line.substr(0, equalPos));
            std::string value = trim(line.substr(equalPos + 1));
            value = unquote(trim(value));

            // 解析配置
            if (currentSection == "default") {
                if (key == "provider") {
                    config_.default_config.provider = stringToProvider(value);
                } else if (key == "model") {
                    config_.default_config.model = value;
                }
            } else if (currentSection == "behavior") {
                if (key == "max_retries") {
                    config_.behavior.max_retries = parseInt(value);
                } else if (key == "timeout") {
                    config_.behavior.timeout = parseInt(value);
                } else if (key == "verbose") {
                    config_.behavior.verbose = parseBool(value);
                } else if (key == "stream_delay") {
                    config_.behavior.stream_delay = parseInt(value);
                }
            } else if (currentSection == "tools") {
                if (key == "bash_timeout") {
                    config_.tools.bash_timeout = parseInt(value);
                } else if (key == "max_read_size") {
                    config_.tools.max_read_size = parseInt(value);
                }
            } else if (currentSection.find("providers.") == 0) {
                // 提供商配置
                std::string providerName = currentSection.substr(10); // "providers."长度
                ProviderType type = stringToProvider(providerName);
                auto it = config_.providers.find(type);
                if (it != config_.providers.end()) {
                    if (key == "api_key") {
                        it->second.api_key = value;
                    } else if (key == "base_url") {
                        it->second.base_url = value;
                    }
                }
            }
        }
    }

    LOG_INFO("配置加载成功");
    return true;
}

bool ConfigManager::save(const std::string& path) {
    // 创建目录
    std::filesystem::path filePath(path);
    if (filePath.has_parent_path()) {
        std::filesystem::create_directories(filePath.parent_path());
    }

    std::ofstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("无法创建配置文件: " + path);
        return false;
    }

    file << generateToml();
    file.close();

    LOG_INFO("配置保存成功: " + path);
    return true;
}

bool ConfigManager::save() {
    return save(getConfigPath());
}

std::string ConfigManager::generateToml() const {
    std::stringstream ss;

    ss << "# ============================================\n";
    ss << "# RoboClaw 配置文件\n";
    ss << "# ============================================\n\n";

    // 默认配置
    ss << "[default]\n";
    ss << "provider = \"" << providerToString(config_.default_config.provider) << "\"\n";
    ss << "model = \"" << config_.default_config.model << "\"\n\n";

    // 提供商配置
    ss << "# ============================================\n";
    ss << "# LLM提供商配置\n";
    ss << "# ============================================\n\n";

    for (const auto& pair : config_.providers) {
        const auto& provider = pair.second;
        ss << "[providers." << provider.name << "]\n";
        ss << "api_key = \"" << provider.api_key << "\"\n";
        ss << "base_url = \"" << provider.base_url << "\"\n";
        ss << "models = [";
        for (size_t i = 0; i < provider.models.size(); ++i) {
            ss << "\"" << provider.models[i] << "\"";
            if (i < provider.models.size() - 1) ss << ", ";
        }
        ss << "]\n\n";
    }

    // 行为设置
    ss << "# ============================================\n";
    ss << "# 行为设置\n";
    ss << "# ============================================\n\n";

    ss << "[behavior]\n";
    ss << "max_retries = " << config_.behavior.max_retries << "\n";
    ss << "timeout = " << config_.behavior.timeout << "\n";
    ss << "verbose = " << (config_.behavior.verbose ? "true" : "false") << "\n";
    ss << "stream_delay = " << config_.behavior.stream_delay << "\n\n";

    // 工具设置
    ss << "# ============================================\n";
    ss << "# 工具设置\n";
    ss << "# ============================================\n\n";

    ss << "[tools]\n";
    ss << "bash_timeout = " << config_.tools.bash_timeout << "\n";
    ss << "max_read_size = " << config_.tools.max_read_size << "\n";
    ss << "forbidden_commands = [";
    for (size_t i = 0; i < config_.tools.forbidden_commands.size(); ++i) {
        ss << "\"" << config_.tools.forbidden_commands[i] << "\"";
        if (i < config_.tools.forbidden_commands.size() - 1) ss << ", ";
    }
    ss << "]\n";

    return ss.str();
}

std::string ConfigManager::getApiKey(ProviderType provider) const {
    auto it = config_.providers.find(provider);
    if (it != config_.providers.end()) {
        return it->second.api_key;
    }
    return "";
}

std::string ConfigManager::getBaseUrl(ProviderType provider) const {
    auto it = config_.providers.find(provider);
    if (it != config_.providers.end()) {
        return it->second.base_url;
    }
    return "";
}

void ConfigManager::setProvider(ProviderType provider) {
    config_.default_config.provider = provider;
}

void ConfigManager::setModel(const std::string& model) {
    config_.default_config.model = model;
}

void ConfigManager::setApiKey(ProviderType provider, const std::string& key) {
    auto it = config_.providers.find(provider);
    if (it != config_.providers.end()) {
        it->second.api_key = key;
    }
}

std::string ConfigManager::providerToString(ProviderType type) {
    switch (type) {
        case ProviderType::ANTHROPIC: return "anthropic";
        case ProviderType::OPENAI:    return "openai";
        case ProviderType::GEMINI:    return "gemini";
        case ProviderType::DEEPSEEK:  return "deepseek";
        case ProviderType::DOUBAO:    return "doubao";
        case ProviderType::QWEN:      return "qwen";
        default:                      return "unknown";
    }
}

ProviderType ConfigManager::stringToProvider(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "anthropic") return ProviderType::ANTHROPIC;
    if (lower == "openai")    return ProviderType::OPENAI;
    if (lower == "gemini")    return ProviderType::GEMINI;
    if (lower == "deepseek")  return ProviderType::DEEPSEEK;
    if (lower == "doubao")    return ProviderType::DOUBAO;
    if (lower == "qwen")      return ProviderType::QWEN;

    return ProviderType::ANTHROPIC;  // 默认
}

ProviderType ConfigManager::typeFromString(const std::string& str) {
    return stringToProvider(str);
}

} // namespace roboclaw
