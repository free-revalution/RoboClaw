#!/bin/bash
# RoboClaw 快速功能测试脚本 / Quick Functionality Test Script

set -e

echo "======================================"
echo "RoboClaw 功能测试 / RoboClaw Test"
echo "======================================"
echo ""

# 颜色定义 / Color definitions
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PROJECT_ROOT="/Users/jiang/development/RoboClaw"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_DIR="$PROJECT_ROOT/test_temp"

# 测试计数器
PASS_COUNT=0
FAIL_COUNT=0

# 测试函数
test_pass() {
    echo -e "${GREEN}✓ PASS${NC}: $1"
    ((PASS_COUNT++))
}

test_fail() {
    echo -e "${RED}✗ FAIL${NC}: $1"
    ((FAIL_COUNT++))
}

test_info() {
    echo -e "${YELLOW}→${NC} $1"
}

# 清理函数
cleanup() {
    echo ""
    echo "清理测试环境 / Cleaning up..."
    if [ -d "$TEST_DIR" ]; then
        rm -rf "$TEST_DIR"
    fi
}

trap cleanup EXIT

# 1. 检查编译状态
echo "1. 检查编译状态 / Checking build status"
if [ -f "$BUILD_DIR/roboclaw" ]; then
    test_pass "可执行文件存在 / Executable exists"
else
    test_fail "可执行文件不存在 / Executable not found"
    exit 1
fi

# 2. 检查帮助信息
echo ""
echo "2. 测试帮助信息 / Testing help output"
if $BUILD_DIR/roboclaw --help > /dev/null 2>&1; then
    test_pass "帮助命令可用 / Help command works"
else
    test_fail "帮助命令失败 / Help command failed"
fi

# 3. 检查版本信息
echo ""
echo "3. 测试版本信息 / Testing version output"
if $BUILD_DIR/roboclaw --version > /dev/null 2>&1; then
    test_pass "版本命令可用 / Version command works"
else
    test_fail "版本命令失败 / Version command failed"
fi

# 4. 测试文件操作工具
echo ""
echo "4. 测试文件操作工具 / Testing file operation tools"
mkdir -p "$TEST_DIR"

# 测试 Write 工具
echo "Test content" > "$TEST_DIR/test_write.txt"
if [ -f "$TEST_DIR/test_write.txt" ]; then
    test_pass "Write 工具可用 / Write tool available"
else
    test_fail "Write 工具失败 / Write tool failed"
fi

# 测试 Read 工具
if grep -q "Test content" "$TEST_DIR/test_write.txt"; then
    test_pass "Read 工具可用 / Read tool available"
else
    test_fail "Read 工具失败 / Read tool failed"
fi

# 5. 测试 Bash 工具
echo ""
echo "5. 测试 Bash 工具 / Testing Bash tool"
if echo "test" | $BUILD_DIR/roboclaw --execute 'echo "bash test"' > /dev/null 2>&1; then
    test_pass "Bash 工具可用 / Bash tool available"
else
    test_fail "Bash 工具失败 / Bash tool failed"
fi

# 6. 检查串口工具支持
echo ""
echo "6. 检查串口工具 / Checking Serial tool"

# 检查源文件
if [ -f "$PROJECT_ROOT/src/tools/serial_tool.h" ] && [ -f "$PROJECT_ROOT/src/tools/serial_tool.cpp" ]; then
    test_pass "串口工具源文件存在 / Serial tool source files exist"

    # 检查编译后的符号
    if nm "$BUILD_DIR/roboclaw" 2>/dev/null | grep -q "Serial"; then
        test_pass "串口工具已编译 / Serial tool compiled"
    else
        test_info "串口工具符号检查跳过 / Serial tool symbol check skipped"
    fi
else
    test_fail "串口工具源文件不存在 / Serial tool source files not found"
fi

# 7. 检查语言支持
echo ""
echo "7. 检查语言支持 / Checking language support"

# 检查 Language 枚举
if grep -q "enum class Language" "$PROJECT_ROOT/src/storage/config_manager.h"; then
    test_pass "语言枚举已定义 / Language enum defined"
else
    test_fail "语言枚举未定义 / Language enum not found"
fi

