# RoboClaw Tests CMake Configuration

cmake_minimum_required(VERSION 3.20)
project(RoboClawTests)

# C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 跨平台设置
if(WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
elseif(APPLE)
    add_definitions(-DPLATFORM_MACOS)
else()
    add_definitions(-DPLATFORM_LINUX)
endif()

# 查找依赖
find_package(nlohmann_json 3.11.3 REQUIRED)

# Google Test (for existing tests)
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
# 对于 Windows：防止覆盖父项目的编译器/链接器设置
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Catch2 (for plugin tests)
FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(catch2)

# 包含主项目头文件
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

# 测试源文件
set(TEST_SOURCES
    unit/test_config_manager.cpp
    unit/test_tools.cpp
    unit/test_thread_pool.cpp
    unit/test_language.cpp
    unit/test_motor_controller_interface.cpp
    unit/test_sensor_interface.cpp
    unit/test_comm_interface.cpp
    unit/test_hal_exception.cpp
    unit/test_serial_comm.cpp
    unit/test_motion_skill.cpp
    unit/test_sensor_skill.cpp
    unit/test_hardware_config.cpp
    unit/test_social_message.cpp
    unit/test_task_coordinator.cpp
    unit/test_agent_bridge.cpp
    unit/test_claude_code_bridge.cpp
    plugins/test_plugin_interface.cpp
    integration/test_hardware_cli.cpp
    integration/test_link_command.cpp
    integration/test_social_manager.cpp
    e2e/test_social_link_flow.cpp
)

# 主项目源文件（用于链接）
set(MAIN_SOURCES
    ../src/storage/config_manager.cpp
    ../src/tools/tool_base.cpp
    ../src/tools/read_tool.cpp
    ../src/tools/write_tool.cpp
    ../src/tools/edit_tool.cpp
    ../src/tools/bash_tool.cpp
    ../src/tools/serial_tool.cpp
    ../src/utils/logger.cpp
    ../src/utils/thread_pool.cpp
    ../src/agent/tool_executor.cpp
    ../src/agent/prompt_builder.cpp
    ../src/agent/task_coordinator.cpp
    ../src/hal/drivers/serial_comm.cpp
    ../src/hal/hardware_config.cpp
    ../src/skills/robot/motion_skill.cpp
    ../src/skills/robot/sensor_skill.cpp
    ../src/cli/link_command.cpp
    ../src/social/telegram_adapter.cpp
    ../src/social/social_manager.cpp
    ../src/agent/claude_code_bridge.cpp
)

# 链接CPR（如果需要）
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../build/_deps/cpr-src/include)
# link_directories(${CMAKE_CURRENT_BINARY_DIR}/../build/_deps/cpr-build)

# 创建测试可执行文件
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source} ${MAIN_SOURCES})

    # Plugin tests use Catch2, others use GoogleTest
    if(${test_source} MATCHES "plugins/")
        target_link_libraries(${test_name}
            Catch2::Catch2WithMain
            nlohmann_json::nlohmann_json
        )
    else()
        target_link_libraries(${test_name}
            gtest
            gtest_main
            nlohmann_json::nlohmann_json
        )
    endif()

    # Windows 需要的库
    if(WIN32)
        target_link_libraries(${test_name} setupapi)
    endif()

    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# 添加一个运行所有测试的便捷目标
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ${TEST_EXECUTABLES}
    COMMENT "Running all tests..."
)
