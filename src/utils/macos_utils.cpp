// macOS平台相关工具函数
// 后续将实现macOS特定的功能

#include <string>

using namespace std;

namespace platform {

// 获取用户主目录
string getHomeDirectory() {
    const char* home = getenv("HOME");
    if (home) {
        return string(home);
    }
    return "/tmp";  // 降级处理
}

// 获取配置目录
string getConfigDirectory() {
    return getHomeDirectory() + "/.robopartner";
}

} // namespace platform
