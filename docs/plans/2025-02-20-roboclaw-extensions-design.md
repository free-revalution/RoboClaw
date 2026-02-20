# RoboClaw 扩展功能设计文档

**版本**: 1.0.0
**日期**: 2025-02-20
**状态**: 设计阶段

---

## 目录

1. [技能系统架构设计](#1-技能系统架构设计)
2. [Token优化策略设计](#2-token优化策略设计)
3. [技能执行流程设计](#3-技能执行流程设计)
4. [Token优化组件设计](#4-token优化组件设计)
5. [CLI集成与用户界面](#5-cli集成与用户界面)
6. [项目目录结构与实现计划](#6-项目目录结构与实现计划)
7. [配置文件更新](#7-配置文件更新)
8. [内置技能示例](#8-内置技能示例)
9. [总结与下一步](#9-总结与下一步)

---

## 1. 技能系统架构设计

### 1.1 推荐方案：简化兼容 + Agent层集成

考虑到实现复杂度和实用性，采用**简化兼容模式**，支持Claude Code技能的核心功能。

### 1.2 技能文件格式（简化版）

```yaml
name: 技能名称
description: 技能描述
triggers:
  - "触发关键词1"
  - "触发关键词2"
actions:
  - type: tool | llm | script
    description: 操作描述
    parameters: ...
```

### 1.3 核心组件

| 组件 | 功能 |
|------|------|
| **SkillParser** | 解析.skill文件（支持YAML和JSON） |
| **SkillRegistry** | 技能注册表，管理所有加载的技能 |
| **SkillExecutor** | 技能执行器，在Agent层运行技能 |
| **SkillDownloader** | 从GitHub等远程仓库下载技能 |
| **SkillCache** | 技能缓存，避免重复下载 |

### 1.4 存储位置

- **本地**：`~/.roboclaw/skills/`
- **远程**：支持从GitHub仓库下载（如`github.com/user/skills`）

### 1.5 与工具系统的关系

- 技能可以调用现有的4个基础工具
- 技能可以组合多个工具形成复合操作
- 技能可以触发Agent的自动编写代码能力

---

## 2. Token优化策略设计

### 2.1 全面优化方案

针对token消耗的多方面问题，采用以下优化策略：

#### 2.1.1 对话历史压缩（智能摘要）
- 当对话历史超过阈值（如8000 tokens）时，自动触发压缩
- 使用LLM对旧对话进行摘要，保留关键信息
- 分层存储：近期对话（完整）+ 中期对话（摘要）+ 远期对话（丢弃）

#### 2.1.2 系统提示词缓存（Prompt Caching）
- 利用Anthropic/OpenAI的缓存机制，系统提示词只计费一次
- 将系统提示词和工具定义设为缓存项
- 缓存命中可节省90%以上的系统提示token

#### 2.1.3 工具调用结果压缩
- 工具执行结果进行智能裁剪（如代码只返回修改部分）
- 大文件只返回摘要而非完整内容
- 错误信息精简，去除冗余堆栈信息

#### 2.1.4 上下文窗口管理
- 动态调整发送给LLM的上下文大小
- 优先保留：最近消息 > 用户当前输入 > 相关历史 > 远期历史
- 支持滑动窗口模式，保持固定上下文大小

#### 2.1.5 Token使用统计与监控
- 实时显示当前对话的token消耗
- 预估下次请求的token数
- 提供优化建议（如"建议开启摘要"）

---

## 3. 技能执行流程设计

### 3.1 技能生命周期管理

```
启动 → 扫描本地skills/目录 → 解析.skill文件 → 注册到SkillRegistry
              ↓
        检查远程配置 → 下载新技能 → 缓存到本地
```

### 3.2 技能触发机制

- **关键词匹配**：用户输入包含触发词时激活
- **意图检测**：LLM判断当前任务是否需要技能
- **手动调用**：用户通过命令显式调用（如`/skill code-review`）

### 3.3 技能执行流程

```
用户输入 → 意图分析 → 匹配技能 → SkillExecutor
                                    ↓
                            解析技能定义
                                    ↓
                            执行操作序列：
                            - 调用工具（read/write/edit/bash）
                            - 调用LLM（代码生成/分析）
                            - 调用其他技能（组合）
                                    ↓
                            返回执行结果
```

### 3.4 技能类型

| 类型 | 描述 |
|------|------|
| **Tool技能** | 组合多个工具操作（如"创建项目骨架"） |
| **LLM技能** | 需要AI分析的能力（如"代码审查"、"优化建议"） |
| **Mixed技能** | 混合使用工具和LLM（如"修bug"：读代码→LLM分析→编辑文件） |

### 3.5 技能参数传递

- 支持用户输入参数（如文件路径、选项）
- 支持上下文注入（当前对话、选中代码）
- 支持默认值和参数验证

---

## 4. Token优化组件设计

### 4.1 TokenOptimizer - Token优化器

```cpp
class TokenOptimizer {
    // 估算消息token数
    int estimateTokens(const std::vector<ChatMessage>& messages);

    // 压缩对话历史
    std::vector<ChatMessage> compressHistory(
        const std::vector<ChatMessage>& history);

    // 启用系统提示缓存
    void enablePromptCaching(bool enable);

    // 压缩工具结果
    std::string compressToolResult(const ToolResult& result);
};
```

### 4.2 ConversationCompressor - 对话压缩器

```cpp
class ConversationCompressor {
    // 分层压缩策略
    struct CompressionLayers {
        std::vector<ChatMessage> recent;      // 最近5条（完整）
        std::vector<ChatMessage> middle;      // 中期10条（摘要）
        std::vector<ChatMessage> old_summary;  // 远期（丢弃或总摘要）
    };

    // 执行压缩
    CompressionLayers compress(const std::vector<ChatMessage>& history);

    // 生成摘要
    std::string generateSummary(const std::vector<ChatMessage>& messages);
};
```

### 4.3 TokenBudget - Token预算管理

```cpp
class TokenBudget {
    // 设置预算限制
    void setBudget(int maxTokens);

    // 检查预算
    bool checkBudget(const std::vector<ChatMessage>& messages);

    // 获取优化建议
    std::string getOptimizationSuggestion();

    // 实时统计
    TokenStats getStats() const;
};
```

### 4.4 PromptCacheManager - 提示词缓存管理

```cpp
class PromptCacheManager {
    // 为Anthropic/OpenAI生成缓存头
    std::map<std::string, std::string> generateCacheHeaders();

    // 缓存键计算（基于内容）
    std::string computeCacheKey(const std::string& systemPrompt,
                                const std::string& toolsSchema);
};
```

---

## 5. CLI集成与用户界面

### 5.1 技能管理命令

```bash
# 列出所有技能
roboclaw skill --list

# 显示技能详情
roboclaw skill --show <skill-name>

# 安装技能（从本地或远程）
roboclaw skill --install <skill-name> [--url <github-url>]

# 卸载技能
roboclaw skill --uninstall <skill-name>

# 更新技能
roboclaw skill --update <skill-name>

# 创建新技能
roboclaw skill --create <skill-name>
```

### 5.2 Token优化命令

```bash
# 显示token使用统计
roboclaw stats --tokens

# 设置token预算
roboclaw config --set max_tokens <number>

# 查看优化建议
roboclaw stats --optimization

# 启用/禁用压缩
roboclaw config --set compression <on|off>
```

### 5.3 交互模式中的技能调用

```
你: /skill list
你: /skill code-review src/main.cpp
你: 使用code-review技能检查main.cpp
```

### 5.4 实时Token显示

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  RoboClaw | Claude Sonnet 4 | Tokens: 2341/8000
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                        ↑
                    实时显示当前/最大token
```

---

## 6. 项目目录结构与实现计划

### 6.1 新增目录结构

```
RoboClaw/
├── src/
│   ├── skills/                    # 技能模块
│   │   ├── skill_parser.h/cpp     # 技能文件解析器
│   │   ├── skill_registry.h/cpp   # 技能注册表
│   │   ├── skill_executor.h/cpp   # 技能执行器
│   │   ├── skill_downloader.h/cpp # 远程下载
│   │   └── builtin_skills/        # 内置技能
│   │       ├── code_review.skill
│   │       └── project_scaffold.skill
│   │
│   ├── optimization/             # Token优化模块
│   │   ├── token_optimizer.h/cpp  # Token优化器
│   │   ├── conversation_compressor.h/cpp  # 对话压缩
│   │   ├── token_budget.h/cpp    # 预算管理
│   │   └── prompt_cache.h/cpp     # 提示词缓存
│   │
│   └── cli/
│       └── skill_commands.h/cpp   # 技能CLI命令
│
├── skills/                        # 本地技能存储
│   ├── user/                      # 用户技能
│   └── builtin/                   # 内置技能
│
└── .roboclaw/
    └── skills/                    # 运行时技能缓存
```

### 6.2 分阶段实现计划

#### 第一阶段：技能基础框架（2周）
- 实现SkillParser（YAML/JSON解析）
- 实现SkillRegistry
- 创建2-3个内置技能作为示例
- CLI命令：`skill --list`, `skill --show`

#### 第二阶段：技能执行引擎（2周）
- 实现SkillExecutor（在Agent层集成）
- 支持Tool类型技能（组合工具操作）
- 支持参数传递和上下文注入
- 交互模式中调用技能

#### 第三阶段：远程技能管理（1周）
- 实现SkillDownloader（GitHub支持）
- 技能缓存机制
- CLI命令：`skill --install`, `skill --update`

#### 第四阶段：Token优化组件（2周）
- 实现TokenOptimizer（估算+压缩）
- 实现ConversationCompressor
- 实现PromptCacheManager（利用缓存）

#### 第五阶段：预算管理与监控（1周）
- 实现TokenBudget
- 实时token显示
- 优化建议系统

---

## 7. 配置文件更新

### 7.1 新增配置项

```toml
# ============================================
# 技能系统配置
# ============================================

[skills]
# 本地技能目录
local_skills_dir = "~/.roboclaw/skills"

# 远程技能仓库
[[skills.repositories]]
name = "官方技能库"
url = "https://github.com/roboclaw/skills"
enabled = true

[[skills.repositories]]
name = "社区技能库"
url = "https://github.com/community/roboclaw-skills"
enabled = true

# 技能自动更新
auto_update = true
update_interval_hours = 24

# ============================================
# Token优化配置
# ============================================

[optimization]
# 启用对话压缩
enable_compression = true

# 压缩阈值（tokens）
compression_threshold = 8000

# 目标token预算
target_budget = 12000

# 启用提示词缓存
enable_prompt_caching = true

# 工具结果压缩
compress_tool_results = true
max_tool_result_length = 5000

# Token统计显示
show_token_stats = true
stats_update_interval = 1

# ============================================
# 缓存配置
# ============================================

[cache]
# 技能缓存目录
skills_cache_dir = ".roboclaw/skills/cache"

# 缓存过期时间（小时）
skill_cache_ttl = 168

# 提示词缓存大小限制（MB）
prompt_cache_size = 100
```

### 7.2 新增CLI命令映射

```toml
# 技能命令别名
[skill_aliases]
"cr" = "code-review"
"review" = "code-review"
"test" = "generate-tests"
"fix" = "fix-bug"
"scaffold" = "project-scaffold"

# Token优化快捷命令
[optimization_aliases]
"stats" = "stats --tokens"
"budget" = "config --show optimization"
"compress" = "config --set compression on"
```

---

## 8. 内置技能示例

### 8.1 Code Review 技能

```yaml
# 内置技能：代码审查
name: code-review
description: 审查代码质量、安全性、性能
triggers:
  - "审查代码"
  - "code review"
  - "检查代码"
  - "review"
actions:
  - type: tool
    name: read
    description: 读取目标文件
    parameters:
      file: "${file_path}"

  - type: llm
    description: 分析代码质量
    prompt: |
      请审查以下代码，检查：
      1. 潜在的bug和逻辑错误
      2. 安全漏洞
      3. 性能优化机会
      4. 代码风格和可读性

      代码内容：
      ${file_content}
```

### 8.2 Fix Bug 技能

```yaml
# 内置技能：修复Bug
name: fix-bug
description: 分析并修复bug
triggers:
  - "修bug"
  - "fix bug"
  - "修复错误"
actions:
  - type: script
    description: 创建bug修复分支
    commands:
      - "branch --new fix-${bug_name}"
      - "edit ${file_path} ${old_code} ${new_code}"
      - "bash 'make test'"
```

### 8.3 Generate Tests 技能

```yaml
# 内置技能：生成测试
name: generate-tests
description: 为代码生成单元测试
triggers:
  - "生成测试"
  - "generate tests"
  - "写测试"
actions:
  - type: tool
    name: read
    parameters:
      file: "${target_file}"

  - type: llm
    description: 生成测试代码
    prompt: |
      为以下代码生成单元测试（使用Catch2框架）：
      ${code}

      要求：
      - 覆盖正常路径和边界条件
      - 包含必要的测试用例
```

### 8.4 Project Scaffold 技能

```yaml
# 内置技能：项目骨架
name: project-scaffold
description: 创建项目目录结构
triggers:
  - "创建项目"
  - "项目骨架"
  - "scaffold"
actions:
  - type: script
    description: 创建目录结构
    commands:
      - "mkdir -p src/{cli,agent,tools,llm}"
      - "mkdir -p tests/{unit,integration}"
      - "write CMakeLists.txt ${cmake_template}"
      - "write README.md ${readme_template}"
```

---

## 9. 总结与下一步

### 9.1 设计方案总结

**技能系统 (Skill System)**
- 简化兼容Claude Code技能格式（YAML/JSON）
- 本地+远程双存储支持
- Agent层集成，灵活调用工具和LLM
- 支持Tool/LLM/Mixed三种技能类型
- 内置4个核心技能作为示例

**Token优化 (Token Optimization)**
- 对话历史智能压缩（分层摘要）
- 提示词缓存（Prompt Caching）
- 工具结果智能裁剪
- 实时token统计与预算管理
- 优化建议系统

### 9.2 新增模块清单

| 模块 | 功能 | 文件数 |
|------|------|--------|
| SkillParser | 技能文件解析 | 2 |
| SkillRegistry | 技能注册表 | 2 |
| SkillExecutor | 技能执行器 | 2 |
| SkillDownloader | 远程下载 | 2 |
| TokenOptimizer | Token优化 | 2 |
| ConversationCompressor | 对话压缩 | 2 |
| TokenBudget | 预算管理 | 2 |
| PromptCacheManager | 缓存管理 | 2 |
| 技能CLI命令 | 用户接口 | 2 |
| 内置技能 | 示例技能 | 4 |

**总计：约22个新文件**

### 9.3 优先级建议

考虑到用户希望减少token成本，建议**优先实现Token优化**（第四阶段），因为它可以立即带来成本节省。

---

**文档结束**
