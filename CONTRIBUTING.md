# Contributing to RoboPartner / 为 RoboPartner 做贡献

Thank you for considering contributing to RoboPartner! This document provides guidelines for contributing to the project.

感谢您考虑为 RoboPartner 做贡献！本文档提供了参与本项目的指南。

---

## Table of Contents / 目录

- [Code of Conduct](#code-of-conduct--行为准则)
- [How to Contribute](#how-to-contribute--如何贡献)
- [Development Setup](#development-setup--开发环境设置)
- [Coding Standards](#coding-standards--代码规范)
- [Submitting Changes](#submitting-changes--提交更改)

---

## Code of Conduct / 行为准则

### Our Pledge / 我们的承诺

**[English]**

In the interest of fostering an open and welcoming environment, we as contributors and maintainers pledge to make participation in our project and our community a harassment-free experience for everyone.

**[中文]**

为了培养开放和包容的环境，作为贡献者和维护者，我们承诺让参与本项目和社区的经历对每个人都是无骚扰的。

### Our Standards / 我们的标准

**[English]**
- Respect differing viewpoints and experiences
- Gracefully accept constructive criticism
- Focus on what is best for the community
- Show empathy towards other community members

**[中文]**
- 尊重不同的观点和经验
- 优雅地接受建设性批评
- 关注什么对社区最有利
- 对其他社区成员表示同理心

---

## How to Contribute / 如何贡献

### Reporting Bugs / 报告 Bug

**[English]**

Bug reports should include:
- A clear description of the problem
- Steps to reproduce the issue
- Expected behavior vs actual behavior
- System information (OS, compiler version)
- Relevant log files or screenshots

**[中文]**

Bug 报告应包括：
- 问题的清晰描述
- 重现问题的步骤
- 预期行为 vs 实际行为
- 系统信息（操作系统、编译器版本）
- 相关日志文件或截图

### Suggesting Enhancements / 建议增强

**[English]**

Enhancement suggestions are welcome! Please include:
- A clear description of the proposed feature
- Use cases and benefits
- Potential implementation approaches
- Examples from similar projects

**[中文]**

欢迎提出增强建议！请包括：
- 建议功能的清晰描述
- 用例和好处
- 潜在的实现方法
- 类似项目的示例

---

## Development Setup / 开发环境设置

### Fork and Clone / Fork 和克隆

```bash
# 1. Fork the repository on GitHub
# 2. Clone your fork
git clone https://github.com/YOUR_USERNAME/RoboPartner.git
cd RoboPartner

# 3. Add upstream remote
git remote add upstream https://github.com/ORIGINAL_OWNER/RoboPartner.git
```

### Build Setup / 构建设置

```bash
# Install dependencies (see README.md)
# Build the project
mkdir build && cd build
cmake --preset=debug
cmake --build .
```

---

## Coding Standards / 代码规范

### C++ Guidelines / C++ 指南

**[English]**

1. Follow C++20 Core Guidelines
2. Use RAII for resource management
3. Prefer `std::unique_ptr` and `std::shared_ptr` over raw pointers
4. Use `const` and `constexpr` where possible
5. Avoid magic numbers - use named constants
6. Keep functions focused and small

**[中文]**

1. 遵循 C++20 核心指南
2. 使用 RAII 进行资源管理
3. 优先使用 `std::unique_ptr` 和 `std::shared_ptr` 而非裸指针
4. 尽可能使用 `const` 和 `constexpr`
5. 避免魔术数字 - 使用命名常量
6. 保持函数专注和小巧

### Code Formatting / 代码格式

**[English]**
- Use 4 spaces for indentation (no tabs)
- Maximum line length: 120 characters
- Opening braces on the same line for functions/methods
- Namespace comments: `// namespace robopartner {`

**[中文]**
- 使用 4 个空格缩进（不使用制表符）
- 最大行长度：120 个字符
- 函数/方法的左大括号在同一行
- 命名空间注释：`// namespace robopartner {`

### Example / 示例

```cpp
// Good / 好的示例
namespace robopartner {

class Example {
public:
    explicit Example(std::string name)
        : name_(std::move(name)) {}

    void process() {
        std::lock_guard<std::mutex> lock(mutex_);
        // Implementation
    }

private:
    std::string name_;
    std::mutex mutex_;
};

} // namespace robopartner
```

### Thread Safety / 线程安全

**[English]**

When writing code that will be accessed by multiple threads:
- Use `std::mutex` for exclusive access
- Use `std::shared_mutex` for read-heavy data structures
- Use `std::atomic` for simple counters/flags
- Follow RAII for lock management

**[中文]**

编写多线程访问的代码时：
- 使用 `std::mutex` 进行独占访问
- 使用 `std::shared_mutex` 处理读多写少的数据结构
- 使用 `std::atomic` 处理简单计数器/标志
- 遵循 RAII 进行锁管理

---

## Submitting Changes / 提交更改

### Commit Messages / 提交消息

**[English]**

Follow conventional commits format:
```
<type>(<scope>): <subject>

<body>

<footer>
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `chore`

**[中文]**

遵循约定式提交格式：
```
<type>(<scope>): <subject>

<body>

<footer>
```

类型：`feat`（新功能）、`fix`（修复）、`docs`（文档）、`style`（格式）、`refactor`（重构）、`perf`（性能）、`test`（测试）、`chore`（杂项）

### Pull Request Checklist / PR 检查清单

**[English]**

Before submitting a PR:
- [ ] Code builds without errors
- [ ] Tests pass (when available)
- [ ] Documentation is updated
- [ ] Commit messages follow conventions
- [ ] No merge conflicts with upstream/main

**[中文]**

提交 PR 前：
- [ ] 代码构建无错误
- [ ] 测试通过（如有）
- [ ] 文档已更新
- [ ] 提交消息遵循约定
- [ ] 与上游/主线无合并冲突

---

## Review Process / 审查流程

**[English]**

1. All submissions require review
2. Maintainers will respond within 7 days
3. Address review feedback promptly
4. Large changes may be split into smaller PRs

**[中文]**

1. 所有提交都需要审查
2. 维护者会在 7 天内回复
3. 及时处理审查反馈
4. 大型更改可能需要拆分为更小的 PR

---

## Getting Help / 获取帮助

**[English]**

- Create an issue for bugs or questions
- Check existing documentation
- Join community discussions

**[中文]**

- 为问题或疑问创建 issue
- 查看现有文档
- 参与社区讨论

---

## License / 许可证

By contributing to RoboPartner, you agree that your contributions will be licensed under the MIT License.

通过为 RoboPartner 做贡献，您同意您的贡献将在 MIT 许可证下许可。
