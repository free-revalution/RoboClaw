cmake_minimum_required(VERSION 3.20)
project(RoboClaw VERSION 0.1.0 LANGUAGES CXX)

# C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 跨平台设置
if(WIN32)
    set(PLATFORM_SOURCES src/utils/windows_utils.cpp)
    add_definitions(-DPLATFORM_WINDOWS)
elseif(APPLE)
    set(PLATFORM_SOURCES src/utils/macos_utils.cpp)
    add_definitions(-DPLATFORM_MACOS)
else()
    set(PLATFORM_SOURCES src/utils/linux_utils.cpp)
    add_definitions(-DPLATFORM_LINUX)
endif()

# 依赖管理（使用FetchContent）
include(FetchContent)

# 1. CPR - HTTP库
FetchContent_Declare(cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.5
)
FetchContent_MakeAvailable(cpr)

# 2. nlohmann/json - JSON库
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# 3. cpp-toml - TOML库
FetchContent_Declare(cpptoml
    GIT_REPOSITORY https://github.com/skystrife/cpptoml.git
    GIT_TAG master
)
FetchContent_MakeAvailable(cpptoml)

# 源文件（初始版本只有main.cpp）
set(SOURCES
    src/main.cpp
    ${PLATFORM_SOURCES}
)

# 头文件路径
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 可执行文件
add_executable(roboclaw ${SOURCES})

# 链接库
target_link_libraries(roboclaw
    cpr::cpr
    nlohmann_json::nlohmann_json
    cpptoml::cpptoml
)

# 编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(roboclaw PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(roboclaw PRIVATE /W4)
endif()

# 安装规则
install(TARGETS roboclaw DESTINATION bin)
