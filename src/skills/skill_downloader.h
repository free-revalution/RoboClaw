// 技能下载器 - SkillDownloader
// 从远程仓库下载技能

#ifndef ROBOCLAW_SKILLS_SKILL_DOWNLOADER_H
#define ROBOCLAW_SKILLS_SKILL_DOWNLOADER_H

#include "../llm/http_client.h"
#include <string>
#include <vector>
#include <memory>

namespace roboclaw {

// 技能仓库信息
struct SkillRepository {
    std::string name;     // 仓库名称
    std::string url;      // 仓库URL
    bool enabled;         // 是否启用
};

// 下载进度回调
using DownloadProgressCallback = std::function<void(const std::string& skillName,
                                                     int current,
                                                     int total)>;

// 技能下载器
class SkillDownloader {
public:
    SkillDownloader(const std::string& cacheDir = ".roboclaw/skills/cache");
    ~SkillDownloader() = default;

    // 设置缓存目录
    void setCacheDir(const std::string& dir);

    // 添加仓库
    void addRepository(const SkillRepository& repo);

    // 获取所有仓库
    std::vector<SkillRepository> getRepositories() const;

    // 从URL下载单个技能
    bool downloadSkill(const std::string& url,
                       const std::string& destPath);

    // 从GitHub下载技能
    bool downloadFromGitHub(const std::string& repo,
                            const std::string& skillFile,
                            const std::string& destPath);

    // 从仓库搜索技能
    std::vector<std::string> searchSkills(const std::string& keyword);

    // 批量下载技能
    int downloadSkills(const std::vector<std::string>& urls,
                       const std::string& destDir,
                       DownloadProgressCallback callback = nullptr);

    // 清理缓存
    void clearCache();

    // 获取缓存大小
    size_t getCacheSize() const;

private:
    // 解析GitHub URL
    bool parseGitHubUrl(const std::string& url,
                        std::string& owner,
                        std::string& repo,
                        std::string& path);

    // 生成缓存键
    std::string generateCacheKey(const std::string& url) const;

    // 检查缓存是否有效
    bool isCacheValid(const std::string& cacheKey,
                      int maxAgeHours = 168) const; // 默认7天

    // 从缓存加载
    bool loadFromCache(const std::string& cacheKey,
                       std::string& content) const;

    // 保存到缓存
    void saveToCache(const std::string& cacheKey,
                     const std::string& content);

    // 执行HTTP GET请求
    bool httpGet(const std::string& url,
                 std::string& response,
                 std::string& error);

    std::string cache_dir_;
    std::vector<SkillRepository> repositories_;
    std::shared_ptr<HttpClient> http_client_;
};

} // namespace roboclaw

#endif // ROBOCLAW_SKILLS_SKILL_DOWNLOADER_H
