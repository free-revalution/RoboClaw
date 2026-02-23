# RoboPartner 重命名为 RoboClaw 实施计划

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task.

**目标**: 将 RoboPartner 全面重命名为 RoboClaw，包括可执行文件名、配置目录、日志文件，同时优化代码质量

**架构**: 分阶段执行：1) 配置和常量重命名 2) 代码文本重命名 3) 文档更新 4) 代码质量优化

**技术栈**: C++20, CMake, Shell 脚本, 正则表达式替换

---

## 第一阶段：配置和常量重命名

### Task 1: 重命名 CMakeLists.txt 中的项目名称

**Files:**
- Modify: `CMakeLists.txt`

**Step 1: 更新项目名称和可执行文件名**

在 CMakeLists.txt 中：
- `project(RoboPartner)` → `project(RoboClaw)`
- `add_executable(robopartner ...)` → `add_executable(roboclaw ...)`
- 所有 `robopartner` 引用 → `roboclaw`

**Step 2: 运行测试验证**

Run: `cd build && cmake .. && make`
Expected: 编译成功，生成 `roboclaw` 可执行文件

**Step 3: 提交**

```bash
git add CMakeLists.txt
git commit -m "refactor: rename project from RoboPartner to RoboClaw

- Update project name in CMakeLists.txt
- Change executable name from robopartner to roboclaw
- Update all related references

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"
```

---

### Task 2: 重命名配置目录和日志文件

**Files:**
- Modify: `src/storage/config_manager.h/cpp`
- Modify: `src/session/session_manager.h/cpp`
- Modify: `src/utils/logger.h/cpp`

**Step 1: 更新配置目录常量**

`config_manager.h/cpp`:
- `.robopartner` → `.roboclaw`

`session_manager.h/cpp`:
- `.robopartner/conversations` → `.roboclaw/conversations`
- `.robopartner/sessions` → `.roboclaw/sessions` (如有)

`logger.h/cpp`:
- `robopartner.log` → `roboclaw.log`

**Step 2: 更新 .gitignore**

Modify: `.gitignore`
```
# RoboPartner运行时数据
.robopartner/
```
→
```
# RoboClaw运行时数据
.roboclaw/
```

**Step 3: 提交**

```bash
git add src/storage/config_manager.h src/storage/config_manager.cpp
git add src/session/session_manager.h src/session/session_manager.cpp
git add src/utils/logger.h src/utils/logger.cpp .gitignore
git commit -m "refactor: rename config directories and log files

- Config dir: .robopartner → .roboclaw
- Conversations dir path update
- Log file: robopartner.log → roboclaw.log
- Update .gitignore

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"
```

---

### Task 3: 重命名环境变量和脚本

**Files:**
- Modify: `scripts/install`
- Modify: `scripts/install.ps1`
- Modify: `src/utils/linux_utils.cpp`
- Modify: `src/utils/macos_utils.cpp`
- Modify: `src/utils/windows_utils.cpp`

**Step 1: 更新安装脚本**

`s/install`:
```bash
- BIN_NAME="robopartner"
+ BIN_NAME="roboclaw"
- CONFIG_DIR=".robopartner"
+ CONFIG_DIR=".roboclaw"
```

`s/install.ps1`: 类似更改

**Step 2: 更新平台工具中的路径**

`linux_utils.cpp`, `macos_utils.cpp`, `windows_utils.cpp`:
- `~/.robopartner` → `~/.roboclaw`

**Step 3: 提交**

```bash
git add scripts/install scripts/install.ps1
git add src/utils/linux_utils.cpp src/utils/macos_utils.cpp src/utils/windows_utils.cpp
git commit -m "refactor: rename scripts and platform utilities

- Update install scripts to use roboclaw
- Update config paths in platform utilities

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"
```

---

## 第二阶段：代码文本重命名

### Task 4: 重命名 CLI 和终端输出

**Files:**
- Modify: `src/cli/*.cpp`
- Modify: `src/utils/terminal.cpp`

**Step 1: 更新欢迎消息和帮助文本**

