// HTTP客户端 - HttpClient
// 基于cpr库的HTTP客户端封装

#ifndef ROBOCLAW_LLM_HTTP_CLIENT_H
#define ROBOCLAW_LLM_HTTP_CLIENT_H

#include <string>
#include <functional>
#include <memory>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace roboclaw {

// HTTP响应
struct HttpResponse {
    int status_code;
    std::string body;
    std::string error;
    bool success;

    // 头部
    std::map<std::string, std::string> headers;

    HttpResponse() : status_code(0), success(false) {}

    static HttpResponse ok(int status, const std::string& body,
                           const std::map<std::string, std::string>& headers = {}) {
        HttpResponse resp;
        resp.status_code = status;
        resp.body = body;
        resp.headers = headers;
        resp.success = (status >= 200 && status < 300);
        return resp;
    }

    static HttpResponse error(const std::string& error) {
        HttpResponse resp;
        resp.error = error;
        resp.success = false;
        return resp;
    }

    json toJson() const {
        json j;
        j["status_code"] = status_code;
        j["body"] = body;
        j["success"] = success;
        if (!error.empty()) {
            j["error"] = error;
        }
        return j;
    }
};

// 流式响应回调
using StreamCallback = std::function<void(const std::string& chunk)>;

// HTTP客户端
class HttpClient {
public:
    HttpClient();
    ~HttpClient() = default;

    // 设置默认超时
    void setTimeout(int seconds) { default_timeout_ = seconds; }

    // 设置默认头部
    void setDefaultHeader(const std::string& key, const std::string& value);

    // GET请求
    HttpResponse get(const std::string& url,
                     const std::map<std::string, std::string>& headers = {},
                     int timeout = 0);

    // POST请求
    HttpResponse post(const std::string& url,
                      const std::string& body,
                      const std::map<std::string, std::string>& headers = {},
                      int timeout = 0);

    // POST请求（JSON）
    HttpResponse postJson(const std::string& url,
                          const json& data,
                          const std::map<std::string, std::string>& headers = {},
                          int timeout = 0);

    // 流式POST请求
    bool postStream(const std::string& url,
                    const json& data,
                    const std::map<std::string, std::string>& headers,
                    StreamCallback callback,
                    int timeout = 0);

    // 带重试的POST请求
    HttpResponse postWithRetry(const std::string& url,
                               const json& data,
                               const std::map<std::string, std::string>& headers,
                               int maxRetries = 3,
                               int timeout = 0);

private:
    int default_timeout_;
    std::map<std::string, std::string> default_headers_;

    // 执行请求（带超时）
    HttpResponse execute(cpr::Session& session, int timeout);
};

} // namespace roboclaw

#endif // ROBOCLAW_LLM_HTTP_CLIENT_H
