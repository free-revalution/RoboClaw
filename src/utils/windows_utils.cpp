// Windows平台相关工具函数
// 后续将实现Windows特定的功能

#include <string>

using namespace std;

namespace platform {

// 获取用户主目录
string getHomeDirectory() {
    const char* home = getenv("USERPROFILE");
    if (home) {
        return string(home);
    }
    const char* drive = getenv("HOMEDRIVE");
    const char* path = getenv("HOMEPATH");
    if (drive && path) {
        return string(drive) + string(path);
    }
    return "C:\\Temp";  // 降级处理
}

// 获取配置目录
string getConfigDirectory() {
    return getHomeDirectory() + "\\.robopartner";
}

} // namespace platform
