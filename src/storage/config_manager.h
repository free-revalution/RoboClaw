// 配置管理器 - ConfigManager
// 负责加载、保存、验证RoboClaw配置文件

#ifndef ROBOCLAW_STORAGE_CONFIG_MANAGER_H
#define ROBOCLAW_STORAGE_CONFIG_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <memory>

namespace roboclaw {

// LLM提供商类型
enum class ProviderType {
    ANTHROPIC,
    OPENAI,
    GEMINI,
    DEEPSEEK,
    DOUBAO,
    QWEN
};

// 提供商信息
struct ProviderInfo {
    ProviderType type;
    std::string name;
    std::string api_key;
    std::string base_url;
    std::vector<std::string> models;
};

// 默认配置
struct DefaultConfig {
    ProviderType provider;
    std::string model;
};

// 行为设置
struct BehaviorConfig {
    int max_retries;
    int timeout;
    bool verbose;
    int stream_delay;
};

// 工具设置
struct ToolsConfig {
    int bash_timeout;
    std::vector<std::string> forbidden_commands;
    int max_read_size;
};

// 配置类
class Config {
public:
    Config() = default;

    DefaultConfig default_config;
    std::map<ProviderType, ProviderInfo> providers;
    BehaviorConfig behavior;
    ToolsConfig tools;

    // 获取当前提供商信息
    ProviderInfo* getCurrentProvider();
    const ProviderInfo* getCurrentProvider() const;

    // 验证配置
    bool validate() const;
    std::string getValidationError() const;
};

// 配置管理器
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager() = default;

    // 获取配置文件路径
    static std::string getConfigPath();

    // 获取配置目录路径
    static std::string getConfigDir();

    // 检查配置文件是否存在
    static bool configExists();

    // 加载配置
    bool load();
    bool load(const std::string& path);

    // 保存配置
    bool save();
    bool save(const std::string& path);

    // 获取配置
    const Config& getConfig() const { return config_; }
    Config& getConfig() { return config_; }

    // 便捷访问方法
    ProviderType getProvider() const { return config_.default_config.provider; }
    std::string getModel() const { return config_.default_config.model; }
    std::string getApiKey(ProviderType provider) const;
    std::string getBaseUrl(ProviderType provider) const;

    // 设置方法
    void setProvider(ProviderType provider);
    void setModel(const std::string& model);
    void setApiKey(ProviderType provider, const std::string& key);

    // 提供商类型转换
    static std::string providerToString(ProviderType type);
    static ProviderType stringToProvider(const std::string& str);
    static ProviderType typeFromString(const std::string& str);

    // 初始化默认配置
    void initializeDefaults();

private:
    Config config_;

    // 解析TOML文件
    bool parseToml(const std::string& content);

    // 生成TOML内容
    std::string generateToml() const;

    // 初始化提供商信息
    void initializeProviderInfo(ProviderType type);
};

} // namespace roboclaw

#endif // ROBOCLAW_STORAGE_CONFIG_MANAGER_H