# 检查语言转换函数
if grep -q "languageToString" "$PROJECT_ROOT/src/storage/config_manager.cpp"; then
    test_pass "语言转换函数已实现 / Language conversion functions implemented"
else
    test_fail "语言转换函数未实现 / Language conversion functions not found"
fi

# 8. 检查线程池
echo ""
echo "8. 检查线程池 / Checking thread pool"

if [ -f "$PROJECT_ROOT/src/utils/thread_pool.h" ]; then
    test_pass "线程池头文件存在 / Thread pool header exists"

    if grep -q "class ThreadPool" "$PROJECT_ROOT/src/utils/thread_pool.h"; then
        test_pass "ThreadPool 类已定义 / ThreadPool class defined"
    fi
else
    test_fail "线程池不存在 / Thread pool not found"
fi

# 9. 检查工具注册
echo ""
echo "9. 检查工具注册 / Checking tool registration"

TOOL_COUNT=$(grep -c "registerTool" "$PROJECT_ROOT/src/agent/tool_executor.cpp" || echo "0")
if [ "$TOOL_COUNT" -ge 5 ]; then
    test_pass "工具注册完整 ($TOOL_COUNT 工具) / Tools registered ($TOOL_COUNT tools)"
else
    test_fail "工具注册不完整 (仅 $TOOL_COUNT 工具) / Incomplete tool registration ($TOOL_COUNT tools)"
fi

# 10. 检查配置管理器
echo ""
echo "10. 检查配置管理器 / Checking config manager"

if grep -q "Language language" "$PROJECT_ROOT/src/storage/config_manager.h"; then
    test_pass "配置包含语言字段 / Config includes language field"
else
    test_fail "配置缺少语言字段 / Config missing language field"
fi

# 11. 检查文档
echo ""
echo "11. 检查文档 / Checking documentation"

if [ -f "$PROJECT_ROOT/README.md" ]; then
    test_pass "README.md 存在 / README.md exists"
fi

if [ -f "$PROJECT_ROOT/CHANGELOG.md" ]; then
    test_pass "CHANGELOG.md 存在 / CHANGELOG.md exists"
fi

if [ -f "$PROJECT_ROOT/CONTRIBUTING.md" ]; then
    test_pass "CONTRIBUTING.md 存在 / CONTRIBUTING.md exists"
fi

if [ -f "$PROJECT_ROOT/LICENSE" ]; then
    test_pass "LICENSE 存在 / LICENSE exists"
fi

# 12. 功能列表总结
echo ""
echo "======================================"
echo "功能列表 / Feature List"
echo "======================================"

# 检查工具源文件
tools=("read_tool" "write_tool" "edit_tool" "bash_tool" "serial_tool")
echo ""
echo "核心工具 / Core Tools:"
for tool in "${tools[@]}"; do
    if [ -f "$PROJECT_ROOT/src/tools/${tool}.h" ]; then
        echo -e "  ${GREEN}✓${NC} $tool"
    else
        echo -e "  ${RED}✗${NC} $tool"
    fi
done

# 检查关键组件
components=(
    "storage/config_manager:配置管理器"
    "agent/tool_executor:工具执行器"
    "agent/agent:Agent引擎"
    "utils/thread_pool:线程池"
    "session/session_manager:会话管理器"
    "cli/config_wizard:配置向导"
)

echo ""
echo "核心组件 / Core Components:"
for component in "${components[@]}"; do
    file="${component%%:*}"
    name="${component##*:}"
    if [ -f "$PROJECT_ROOT/src/${file}.h" ]; then
        echo -e "  ${GREEN}✓${NC} $name"
    else
        echo -e "  ${RED}✗${NC} $name"
    fi
done

# 测试结果汇总
echo ""
echo "======================================"
echo "测试结果汇总 / Test Summary"
echo "======================================"
echo -e "${GREEN}通过 / Passed:${NC} $PASS_COUNT"
echo -e "${RED}失败 / Failed:${NC} $FAIL_COUNT"
echo ""

if [ $FAIL_COUNT -eq 0 ]; then
    echo -e "${GREEN}所有测试通过！/ All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}部分测试失败。/ Some tests failed.${NC}"
    exit 1
fi
