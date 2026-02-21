// HttpClient实现

#include "http_client.h"
#include <chrono>
#include <thread>
#include <sstream>
#include <tuple>

namespace roboclaw {

HttpClient::HttpClient()
    : default_timeout_(60)
    , active_async_requests_(0) {
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
        session.SetTimeout(cpr::Timeout{std::chrono::milliseconds(actualTimeout * 1000)});

        // 设置头部
        cpr::Header header;
        for (const auto& pair : default_headers_) {
            header[pair.first] = pair.second;
        }
        for (const auto& pair : headers) {
            header[pair.first] = pair.second;
        }
        session.SetHeader(header);

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
        session.SetTimeout(cpr::Timeout{std::chrono::milliseconds(actualTimeout * 1000)});

        // 设置头部
        cpr::Header header;
        for (const auto& pair : default_headers_) {
            header[pair.first] = pair.second;
        }
        for (const auto& pair : headers) {
            header[pair.first] = pair.second;
        }
        session.SetHeader(header);

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
    // 暂时简化：使用非流式方式，然后一次性回调
    try {
        HttpResponse response = postJson(url, data, headers, timeout);
        if (response.success) {
            // 简单的 SSE 数据模拟
            callback(response.body);
            return true;
        }
        return false;
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

// ==================== 异步请求实现 ====================

std::future<HttpResponse> HttpClient::getAsync(const std::string& url,
                                                const std::map<std::string, std::string>& headers,
                                                int timeout) {
    active_async_requests_++;

    return std::async(std::launch::async, [this, url, headers, timeout]() {
        HttpResponse response = get(url, headers, timeout);
        active_async_requests_--;
        return response;
    });
}

std::future<HttpResponse> HttpClient::postAsync(const std::string& url,
                                                 const std::string& body,
                                                 const std::map<std::string, std::string>& headers,
                                                 int timeout) {
    active_async_requests_++;

    return std::async(std::launch::async, [this, url, body, headers, timeout]() {
        HttpResponse response = post(url, body, headers, timeout);
        active_async_requests_--;
        return response;
    });
}

std::future<HttpResponse> HttpClient::postJsonAsync(const std::string& url,
                                                     const json& data,
                                                     const std::map<std::string, std::string>& headers,
                                                     int timeout) {
    active_async_requests_++;

    return std::async(std::launch::async, [this, url, data, headers, timeout]() {
        HttpResponse response = postJson(url, data, headers, timeout);
        active_async_requests_--;
        return response;
    });
}

void HttpClient::postAsyncCallback(const std::string& url,
                                    const std::string& body,
                                    const std::map<std::string, std::string>& headers,
                                    std::function<void(const HttpResponse&)> callback,
                                    int timeout) {
    active_async_requests_++;

    std::thread([this, url, body, headers, callback, timeout]() {
        HttpResponse response = post(url, body, headers, timeout);
        active_async_requests_--;
        callback(response);
    }).detach();
}

void HttpClient::postJsonAsyncCallback(const std::string& url,
                                       const json& data,
                                       const std::map<std::string, std::string>& headers,
                                       std::function<void(const HttpResponse&)> callback,
                                       int timeout) {
    active_async_requests_++;

    std::thread([this, url, data, headers, callback, timeout]() {
        HttpResponse response = postJson(url, data, headers, timeout);
        active_async_requests_--;
        callback(response);
    }).detach();
}

std::vector<std::future<HttpResponse>> HttpClient::postBatchAsync(
    const std::vector<std::tuple<std::string, json, std::map<std::string, std::string>>>& requests) {

    std::vector<std::future<HttpResponse>> futures;
    futures.reserve(requests.size());

    for (const auto& request : requests) {
        const auto& url = std::get<0>(request);
        const auto& data = std::get<1>(request);
        const auto& headers = std::get<2>(request);

        futures.push_back(postJsonAsync(url, data, headers));
    }

    return futures;
}

void HttpClient::cancelAllAsync() {
    // CPR库不支持真正取消正在进行的请求
    // 这里我们只能减少计数器并记录日志
    int active = active_async_requests_.load();
    if (active > 0) {
        // 注意：实际请求仍在执行，只是我们不再追踪它们
        active_async_requests_ = 0;
    }
}

} // namespace roboclaw
