# Changelog / 更新日志

All notable changes to RoboClaw will be documented in this file.
RoboClaw 的所有重要更改都将记录在此文件中。

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
格式基于 [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)，
本项目遵循 [语义化版本](https://semver.org/spec/v2.0.0.html)。

---

## [Unreleased]

### Added / 新增
- Thread pool implementation for concurrent task execution / 并发任务执行的线程池实现
- Token optimization with LRU cache / 带 LRU 缓存的 Token 优化
- Skill system with parser, registry, and executor / 带解析器、注册表和执行器的技能系统
- Session management with tree-structured conversations / 树状对话结构的会话管理
- Multi-platform support (macOS, Linux, Windows) / 多平台支持
- VSCode integration with CMake presets / 带 CMake 预设的 VSCode 集成

### Changed / 更改
- Refactored tool result structure (error → error_message) / 重构工具结果结构
- Updated to C++20 standard / 更新到 C++20 标准
- Improved thread safety with read-write locks / 用读写锁改进线程安全

### Fixed / 修复
- Fixed compilation errors across all modules / 修复所有模块的编译错误
- Fixed header include paths / 修复头文件包含路径
- Fixed CMake configuration for Homebrew dependencies / 修复 Homebrew 依赖的 CMake 配置

---

## [0.1.0] - 2025-02-21

### Added / 新增
- Initial project structure / 初始项目结构
- CMake build configuration / CMake 构建配置
- Four core tools: Read, Write, Edit, Bash / 四个核心工具
- LLM provider interface (Anthropic, OpenAI) / LLM 提供商接口
- Agent engine with conversation history / 带对话历史的 Agent 引擎
- Interactive CLI mode / 交互式 CLI 模式
- Configuration wizard / 配置向导
- Token estimation and optimization / Token 估算和优化

### Features / 特性
- Cross-platform tool execution / 跨平台工具执行
- Stream processing support / 流式处理支持
- Session persistence / 会话持久化
- Modular architecture / 模块化架构
