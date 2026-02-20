// HttpClient实现

#include "http_client.h"
#include <chrono>
#include <thread>
#include <sstream>

namespace roboclaw {

HttpClient::HttpClient()
    : default_timeout_(60) {
}

void HttpClient::setDefaultHeader(const std::string& key, const std::string& value) {
    default_headers_[key] = value;
}

HttpResponse HttpClient::get(const std::string& url,
                              const std::map<std::string, std::string>& headers,
                              int timeout) {
    try {
        cpr::Session session;
        session.SetUrl(url);

        // 设置超时
        int actualTimeout = timeout > 0 ? timeout : default_timeout_;
        session.SetTimeout(std::chrono::seconds(actualTimeout));

        // 设置头部
        for (const auto& pair : default_headers_) {
            session.UpdateHeader(pair.first, pair.second);
        }
        for (const auto& pair : headers) {
            session.UpdateHeader(pair.first, pair.second);
        }

        // 执行请求
        cpr::Response response = session.Get();

        // 构建响应
        HttpResponse resp;
        resp.status_code = response.status_code;
        resp.body = response.text;
        resp.success = (response.status_code >= 200 && response.status_code < 300);

        // 复制头部
        for (const auto& pair : response.header) {
            resp.headers[pair.first] = pair.second;
        }

        return resp;

    } catch (const std::exception& e) {
        return HttpResponse::error(std::string("GET请求失败: ") + e.what());
    }
}

HttpResponse HttpClient::post(const std::string& url,
                               const std::string& body,
                               const std::map<std::string, std::string>& headers,
                               int timeout) {
    try {
        cpr::Session session;
        session.SetUrl(url);
        session.SetBody(body);

        // 设置超时
        int actualTimeout = timeout > 0 ? timeout : default_timeout_;
        session.SetTimeout(std::chrono::seconds(actualTimeout));

        // 设置头部
        for (const auto& pair : default_headers_) {
            session.UpdateHeader(pair.first, pair.second);
        }
        for (const auto& pair : headers) {
            session.UpdateHeader(pair.first, pair.second);
        }

        // 执行请求
        cpr::Response response = session.Post();

        // 构建响应
        HttpResponse resp;
        resp.status_code = response.status_code;
        resp.body = response.text;
        resp.success = (response.status_code >= 200 && response.status_code < 300);

        // 复制头部
        for (const auto& pair : response.header) {
            resp.headers[pair.first] = pair.second;
        }

        return resp;

    } catch (const std::exception& e) {
        return HttpResponse::error(std::string("POST请求失败: ") + e.what());
    }
}

HttpResponse HttpClient::postJson(const std::string& url,
                                  const json& data,
                                  const std::map<std::string, std::string>& headers,
                                  int timeout) {
    // 添加Content-Type头部（如果未指定）
    auto finalHeaders = headers;
    if (finalHeaders.find("Content-Type") == finalHeaders.end()) {
        finalHeaders["Content-Type"] = "application/json";
    }

    return post(url, data.dump(), finalHeaders, timeout);
}

bool HttpClient::postStream(const std::string& url,
                             const json& data,
                             const std::map<std::string, std::string>& headers,
                             StreamCallback callback,
                             int timeout) {
    try {
        cpr::Session session;
        session.SetUrl(url);
        session.SetBody(data.dump());

        // 设置超时
        int actualTimeout = timeout > 0 ? timeout : default_timeout_;
        session.SetTimeout(std::chrono::seconds(actualTimeout));
        session.SetConnectTimeout(std::chrono::seconds(actualTimeout));

        // 设置头部
        for (const auto& pair : default_headers_) {
            session.UpdateHeader(pair.first, pair.second);
        }
        for (const auto& pair : headers) {
            session.UpdateHeader(pair.first, pair.second);
        }

        // 设置流式回调
        bool streamComplete = false;
        std::string buffer;

        session.SetWriteCallback([&](const std::string& data) -> bool {
            buffer += data;

            // 按行处理
            size_t pos = 0;
            while ((pos = buffer.find('\n')) != std::string::npos) {
                std::string line = buffer.substr(0, pos);
                buffer.erase(0, pos + 1);

                // 处理SSE格式
                if (line.find("data: ") == 0) {
                    std::string content = line.substr(6);  // 跳过 "data: "
                    if (content != "[DONE]") {
                        callback(content);
                    }
                }
            }

            return true;
        });

        // 执行请求
        cpr::Response response = session.Post();

        // 处理剩余数据
        if (!buffer.empty()) {
            if (buffer.find("data: ") == 0) {
                std::string content = buffer.substr(6);
                if (content != "[DONE]") {
                    callback(content);
                }
            }
        }

        return response.status_code >= 200 && response.status_code < 300;

    } catch (const std::exception& e) {
        callback("error: " + std::string(e.what()));
        return false;
    }
}

HttpResponse HttpClient::postWithRetry(const std::string& url,
                                       const json& data,
                                       const std::map<std::string, std::string>& headers,
                                       int maxRetries,
                                       int timeout) {
    int retryCount = 0;
    long backoffMs = 1000;  // 初始退避时间

    while (retryCount <= maxRetries) {
        HttpResponse response = postJson(url, data, headers, timeout);

        // 如果成功，直接返回
        if (response.success) {
            return response;
        }

        // 检查是否是可重试的错误
        bool retryable = (response.status_code == 429 ||  // Rate limit
                         response.status_code == 503 ||  // Service unavailable
                         response.status_code == 504 ||  // Gateway timeout
                         response.status_code == 0);     // Network error

        if (!retryable || retryCount == maxRetries) {
            return response;
        }

        // 指数退避
        retryCount++;
        std::this_thread::sleep_for(std::chrono::milliseconds(backoffMs));
        backoffMs = std::min(backoffMs * 2, 10000L);  // 最大10秒
    }

    return HttpResponse::error("请求失败，超过最大重试次数");
}

HttpResponse HttpClient::execute(cpr::Session& session, int timeout) {
    int actualTimeout = timeout > 0 ? timeout : default_timeout_;
    session.SetTimeout(std::chrono::seconds(actualTimeout));

    cpr::Response response = session.Post();

    HttpResponse resp;
    resp.status_code = response.status_code;
    resp.body = response.text;
    resp.success = (response.status_code >= 200 && response.status_code < 300);

    for (const auto& pair : response.header) {
        resp.headers[pair.first] = pair.second;
    }

    return resp;
}

} // namespace roboclaw
