# RoboPartner 测试文档 / Test Documentation

## 测试概述 / Test Overview

RoboPartner 项目包含三种类型的测试：

### 1. 单元测试 (Unit Tests)
位置：`tests/unit/`

- **test_config_manager.cpp** - 配置管理器测试
  - 语言转换功能
  - 提供商转换功能
  - 配置保存和加载
  - 配置验证
  - TOML 生成和解析

- **test_tools.cpp** - 工具测试
  - Read 工具功能
  - Write 工具功能
  - Edit 工具功能
  - Bash 工具功能
  - 工具描述和参数验证
  - 工具注册表
  - 错误处理

- **test_thread_pool.cpp** - 线程池测试
  - 基本任务提交
  - 带返回值的任务
  - 并发性能测试
  - 统计信息
  - 动态调整
  - 延迟任务
  - 停止和清理
  - 配置管理

- **test_language.cpp** - 语言支持测试
  - 语言枚举值
  - 字符串转换
  - 配置持久化
  - TOML 解析
  - 语言与提供商配置独立性

### 2. 集成测试 (Integration Tests)
位置：`tests/integration/`

- **test_integration.cpp** - 组件集成测试
  - 配置管理器与工具执行器集成
  - 多工具工作流
  - 并发工具执行
  - Bash 工具与文件操作集成
  - 配置持久化
  - 语言切换集成
  - 工具描述检索
  - 错误处理
  - 工具注册表完整性

### 3. 端到端测试 (E2E Tests)
位置：`tests/e2e/`

端到端测试用于验证完整的用户场景。

## 运行测试 / Running Tests

### 构建测试 / Build Tests

```bash
# 创建测试构建目录
mkdir -p build/tests
cd build/tests

# 配置测试
cmake ../../tests

# 构建测试
cmake --build .

# 或者使用 ninja (如果可用)
ninja
```

### 运行所有测试 / Run All Tests

```bash
# 使用 CTest
cd build/tests
ctest --output-on-failure

# 或者运行单个测试
./test_config_manager
./test_tools
./test_thread_pool
./test_language
./test_integration
```

### 运行特定测试 / Run Specific Test

```bash
# 运行特定测试用例
./test_config_manager --gtest_filter=ConfigManagerTest.LanguageConversion

# 运行所有测试并显示详细输出
./test_tools --gtest_verbose
```

## 测试覆盖率 / Test Coverage

### 当前测试覆盖

| 模块 | 单元测试 | 集成测试 | 覆盖率估计 |
|------|---------|---------|-----------|
| ConfigManager | ✅ | ✅ | ~85% |
| Tools (Read/Write/Edit/Bash) | ✅ | ✅ | ~80% |
| SerialTool | ⚠️ | ⚠️ | ~30% |
| BrowserTool | ⚠️ | ⚠️ | ~30% |
| AgentTool | ⚠️ | ⚠️ | ~30% |
| ThreadPool | ✅ | ✅ | ~75% |
| Language Support | ✅ | ✅ | ~90% |
| ToolExecutor | ⚠️ | ✅ | ~60% |

图例：✅ 完整覆盖 | ⚠️ 部分覆盖 | ❌ 未覆盖

### 未覆盖的功能 / Not Covered

- 串口工具的完整功能（需要硬件）
- 浏览器自动化工具的完整功能（需要浏览器驱动）
- Agent 发现工具的完整功能（需要本地 agents）
- LLM 提供商集成（需要 API 密钥）
- 会话管理器（部分）
- 技能系统（部分）
- Token 优化器（部分）

## 添加新测试 / Adding New Tests

### 创建新的单元测试

1. 在 `tests/unit/` 创建新文件
2. 包含必要的头文件
3. 继承 `::testing::Test` 类
4. 编写测试用例（使用 `TEST_F` 宏）

```cpp
#include <gtest/gtest.h>
#include "../../src/your_module.h"

using namespace robopartner;

class YourModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 测试初始化
    }

    void TearDown() override {
        // 测试清理
    }
};

TEST_F(YourModuleTest, YourTestCase) {
    EXPECT_EQ(expected, actual);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

### 更新 CMakeLists.txt

在 `tests/CMakeLists.txt` 中添加新的测试源文件：

```cmake
set(TEST_SOURCES
    unit/test_config_manager.cpp
    unit/test_tools.cpp
    unit/test_your_new_test.cpp  # 添加这里
    ...
)
```

## 持续集成 / Continuous Integration

测试应该在以下情况下运行：

1. 每次提交前
2. Pull Request 创建时
3. 合并到主分支前

### GitHub Actions 示例

```yaml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]

    steps:
    - uses: actions/checkout@v2

    - name: Install dependencies
      run: |
        # Ubuntu
        sudo apt-get install -y cmake ninja-build nlohmann-json3-dev
        # macOS
        brew install cmake ninja nlohmann-json

    - name: Build tests
      run: |
        mkdir build && cd build
        cmake ../tests
        cmake --build .

    - name: Run tests
      run: |
        cd build
        ctest --output-on-failure
```

## 测试最佳实践 / Testing Best Practices

1. **独立性**：每个测试应该独立运行，不依赖其他测试
2. **可重复性**：测试应该产生相同的结果
3. **快速**：单元测试应该快速执行
4. **清晰**：测试名称和断言应该清晰表达意图
5. **边界条件**：测试正常和边界情况
6. **错误处理**：测试错误路径和异常情况

## 已知问题 / Known Issues

1. **串口测试**：需要实际硬件或更好的模拟
2. **LLM 集成测试**：需要 mock 或测试 API
3. **Windows 平台测试**：某些测试可能需要调整

## 贡献 / Contributing

添加新功能时，请同时添加：

1. 单元测试覆盖新功能
2. 集成测试验证组件交互
3. 更新此文档