`cli/interactive_mode.cpp`, `cli/config_wizard.cpp`, `cli/skill_commands.cpp`, `cli/link_command.cpp`:
- "RoboPartner" → "RoboClaw"
- "robopartner" → "roboclaw"

`utils/terminal.cpp`:
- 所有用户可见的输出文本

**Step 2: 提交**

```bash
git add src/cli/interactive_mode.cpp src/cli/config_wizard.cpp
git add src/cli/skill_commands.cpp src/cli/link_command.cpp
git add src/utils/terminal.cpp
git commit -m "refactor: rename CLI and terminal output text

- Update all user-facing text from RoboPartner to RoboClaw
- Fix help messages and prompts

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"
```

---

### Task 5: 重命名错误消息和日志

**Files:**
- Modify: `src/agent/*.cpp`
- Modify: `src/skills/*.cpp`
- Modify: `src/social/*.cpp`
- Modify: 所有其他源文件

**Step 1: 批量替换源文件中的文本**

使用 sed 或手动替换：
- 错误消息中的 "RoboPartner" → "RoboClaw"
- 日志消息中的 "robopartner" → "roboclaw"

**Step 2: 提交**

```bash
git add src/
git commit -m "refactor: rename error messages and log output

- Update all error and log messages to use RoboClaw/roboclaw
- Cover agent, skills, social, tools, llm, session modules

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"
```

---

## 第三阶段：文档更新

### Task 6: 更新主要文档

**Files:**
- Modify: `README.md`
- Modify: `CHANGELOG.md`
- Modify: `文档.md`
- Modify: `docs/social-link-guide.md`
- Modify: `docs/embedded-quickstart.md`

**Step 1: 更新 README.md**

```markdown
- 项目标题和描述
- 所有命令示例 (robopartner → roboclaw)
- 配置路径 (.robopartner → .roboclaw)
```

**Step 2: 更新 CHANGELOG.md**

在顶部添加新条目：
```markdown
## [0.4.1] - 2025-02-23

### Changed / 更改
- **Project renamed** from "RoboPartner" back to "RoboClaw" / 项目从 "RoboPartner" 更名为 "RoboClaw"
- **Executable renamed** from `robopartner` to `roboclaw` / 可执行文件从 `robopartner` 更名为 `roboclaw`
- **Config directory** changed from `.robopartner` to `.roboclaw` / 配置目录从 `.robopartner` 改为 `.roboclaw`
- **Log file** changed from `robopartner.log` to `roboclaw.log` / 日志文件从 `robopartner.log` 改为 `roboclaw.log`
```

**Step 3: 更新其他文档**

`文档.md`, `docs/social-link-guide.md`, `docs/embedded-quickstart.md`:
- 所有提及 RoboPartner 的地方改为 RoboClaw

**Step 4: 提交**

```bash
git add README.md CHANGELOG.md 文档.md
git add docs/social-link-guide.md docs/embedded-quickstart.md
git commit -m "docs: update documentation for RoboClaw rename

- Update README with new project name and executable
- Add v0.4.1 changelog entry
- Update all Chinese and English documentation

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"
```

---

### Task 7: 更新测试文件

**Files:**
- Modify: `tests/**/*.cpp`

**Step 1: 更新测试中的文本**

`tests/unit/*.cpp`, `tests/integration/*.cpp`, `tests/e2e/*.cpp`:
- 测试消息中的 "RoboPartner" → "RoboClaw"
- 测试路径中的 ".robopartner" → ".roboclaw"

**Step 2: 提交**

```bash
git add tests/
git commit -m "test: update test files for RoboClaw rename

- Update test messages and assertions
- Fix config paths in tests

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"
```

---

## 第四阶段：代码质量优化

### Task 8: 内存安全与资源管理

**Files:**
- Review: 所有涉及动态内存的源文件
- Focus: `src/agent/`, `src/social/`, `src/tools/`, `src/hal/`

**Step 1: 检查智能指针使用**

审查以下问题：
- 是否有裸指针没有对应的 unique_ptr/shared_ptr？
- 是否有手动 new/delete 需要转换为智能指针？
- 循环引用风险？

**Step 2: 检查 RAII 模式**

