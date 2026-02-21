// SkillDownloader实现

#include "skill_downloader.h"
#include "../utils/logger.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <regex>
#include <algorithm>
#include <chrono>

namespace roboclaw {

SkillDownloader::SkillDownloader(const std::string& cacheDir)
    : cache_dir_(cacheDir)
    , http_client_(std::make_shared<HttpClient>()) {

    // 创建缓存目录
    try {
        std::filesystem::create_directories(cache_dir_);
    } catch (...) {
        LOG_WARNING("无法创建缓存目录: " + cache_dir_);
    }
}

void SkillDownloader::setCacheDir(const std::string& dir) {
    cache_dir_ = dir;

    try {
        std::filesystem::create_directories(cache_dir_);
    } catch (...) {
        LOG_WARNING("无法创建缓存目录: " + cache_dir_);
    }
}

void SkillDownloader::addRepository(const SkillRepository& repo) {
    repositories_.push_back(repo);
    LOG_INFO("添加技能仓库: " + repo.name + " (" + repo.url + ")");
}

std::vector<SkillRepository> SkillDownloader::getRepositories() const {
    return repositories_;
}

bool SkillDownloader::downloadSkill(const std::string& url,
                                     const std::string& destPath) {
    // 检查缓存
    std::string cacheKey = generateCacheKey(url);
    std::string content;

    if (isCacheValid(cacheKey)) {
        if (loadFromCache(cacheKey, content)) {
            LOG_INFO("从缓存加载: " + url);
        } else {
            LOG_WARNING("缓存加载失败，重新下载");
        }
    }

    // 如果缓存无效，下载
    if (content.empty()) {
        std::string error;
        if (!httpGet(url, content, error)) {
            LOG_ERROR("下载失败: " + url + " - " + error);
            return false;
        }

        // 保存到缓存
        saveToCache(cacheKey, content);
    }

    // 写入目标文件
    try {
        // 确保目标目录存在
        std::filesystem::path dest(destPath);
        std::filesystem::create_directories(dest.parent_path());

        std::ofstream file(destPath);
        if (!file.is_open()) {
            LOG_ERROR("无法创建文件: " + destPath);
            return false;
        }

        file << content;
        file.close();

        LOG_INFO("技能已下载: " + destPath);
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("写入文件失败: " + std::string(e.what()));
        return false;
    }
}

bool SkillDownloader::downloadFromGitHub(const std::string& repo,
                                          const std::string& skillFile,
                                          const std::string& destPath) {
    // 解析仓库
    std::string owner, repoName, path;
    if (!parseGitHubUrl(repo, owner, repoName, path)) {
        LOG_ERROR("无效的GitHub URL: " + repo);
        return false;
    }

    // 构建原始文件URL
    std::string rawUrl = "https://raw.githubusercontent.com/" +
                         owner + "/" + repoName + "/main/" +
                         skillFile;

    // 如果path不为空，追加到路径
    if (!path.empty()) {
        rawUrl = "https://raw.githubusercontent.com/" +
                 owner + "/" + repoName + "/" + path + "/" +
                 skillFile;
    }

    return downloadSkill(rawUrl, destPath);
}

std::vector<std::string> SkillDownloader::searchSkills(const std::string& keyword) {
    std::vector<std::string> results;

    // TODO: 实现远程搜索
    // 这需要仓库提供搜索API或索引文件

    LOG_INFO("搜索技能: " + keyword);
    return results;
}

int SkillDownloader::downloadSkills(const std::vector<std::string>& urls,
                                     const std::string& destDir,
                                     DownloadProgressCallback callback) {
    int downloaded = 0;
    int total = static_cast<int>(urls.size());

    for (int i = 0; i < total; ++i) {
        const std::string& url = urls[i];

        // 从URL提取文件名
        std::string filename = url;
        size_t lastSlash = url.find_last_of('/');
        if (lastSlash != std::string::npos) {
            filename = url.substr(lastSlash + 1);
        }

        std::string destPath = destDir + "/" + filename;

        if (downloadSkill(url, destPath)) {
            downloaded++;
        }

        // 回调进度
        if (callback) {
            callback(filename, i + 1, total);
        }
    }

    LOG_INFO("批量下载完成: " + std::to_string(downloaded) + "/" +
             std::to_string(total));

    return downloaded;
}

void SkillDownloader::clearCache() {
    try {
        if (std::filesystem::exists(cache_dir_)) {
            size_t count = 0;
            for (const auto& entry : std::filesystem::directory_iterator(cache_dir_)) {
                if (entry.is_regular_file()) {
                    std::filesystem::remove(entry.path());
                    count++;
                }
            }
            LOG_INFO("已清理缓存: " + std::to_string(count) + " 个文件");
        }
    } catch (const std::exception& e) {
        LOG_ERROR("清理缓存失败: " + std::string(e.what()));
    }
}

size_t SkillDownloader::getCacheSize() const {
    size_t totalSize = 0;

    try {
        if (std::filesystem::exists(cache_dir_)) {
            for (const auto& entry : std::filesystem::directory_iterator(cache_dir_)) {
                if (entry.is_regular_file()) {
                    totalSize += entry.file_size();
                }
            }
        }
    } catch (...) {
        // 忽略错误
    }

    return totalSize;
}

bool SkillDownloader::parseGitHubUrl(const std::string& url,
                                      std::string& owner,
                                      std::string& repo,
                                      std::string& path) {
    // 支持格式：
    // github.com/owner/repo
    // github.com/owner/repo/path/to/skills

    std::regex pattern(R"(github\.com/([^/]+)/([^/]+)(/(.*))?)");
    std::smatch match;

    if (std::regex_search(url, match, pattern)) {
        owner = match[1].str();
        repo = match[2].str();
        path = match[4].str();
        return true;
    }

    return false;
}

std::string SkillDownloader::generateCacheKey(const std::string& url) const {
    // 简单哈希：使用URL的文件名部分
    std::string filename = url;
    size_t lastSlash = url.find_last_of('/');
    if (lastSlash != std::string::npos) {
        filename = url.substr(lastSlash + 1);
    }

    // 添加扩展名.cache
    return filename + ".cache";
}

bool SkillDownloader::isCacheValid(const std::string& cacheKey,
                                    int maxAgeHours) const {
    std::string cachePath = cache_dir_ + "/" + cacheKey;

    if (!std::filesystem::exists(cachePath)) {
        return false;
    }

    try {
        auto ftime = std::filesystem::last_write_time(cachePath);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now()
                     + std::chrono::system_clock::now());
        auto age = std::chrono::system_clock::now() - sctp;
        auto ageHours = std::chrono::duration_cast<std::chrono::hours>(age).count();

        return ageHours < maxAgeHours;

    } catch (...) {
        return false;
    }
}

