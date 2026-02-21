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

# 2. nlohmann/json - JSON库 (使用系统包管理器安装)
find_package(nlohmann_json 3.11.3 REQUIRED)

# 源文件
set(SOURCES
    # 主程序
    src/main.cpp

    # 工具模块
    src/tools/tool_base.cpp
    src/tools/read_tool.cpp
    src/tools/write_tool.cpp
    src/tools/edit_tool.cpp
    src/tools/bash_tool.cpp

    # Agent模块
    src/agent/tool_executor.cpp
    src/agent/prompt_builder.cpp
    src/agent/agent.cpp

    # LLM模块
    src/llm/http_client.cpp
    src/llm/anthropic_provider.cpp
    src/llm/openai_provider.cpp

    # Session模块
    src/session/conversation_node.cpp
    src/session/conversation_tree.cpp
    src/session/session_manager.cpp

    # Token优化模块
    src/optimization/token_optimizer.cpp
    src/optimization/conversation_compressor.cpp
    src/optimization/token_budget.cpp

    # 技能模块
    src/skills/skill_parser.cpp
    src/skills/skill_registry.cpp
    src/skills/skill_executor.cpp
    src/skills/skill_downloader.cpp

    # 工具类
    src/utils/logger.cpp
    src/utils/thread_pool.cpp

    # 存储模块
    src/storage/config_manager.cpp

    # CLI模块
    src/cli/config_wizard.cpp
    src/cli/interactive_mode.cpp
    src/cli/skill_commands.cpp

    # 平台相关
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
)

# 编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(roboclaw PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(roboclaw PRIVATE /W4)
endif()

# 安装规则
install(TARGETS roboclaw DESTINATION bin)