- 所有资源（文件、串口、网络连接）是否使用 RAII？
- 析构函数是否正确释放资源？
- 异常安全性如何？

**Step 3: 应用修复**

```bash
# 示例：发现并修复问题
# src/social/telegram_adapter.cpp - 确保析构函数清理
# src/hal/drivers/serial_comm.cpp - 确保串口关闭
```

**Step 4: 提交**

```bash
git add src/
git commit -m "refactor(safety): improve memory safety and resource management

- Use smart pointers where appropriate
- Ensure RAII pattern for resource management
- Improve exception safety

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"
```

---

### Task 9: 错误处理与健壮性

**Files:**
- Review: 所有源文件

**Step 1: 检查裸 catch 块**

搜索并改进：
- `catch (...)` 没有处理逻辑 → 添加日志或特定异常捕获

**Step 2: 检查返回值处理**

- API 调用的返回值是否被检查？
- 失败情况是否被正确处理？

**Step 3: 添加边界检查**

- 数组访问边界检查
- 空指针检查
- JSON 字段存在性检查

**Step 4: 提交**

```bash
git add src/
git commit -m "refactor(robustness): improve error handling and input validation

- Replace bare catch blocks with specific exception handling
- Add return value checks for API calls
- Improve input validation and boundary checks

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"
```

---

### Task 10: 代码结构与可维护性

**Files:**
- Review: 所有源文件

**Step 1: 检查代码重复**

使用 knip 或手动检查：
- 重复的代码块 → 提取为函数
- 重复的常量 → 提取为共享常量

**Step 2: 检查命名一致性**

- 函数命名风格是否一致？
- 变量命名是否清晰？
- 魔数是否应该定义为常量？

**Step 3: 检查未使用的代码**

- 未使用的函数
- 未使用的变量
- 注释掉的代码

**Step 4: 提交**

```bash
git add src/
git commit -m "refactor(cleanup): improve code structure and maintainability

- Extract common code into reusable functions
- Define magic numbers as named constants
- Remove unused code and variables

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"
```

---

### Task 11: 测试覆盖与质量

**Files:**
- Review: `tests/` 目录

**Step 1: 检查测试覆盖**

识别缺少测试的模块：
- 新增的社交功能
- Agent bridge
- Task coordinator

**Step 2: 增加边缘情况测试**

- 空输入处理
- 错误输入处理
- 资源不可用情况

**Step 3: 提交**

```bash
git add tests/
git commit -m "test: improve test coverage and quality

- Add tests for edge cases
- Improve error scenario testing
- Add missing test coverage for new features

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"
```

---

## 第五阶段：最终验证和发布

### Task 12: 全面编译和测试

**Step 1: 清理并重新编译**

```bash
cd build
rm -rf *
cmake ..
make -j4
```

Expected: 编译成功，0 错误

**Step 2: 运行测试**

```bash
ctest --output-on-failure
```

Expected: 所有测试通过

**Step 3: 手动验证**

```bash
./roboclaw --help
./roboclaw --version
```

Expected: 输出显示 "RoboClaw" 而非 "RoboPartner"

---

### Task 13: 更新版本号和最终文档

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `CHANGELOG.md`

**Step 1: 更新版本号**

`CMakeLists.txt`:
```cmake
project(RoboClaw VERSION 0.4.1)
```

**Step 2: 添加最终 CHANGELOG 条目**

已在 Task 6 添加，验证格式正确。

**Step 3: 提交**

```bash
git add CMakeLists.txt CHANGELOG.md
git commit -m "chore: bump version to 0.4.1

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"
```

---

## 执行顺序总结

1. ✅ **Task 1-3**: 配置和常量重命名（CMake、目录、脚本）
2. ✅ **Task 4-5**: 代码文本重命名（CLI、日志、错误消息）
3. ✅ **Task 6-7**: 文档更新（README、CHANGELOG、测试）
4. ✅ **Task 8-11**: 代码质量优化（内存、错误、结构、测试）
5. ✅ **Task 12-13**: 最终验证和发布

**总计**: 13 个主要任务

**依赖**: 无外部依赖，使用现有工具链