bool SkillDownloader::loadFromCache(const std::string& cacheKey,
                                     std::string& content) const {
    std::string cachePath = cache_dir_ + "/" + cacheKey;

    try {
        std::ifstream file(cachePath);
        if (!file.is_open()) {
            return false;
        }

        std::string str((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
        file.close();

        content = str;
        return true;

    } catch (...) {
        return false;
    }
}

void SkillDownloader::saveToCache(const std::string& cacheKey,
                                   const std::string& content) {
    std::string cachePath = cache_dir_ + "/" + cacheKey;

    try {
        std::ofstream file(cachePath);
        if (!file.is_open()) {
            LOG_WARNING("无法创建缓存文件: " + cachePath);
            return;
        }

        file << content;
        file.close();

    } catch (const std::exception& e) {
        LOG_WARNING("保存缓存失败: " + std::string(e.what()));
    }
}

bool SkillDownloader::httpGet(const std::string& url,
                               std::string& response,
                               std::string& error) {
    try {
        // 使用HttpClient进行GET请求
        HttpResponse httpResponse = http_client_->get(url);

        if (httpResponse.success) {
            response = httpResponse.body;
            return true;
        } else {
            error = httpResponse.error_message;
            return false;
        }

    } catch (const std::exception& e) {
        error = std::string("HTTP请求异常: ") + e.what();
        return false;
    }
}

} // namespace roboclaw
