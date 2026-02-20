# RoboClaw 设计文档

**版本**: 1.0.0
**日期**: 2025-02-20
**作者**: AI Agent
**状态**: 设计阶段

---

## 目录

1. [项目概览](#1-项目概览)
2. [整体架构设计](#2-整体架构设计)
3. [工具系统详细设计](#3-工具系统详细设计)
4. [对话树结构设计](#4-对话树结构设计)
5. [LLM接口层设计](#5-llm接口层设计)
6. [项目目录结构与构建](#6-项目目录结构与构建)
7. [配置文件设计](#7-配置文件设计)
8. [系统提示词设计](#8-系统提示词设计)
9. [CLI命令设计](#9-cli命令设计)
10. [错误处理与日志系统](#10-错误处理与日志系统)
11. [测试策略](#11-测试策略)
12. [开发路线图](#12-开发路线图)
13. [开发环境准备](#13-开发环境准备)

---

## 1. 项目概览

### 项目名称
**RoboClaw** - 用C++复现OpenClaw项目Pi引擎的极简AI Agent框架

### 核心理念
与主流Agent框架（如LangChain、AutoGPT）不同，Pi采用"少即是多"的设计哲学：
- **只有4个工具**：Read（读文件）、Write（写文件）、Edit（编辑文件）、Bash（执行命令）
- **极简系统提示词**：最短的Agent系统提示词
- **自编码能力**：需要新功能时让Agent自己写代码，而不是安装插件
- **树状对话结构**：支持分支，修bug不影响主线

### 目标平台
- **开发环境**：macOS
- **跨平台支持**：同时支持 macOS、Linux、Windows
- **构建工具**：CMake
- **依赖管理**：FetchContent（CMake内置）

### 用户目标
本项目面向编程小白用户，设计文档需要详细易懂，让初学者能够理解整个系统的运作原理。

---

## 2. 整体架构设计

### 系统分层架构

```
┌─────────────────────────────────────────────────────────┐
│                    用户命令行界面 (CLI)                    │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                    会话管理器 (SessionManager)            │
│  - 创建/加载/切换对话树                                     │
│  - 管理对话分支                                            │
│  - 处理用户输入                                            │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                    Agent引擎 (Agent)                      │
│  - 构建系统提示词                                          │
│  - 管理工具调用                                            │
│  - 对话历史管理                                            │
└─────────────────────────────────────────────────────────┘
                            ↓
┌───────────────────┬───────────────────┬─────────────────┐
│     工具系统       │    LLM接口层      │    存储层       │
│  - Read           │  - Anthropic      │  - 对话树持久化  │
│  - Write          │  - OpenAI         │  - 配置文件      │
│  - Edit           │  - Gemini         │  - 状态管理      │
│  - Bash           │  - 国内模型       │                  │
└───────────────────┴───────────────────┴─────────────────┘
```

### 核心模块说明

**1. CLI层 (`src/cli/`)**
- 负责解析命令行参数
- 首次运行时引导用户配置（API密钥、默认模型等）
- 显示对话内容、工具调用、调试信息

**2. 会话管理器 (`src/session/`)**
- 管理对话树的创建、加载、保存
- 支持在分支间切换
- 追踪当前对话节点位置

**3. Agent引擎 (`src/agent/`)**
- 维护完整的对话历史（用户消息 + 助手回复 + 工具调用）
- 构建发送给LLM的提示词
- 解析LLM返回的工具调用请求
- 执行工具并将结果反馈给LLM

**4. 工具系统 (`src/tools/`)**
- 四个基础工具的实现
- 工具调用的参数验证
- 工具执行结果的格式化

**5. LLM接口层 (`src/llm/`)**
- 统一的API抽象，支持多家提供商
- 流式响应处理（打字机效果）
- 错误处理和重试机制

---

## 3. 工具系统详细设计

### 工具调用原理

LLM不直接执行操作，而是返回"工具调用"的JSON格式请求，程序解析后执行对应工具，将结果返回给LLM。

**示例流程：**
```
用户: "帮我看看README.md的内容"
  ↓
LLM返回: {"tool": "read", "path": "README.md"}
  ↓
程序执行: Read工具读取文件内容
  ↓
程序返回LLM: {"result": "文件内容是..."}
  ↓
LLM生成最终回复给用户
```

### 四个工具的详细设计

#### 3.1 Read 工具
```cpp
// 功能：读取文件内容
// 参数：
//   - path: 文件路径（必需）
//   - offset: 起始行号（可选，默认0）
//   - limit: 读取行数（可选，默认全部）
// 返回：文件内容 + 元信息（总行数、编码等）
```

**实现要点：**
- 支持相对路径和绝对路径
- 自动检测文件编码（UTF-8是重点）
- 大文件分页读取（避免内存溢出）
- 错误处理：文件不存在、无权限等

#### 3.2 Write 工具
```cpp
// 功能：创建新文件或覆盖现有文件
// 参数：
//   - path: 文件路径（必需）
//   - content: 文件内容（必需）
// 返回：写入状态 + 文件大小
```

**安全机制：**
- 写入前检查目录是否存在，不存在则创建
- 不允许覆盖正在被编辑的文件
- 支持原子写入（先写临时文件，成功后重命名）

#### 3.3 Edit 工具
```cpp
// 功能：精确替换文件中的内容
// 参数：
//   - path: 文件路径（必需）
//   - old_string: 要替换的内容（必需）
//   - new_string: 替换后的内容（必需）
// 返回：替换次数 + 影响行号
```

**实现要点：**
- 精确匹配（考虑缩进、空格）
- 支持多处替换（old_string出现多次）
- 必须先Read文件才能Edit（防止盲目修改）
- old_string必须唯一或由用户确认

#### 3.4 Bash 工具
```cpp
// 功能：执行shell命令
// 参数：
//   - command: 命令字符串（必需）
//   - timeout: 超时时间（可选，默认120秒）
// 返回：标准输出 + 标准错误 + 退出码
```

**安全机制：**
- 命令超时限制（防止死循环）
- 禁止某些危险命令（rm -rf /等）
- 工作目录保持在项目根目录
- 跨平台兼容（Windows用cmd，Unix用bash）

---

## 4. 对话树结构设计

### 树状数据模型

```
                    [根节点]
                  /    |    \
              分支A   分支B   分支C
              /  \      |
           节点1 节点2  节点3
```

每个节点代表一次"用户输入 + AI回复"的完整交互。

### 节点结构设计

```cpp
struct ConversationNode {
    string id;                    // 唯一标识符
    string parent_id;             // 父节点ID（根节点为空）
    vector<string> children;      // 子节点ID列表

    // 用户消息
    string user_message;
    time_t timestamp;

    // AI回复（包含工具调用历史）
    struct AssistantMessage {
        string content;
        vector<ToolCall> tool_calls;  // 调用的工具及结果
    } assistant_message;

    // 元数据
    string branch_name;           // 分支名称（如"fix-bug"、"experiment"）
    bool is_active;               // 是否为当前活动节点
};
```

### 分支操作

**创建分支（用于修bug/实验）：**
```
主线: A → B → C → D
            ↓
创建分支:  → E → F (修复bug)
```

- 从任意节点创建新分支
- 分支拥有独立历史
- 修复完成后可以"合并回主线"（接受分支的改动）

**切换分支：**
```bash
roboclaw branch --list        # 查看所有分支
roboclaw branch --switch <id> # 切换到指定分支
```

### 存储结构

在当前工作目录的`.roboclaw/`下：
```
.roboclaw/
├── config.toml              # 全局配置
├── conversations/
│   ├── <conversation-id>/
│   │   ├── tree.json        # 对话树结构
│   │   ├── nodes/
│   │   │   ├── <node-id>.json  # 每个节点的详细数据
│   │   │   └── ...
│   │   └── current.txt      # 当前活动节点ID
```

### 树的合并策略

当分支修复bug后合并回主线：
1. 展示分支与主线的差异
2. 用户确认后，将主线节点指向分支终点
3. 保留分支历史供参考

---

## 5. LLM接口层设计

### 统一API抽象

为了支持多家LLM提供商，设计一个统一的接口：

```cpp
// LLM提供商抽象基类
class LLMProvider {
public:
    virtual ~LLMProvider() = default;

    // 发送消息，获取响应（支持流式）
    virtual string chat(
        const vector<ChatMessage>& messages,
        const vector<Tool>& tools
    ) = 0;

    // 流式响应（回调函数逐字返回）
    virtual void chatStream(
        const vector<ChatMessage>& messages,
        const vector<Tool>& tools,
        function<void(const string&)> on_chunk
    ) = 0;

    // 获取模型信息
    virtual string getModelName() const = 0;
    virtual int getMaxTokens() const = 0;
};

// 支持的提供商
enum class ProviderType {
    ANTHROPIC,  // Claude系列
    OPENAI,     // GPT系列
    GEMINI,     // Google Gemini
    DEEPSEEK,   // 深度求索
    DOUBAO,     // 字节豆包
    QWEN        // 阿里通义千问
};
```

### 消息格式统一

```cpp
struct ChatMessage {
    enum Role { USER, ASSISTANT, SYSTEM, TOOL };
    Role role;
    string content;

    // 工具调用相关（仅ASSISTANT角色使用）
    struct ToolCall {
        string id;
        string name;
        string arguments;  // JSON字符串
    };
    vector<ToolCall> tool_calls;

    // 工具返回结果（仅TOOL角色使用）
    string tool_call_id;
    string tool_result;
};
```

### 工具定义格式（发送给LLM）

```json
{
  "tools": [
    {
      "name": "read",
      "description": "读取文件内容",
      "parameters": {
        "type": "object",
        "properties": {
          "path": {"type": "string"},
          "offset": {"type": "integer"},
          "limit": {"type": "integer"}
        },
        "required": ["path"]
      }
    }
  ]
}
```

### 流式响应处理

```
LLM返回: "我" → "我来" → "我来帮" → "我来帮你" → ...
  ↓
逐字显示在终端（打字机效果）
  ↓
同时解析工具调用（如<tool=read>）
```

### 错误处理与重试

- 网络错误：自动重试（最多3次，指数退避）
- API限流：等待后重试
- 密钥无效：提示用户重新配置
- 超时：可配置超时时间（默认60秒）

---

## 6. 项目目录结构与构建

### 完整目录结构

```
RoboClaw/
├── CMakeLists.txt              # CMake主配置文件
├── README.md                   # 项目说明
├── docs/
│   └── plans/
│       └── 2025-02-20-roboclaw-design.md  # 本设计文档
│
├── src/                        # 源代码目录
│   ├── main.cpp                # 程序入口
│   │
│   ├── cli/                    # 命令行界面
│   │   ├── cli.h
│   │   ├── cli.cpp
│   │   └── config_wizard.h/cpp  # 首次配置引导
│   │
│   ├── session/                # 会话管理
│   │   ├── session_manager.h/cpp
│   │   ├── conversation_tree.h/cpp
│   │   └── conversation_node.h/cpp
│   │
│   ├── agent/                  # Agent引擎
│   │   ├── agent.h/cpp
│   │   ├── prompt_builder.h/cpp
│   │   └── tool_executor.h/cpp
│   │
│   ├── tools/                  # 工具实现
│   │   ├── tool_base.h/cpp     # 工具基类
│   │   ├── read_tool.h/cpp
│   │   ├── write_tool.h/cpp
│   │   ├── edit_tool.h/cpp
│   │   └── bash_tool.h/cpp
│   │
│   ├── llm/                    # LLM接口
│   │   ├── llm_provider.h      # 抽象接口
│   │   ├── anthropic_provider.h/cpp
│   │   ├── openai_provider.h/cpp
│   │   ├── gemini_provider.h/cpp
│   │   ├── deepseek_provider.h/cpp
│   │   └── http_client.h/cpp   # HTTP请求封装
│   │
│   ├── storage/                # 存储层
│   │   ├── storage.h/cpp
│   │   ├── config_manager.h/cpp
│   │   └── tree_serializer.h/cpp
│   │
│   └── utils/                  # 工具类
│       ├── logger.h/cpp
│       ├── string_utils.h/cpp
│       └── file_utils.h/cpp
│
├── tests/                      # 测试代码
│   ├── unit/
│   └── integration/
│
└── external/                   # 第三方库（自动下载）
    ├── cpr/                   # HTTP库
    ├── nlohmann/              # JSON库
    └── cpp-toml/              # TOML解析库
```

### CMakeLists.txt 详解

```cmake
cmake_minimum_required(VERSION 3.20)
project(RoboClaw VERSION 1.0.0 LANGUAGES CXX)

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
    GIT_TAG 1.10.0
)
FetchContent_MakeAvailable(cpr)

# 2. nlohmann/json - JSON库
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.0
)
FetchContent_MakeAvailable(nlohmann_json)

# 3. cpp-toml - TOML库
FetchContent_Declare(cpptoml
    GIT_REPOSITORY https://github.com/skystrife/cpptoml.git
    GIT_TAG master
)
FetchContent_MakeAvailable(cpptoml)

# 源文件
set(SOURCES
    src/main.cpp
    src/cli/*.cpp
    src/session/*.cpp
    src/agent/*.cpp
    src/tools/*.cpp
    src/llm/*.cpp
    src/storage/*.cpp
    src/utils/*.cpp
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

# 安装规则
install(TARGETS roboclaw DESTINATION bin)
```

### 编译步骤（用户视角）

```bash
# 1. 创建构建目录
mkdir build && cd build

# 2. 配置项目（CMake自动下载依赖）
cmake ..

# 3. 编译
cmake --build .

# 4. 安装（可选）
sudo cmake --install .
```

---

## 7. 配置文件设计

### 配置文件格式 (TOML)

配置文件位置：`~/.roboclaw/config.toml`

```toml
# ============================================
# RoboClaw 配置文件
# ============================================

# 默认LLM提供商
[default]
provider = "anthropic"  # anthropic, openai, gemini, deepseek, doubao, qwen
model = "claude-sonnet-4-20250514"

# ============================================
# LLM提供商配置
# ============================================

# Anthropic (Claude)
[providers.anthropic]
api_key = "sk-ant-xxx..."
base_url = "https://api.anthropic.com"
models = ["claude-sonnet-4-20250514", "claude-opus-4-20250514"]

# OpenAI (GPT)
[providers.openai]
api_key = "sk-xxx..."
base_url = "https://api.openai.com/v1"
models = ["gpt-4o", "gpt-4o-mini"]

# Google Gemini
[providers.gemini]
api_key = "AIxxx..."
base_url = "https://generativelanguage.googleapis.com/v1"
models = ["gemini-2.0-flash"]

# 深度求索
[providers.deepseek]
api_key = "sk-xxx..."
base_url = "https://api.deepseek.com"
models = ["deepseek-chat", "deepseek-coder"]

# 字节豆包
[providers.doubao]
api_key = "xxx..."
base_url = "https://ark.cn-beijing.volces.com/api/v3"
models = ["doubao-pro-32k"]

# 阿里通义千问
[providers.qwen]
api_key = "sk-xxx..."
base_url = "https://dashscope.aliyuncs.com/compatible-mode/v1"
models = ["qwen-max", "qwen-plus"]

# ============================================
# 行为设置
# ============================================

[behavior]
# 最大重试次数
max_retries = 3

# 请求超时（秒）
timeout = 60

# 是否显示详细日志
verbose = true

# 流式响应延迟（毫秒，打字机效果）
stream_delay = 10

# ============================================
# 工具设置
# ============================================

[tools]
# Bash命令超时（秒）
bash_timeout = 30

# 禁止的危险命令
forbidden_commands = ["rm -rf /", "rm -rf /*", "mkfs", "dd if=/dev/zero"]

# 最大读取文件大小（MB）
max_read_size = 10
```

### 配置管理器功能

```cpp
class ConfigManager {
public:
    // 加载配置文件
    bool load(const string& path);

    // 保存配置文件
    bool save(const string& path);

    // 获取值
    string getProvider() const;
    string getModel() const;
    string getApiKey(const string& provider) const;

    // 设置值
    void setProvider(const string& provider);
    void setModel(const string& model);
    void setApiKey(const string& provider, const string& key);

    // 验证配置
    bool validate() const;
};
```

### 首次运行引导流程

```
第一次运行 roboclaw
  ↓
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  欢迎使用 RoboClaw！
  首次运行需要进行配置...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  ↓
请选择默认的LLM提供商:
  1. Anthropic (Claude)      [推荐]
  2. OpenAI (GPT)
  3. Google Gemini
  4. 深度求索
  5. 字节豆包
  6. 阿里通义千问
  ↓
请输入 API Key: sk-ant-xxx...
  ↓
请选择默认模型:
  1. claude-sonnet-4-20250514  [推荐]
  2. claude-opus-4-20250514
  ↓
配置已保存到 ~/.roboclaw/config.toml
  ↓
现在可以开始使用了！
```

---

## 8. 系统提示词设计

### Pi的极简提示词理念

Pi的核心优势是极短的系统提示词，让LLM更专注于用户任务而非复杂的指令。

### RoboClaw系统提示词（中文版）

```
你是RoboClaw，一个AI编程助手。

你可以使用以下工具：
- read(path, offset?, limit?): 读取文件内容
- write(path, content): 创建或覆盖文件
- edit(path, old_string, new_string): 编辑文件（精确替换）
- bash(command, timeout?): 执行shell命令

工具调用格式：{"tool": "read", "path": "文件路径"}
执行工具后，将结果反馈给用户，然后继续你的工作。

重要规则：
1. 修改文件前先用read确认内容
2. edit的old_string必须精确匹配（包括缩进）
3. bash命令超时默认120秒
4. 保持简洁，直接执行任务
```

### 提示词构建器

```cpp
class PromptBuilder {
public:
    // 构建完整提示词
    string buildPrompt(const vector<ChatMessage>& history) {
        string prompt = system_prompt_ + "\n\n";
        prompt += "可用工具：\n";
        prompt += buildToolsSchema();
        prompt += "\n对话历史：\n";
        prompt += buildConversationHistory(history);
        return prompt;
    }

private:
    string buildToolsSchema() {
        // 生成工具的JSON Schema格式
        // 用于让LLM理解如何调用工具
    }

    string buildConversationHistory(const vector<ChatMessage>& history) {
        // 将历史消息转换为LLM可读格式
    }

    string system_prompt_;  // 存储系统提示词
};
```

### 提示词模板化

```cpp
// 不同场景的提示词变体
enum class PromptMode {
    MINIMAL,      // 极简模式（如上）
    VERBOSE,      // 详细模式（更多规则）
    CODING,       // 编程专用
    DEBUGGING     // 调试专用
};
```

### 工具Schema生成

```cpp
string buildToolsSchema() {
    return R"(
可用工具：
1. read
   - 描述: 读取文件内容
   - 参数: path (string, 必需), offset (int, 可选), limit (int, 可选)

2. write
   - 描述: 创建新文件或覆盖现有文件
   - 参数: path (string, 必需), content (string, 必需)

3. edit
   - 描述: 精确替换文件内容
   - 参数: path (string, 必需), old_string (string, 必需), new_string (string, 必需)

4. bash
   - 描述: 执行shell命令
   - 参数: command (string, 必需), timeout (int, 可选)
    )";
}
```

---

## 9. CLI命令设计

### 命令结构

```
roboclaw [命令] [选项]
```

### 主要命令

#### 9.1 启动对话（默认）
```bash
roboclaw                    # 启动新对话或继续上次对话
roboclaw --new              # 强制创建新对话
roboclaw --continue <id>    # 继续指定ID的对话
```

#### 9.2 分支管理
```bash
roboclaw branch --list              # 查看所有分支
roboclaw branch --new <name>        # 创建新分支
roboclaw branch --switch <id>       # 切换到指定分支
roboclaw branch --merge <id>        # 将分支合并到当前
roboclaw branch --delete <id>       # 删除分支
```

#### 9.3 对话管理
```bash
roboclaw conversation --list        # 列出所有对话
roboclaw conversation --show <id>   # 显示对话详情
roboclaw conversation --delete <id> # 删除对话
roboclaw conversation --export <id> # 导出对话为Markdown
```

#### 9.4 配置管理
```bash
roboclaw config --show              # 显示当前配置
roboclaw config --edit              # 编辑配置文件
roboclaw config --reset             # 重置为默认配置
roboclaw config --set provider <name>   # 设置提供商
roboclaw config --set model <name>      # 设置模型
```

#### 9.5 模型切换
```bash
roboclaw --provider <name>          # 临时切换提供商
roboclaw --model <name>             # 临时切换模型
```

#### 9.6 帮助与版本
```bash
roboclaw --help                     # 显示帮助
roboclaw --version                  # 显示版本
```

### 交互模式界面

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  RoboClaw v1.0.0 | Claude Sonnet 4 | 分支: main
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

你: 帮我看看README的内容

Assistant [正在思考...]

Assistant: 我来帮你读取README文件

[Tool: read] path: "README.md"
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
README.md内容：
这是RoboClaw项目...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

我看到了README的内容，这是一个...

你:
```

### CLI选项

```cpp
struct CLIOptions {
    // 命令
    string command = "chat";  // chat, branch, conversation, config

    // 选项
    bool new_conversation = false;
    string continue_id;
    string provider;
    string model;
    bool verbose = true;

    // 分支选项
    string branch_name;
    string switch_id;
    string merge_id;
    string delete_id;

    // 配置选项
    bool show_config = false;
    bool edit_config = false;
    bool reset_config = false;
};
```

---

## 10. 错误处理与日志系统

### 错误类型分类

```cpp
enum class ErrorCode {
    // 配置错误 (1xxx)
    CONFIG_NOT_FOUND = 1001,
    CONFIG_INVALID = 1002,
    API_KEY_MISSING = 1003,

    // 文件操作错误 (2xxx)
    FILE_NOT_FOUND = 2001,
    FILE_READ_FAILED = 2002,
    FILE_WRITE_FAILED = 2003,
    FILE_EDIT_FAILED = 2004,

    // LLM API错误 (3xxx)
    API_REQUEST_FAILED = 3001,
    API_RATE_LIMIT = 3002,
    API_INVALID_KEY = 3003,
    API_TIMEOUT = 3004,
    API_QUOTA_EXCEEDED = 3005,

    // 对话错误 (4xxx)
    CONVERSATION_NOT_FOUND = 4001,
    BRANCH_NOT_FOUND = 4002,
    NODE_NOT_FOUND = 4003,

    // 工具执行错误 (5xxx)
    TOOL_EXECUTION_FAILED = 5001,
    BASH_COMMAND_FAILED = 5002,
    BASH_COMMAND_TIMEOUT = 5003,
};

struct Error {
    ErrorCode code;
    string message;
    string details;  // 额外细节
    bool recoverable;  // 是否可恢复
};
```

### 错误处理策略

```cpp
class ErrorHandler {
public:
    // 处理错误
    void handle(const Error& error) {
        log(error);

        if (error.recoverable) {
            recover(error);
        } else {
            fatal(error);
        }
    }

private:
    void log(const Error& error) {
        // 记录到日志文件
    }

    void recover(const Error& error) {
        switch (error.code) {
            case ErrorCode::API_RATE_LIMIT:
                // 等待后重试
                waitForRetry();
                break;
            case ErrorCode::API_TIMEOUT:
                // 重试请求
                retry();
                break;
            // ...
        }
    }

    void fatal(const Error& error) {
        // 显示错误信息并退出
        cerr << "致命错误: " << error.message << endl;
        exit(1);
    }
};
```

### 日志系统

```cpp
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    void log(LogLevel level, const string& message) {
        if (level < min_level_) return;

        auto time = getCurrentTime();
        string prefix = getLevelPrefix(level);

        string log_entry = format("[{}] [{}] {}", time, prefix, message);

        // 输出到控制台
        cout << log_entry << endl;

        // 输出到文件
        log_file_ << log_entry << endl;
    }

    void setLogLevel(LogLevel level) {
        min_level_ = level;
    }

private:
    LogLevel min_level_ = LogLevel::INFO;
    ofstream log_file_;
};
```

### 日志输出格式

```
[2025-02-20 14:30:25] [INFO] 启动RoboClaw
[2025-02-20 14:30:25] [DEBUG] 加载配置: /Users/jiang/.roboclaw/config.toml
[2025-02-20 14:30:26] [INFO] 连接到LLM: Anthropic Claude Sonnet 4
[2025-02-20 14:30:27] [DEBUG] 发送消息: "帮我看看README的内容"
[2025-02-20 14:30:28] [INFO] [Tool: read] path: "README.md"
[2025-02-20 14:30:28] [DEBUG] 读取文件: 473 字节
[2025-02-20 14:30:30] [INFO] 收到响应: "我看到了README的内容..."
[2025-02-20 14:30:35] [WARNING] API速率限制，等待10秒后重试
[2025-02-20 14:31:02] [ERROR] 文件不存在: /path/to/file.txt
```

### 用户友好的错误提示

```cpp
string getUserFriendlyMessage(const Error& error) {
    switch (error.code) {
        case ErrorCode::API_KEY_MISSING:
            return "❌ API密钥未配置\n"
                   "请运行: roboclaw config --edit";

        case ErrorCode::API_INVALID_KEY:
            return "❌ API密钥无效\n"
                   "请检查配置: roboclaw config --show";

        case ErrorCode::FILE_NOT_FOUND:
            return "❌ 文件不存在: " + error.details;

        case ErrorCode::API_RATE_LIMIT:
            return "⚠️ API请求过于频繁，请稍后再试";

        default:
            return "❌ 错误: " + error.message;
    }
}
```

---

## 11. 测试策略

### 测试架构

```
tests/
├── unit/              # 单元测试
│   ├── tools/         # 工具测试
│   ├── llm/           # LLM接口测试
│   ├── storage/       # 存储测试
│   └── utils/         # 工具类测试
│
├── integration/       # 集成测试
│   ├── agent/         # Agent流程测试
│   ├── conversation/  # 对话管理测试
│   └── branch/        # 分支操作测试
│
└── e2e/               # 端到端测试
    └── scenarios/     # 完整场景测试
```

### 单元测试示例

```cpp
// tests/unit/tools/read_tool_test.cpp
#include "tools/read_tool.h"
#include <cassert>

void testReadBasicFile() {
    // 准备测试文件
    string test_file = "/tmp/test_read.txt";
    writeTestFile(test_file, "Hello, World!");

    // 测试读取
    ReadTool tool;
    json args = {{"path", test_file}};
    auto result = tool.execute(args);

    // 验证
    assert(result["success"] == true);
    assert(result["content"] == "Hello, World!");

    // 清理
    std::remove(test_file.c_str());
}

void testReadNonExistentFile() {
    ReadTool tool;
    json args = {{"path", "/nonexistent/file.txt"}};
    auto result = tool.execute(args);

    assert(result["success"] == false);
    assert(result["error"] == "File not found");
}

void testReadWithOffsetAndLimit() {
    // 准备多行测试文件
    string test_file = "/tmp/test_read_lines.txt";
    writeTestFile(test_file, "Line1\nLine2\nLine3\nLine4\nLine5");

    ReadTool tool;
    json args = {
        {"path", test_file},
        {"offset", 1},
        {"limit", 2}
    };
    auto result = tool.execute(args);

    assert(result["success"] == true);
    assert(result["content"] == "Line2\nLine3");

    std::remove(test_file.c_str());
}
```

### 集成测试示例

```cpp
// tests/integration/agent/agent_flow_test.cpp
void testAgentToolCallingFlow() {
    // 创建模拟LLM（返回工具调用）
    MockLLM mock_llm;
    mock_llm.setResponse(R"({
        "tool_calls": [{
            "tool": "read",
            "path": "test.txt"
        }]
    })");

    // 创建Agent
    Agent agent(&mock_llm);

    // 发送用户消息
    auto response = agent.process("帮我看看test.txt");

    // 验证工具被调用
    assert(mock_llm.getLastToolCall()["tool"] == "read");

    // 验证结果
    assert(response.find("文件内容") != string::npos);
}
```

### 端到端测试场景

```cpp
// tests/e2e/scenarios/bug_fix_workflow_test.cpp
void testBugFixWorkflow() {
    // 场景：Agent修bug的完整流程

    // 1. 创建初始对话
    Session session = createSession();

    // 2. 用户报告bug
    session.sendUserMessage("程序有个bug，输出不对");

    // 3. Agent读取代码
    // 验证调用了read工具

    // 4. Agent创建分支修复
    session.createBranch("fix-output-bug");
    assert(session.getCurrentBranch() == "fix-output-bug");

    // 5. Agent修改代码
    // 验证调用了edit工具

    // 6. 测试修复
    session.sendUserMessage("测试一下修复");
    // 验证调用了bash工具运行测试

    // 7. 合并分支
    session.mergeBranch("fix-output-bug");

    // 8. 验证主线已更新
}
```

### 测试框架选择

使用 **Catch2** - 轻量级C++测试框架

```cmake
# CMakeLists.txt
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(roboclaw_tests
    tests/unit/**/*.cpp
    tests/integration/**/*.cpp
)

target_link_libraries(roboclaw_tests Catch2::Catch2WithMain)
```

### 运行测试

```bash
# 编译测试
cd build && cmake --build . --target roboclaw_tests

# 运行所有测试
./roboclaw_tests

# 运行特定测试
./roboclaw_tests "testReadBasicFile"

# 详细输出
./roboclaw_tests -s
```

---

## 12. 开发路线图

### 分阶段实施计划

#### 第一阶段：基础框架（2-3周）

```
目标：建立项目骨架，能够编译运行
├── ✅ 创建CMakeLists.txt
├── ✅ 配置第三方依赖（cpr, nlohmann/json, cpptoml）
├── ✅ 实现Logger（日志系统）
├── ✅ 实现ConfigManager（配置管理）
├── ✅ 实现首次运行配置引导
└── ✅ 实现基础CLI框架（--help, --version）
```

**交付物**：可编译的项目，运行`roboclaw --help`显示帮助信息

#### 第二阶段：工具系统（2-3周）

```
目标：实现四个基础工具
├── ✅ 实现Tool基类
├── ✅ 实现Read工具
├── ✅ 实现Write工具
├── ✅ 实现Edit工具
├── ✅ 实现Bash工具（跨平台）
├── ✅ 工具参数验证
└── ✅ 单元测试
```

**交付物**：四个工具可独立使用，测试通过

#### 第三阶段：LLM接口（2-3周）

```
目标：实现LLM通信
├── ✅ 实现LLMProvider抽象接口
├── ✅ 实现HTTPClient（基于cpr）
├── ✅ 实现Anthropic提供商
├── ✅ 实现OpenAI提供商
├── ✅ 实现流式响应处理
├── ✅ 错误处理与重试
└── ✅ 单元测试
```

**交付物**：能够与LLM通信，接收流式响应

#### 第四阶段：Agent引擎（3-4周）

```
目标：实现核心Agent逻辑
├── ✅ 实现PromptBuilder
├── ✅ 实现Agent类
├── ✅ 工具调用解析与执行
├── ✅ 对话历史管理
├── ✅ 消息格式转换
└── ✅ 集成测试
```

**交付物**：Agent能够理解指令并调用工具

#### 第五阶段：对话树系统（3-4周）

```
目标：实现树状对话结构
├── ✅ 实现ConversationNode
├── ✅ 实现ConversationTree
├── ✅ 实现SessionManager
├── ✅ 实现分支创建与切换
├── ✅ 实现树的序列化/反序列化
├── ✅ 实现分支合并
└── ✅ 集成测试
```

**交付物**：完整的对话树功能

#### 第六阶段：CLI完善（2周）

```
目标：完善命令行界面
├── ✅ 实现交互式对话模式
├── ✅ 实现分支管理命令
├── ✅ 实现对话管理命令
├── ✅ 实现配置管理命令
├── ✅ 美化输出格式
└── ✅ 用户文档
```

**交付物**：用户友好的完整CLI

#### 第七阶段：扩展LLM支持（2-3周）

```
目标：支持更多LLM提供商
├── ✅ 实现Gemini提供商
├── ✅ 实现DeepSeek提供商
├── ✅ 实现豆包提供商
├── ✅ 实现通义千问提供商
└── ✅ 提供商切换测试
```

**交付物**：支持6家以上LLM提供商

#### 第八阶段：测试与优化（2-3周）

```
目标：完善测试，优化性能
├── ✅ 端到端测试
├── ✅ 性能优化
├── ✅ 内存泄漏检查
├── ✅ 跨平台测试（macOS, Linux, Windows）
└── ✅ Bug修复
```

**交付物**：稳定可用的v1.0版本

### 时间线总览

```
月1 |----框架----|
月2 |----工具----|
月3 |--LLM---|
月4 |---Agent---|
月5 |--对话树--|
月6 |CLI扩展|
月7 |-LLM扩展-|
月8 |-测试优化-|
```

**总计：约2-3个月（单人开发）**

### 里程碑

| 里程碑 | 描述 | 验收标准 |
|--------|------|----------|
| M1 | 框架搭建完成 | 可编译运行 |
| M2 | 工具系统完成 | 四个工具可用 |
| M3 | LLM通信完成 | 能与Claude对话 |
| M4 | Agent完成 | 能执行工具调用 |
| M5 | 对话树完成 | 支持分支操作 |
| M6 | v1.0发布 | 完整功能可用 |

---

## 13. 开发环境准备

### macOS开发环境

#### 必需软件

**1. Xcode Command Line Tools**
```bash
xcode-select --install
```

**2. CMake（版本3.20+）**
```bash
# 使用Homebrew安装
brew install cmake

# 验证安装
cmake --version
```

**3. Git**
```bash
# macOS通常预装Git，或通过Homebrew安装
brew install git

# 验证安装
git --version
```

**4. 可选：VS Code（推荐IDE）**
```bash
# 通过Homebrew Cask安装
brew install --cask visual-studio-code

# 推荐扩展：
# - C/C++ (Microsoft)
# - CMake Tools (Microsoft)
# - TOML Language Server (be5invis)
```

### 项目初始化步骤

```bash
# 1. 创建项目目录
cd ~/development/RoboClaw

# 2. 初始化Git仓库
git init
git add README.md
git commit -m "Initial commit: README"

# 3. 创建目录结构
mkdir -p src/{cli,session,agent,tools,llm,storage,utils}
mkdir -p tests/{unit,integration,e2e}
mkdir -p docs/plans
mkdir -p build

# 4. 创建.gitignore
cat > .gitignore << 'EOF'
# 构建目录
build/
*.o
*.a
*.so
*.dylib
*.exe

# IDE配置
.vscode/
.idea/
*.swp
*.swo

# macOS
.DS_Store

# RoboClaw运行时数据
.roboclaw/

# 日志
*.log

# 依赖（如果使用本地缓存）
external/
EOF

git add .gitignore
git commit -m "Add .gitignore"
```

### 第一个Hello World程序

```cpp
// src/main.cpp
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char* argv[]) {
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << endl;
    cout << "  RoboClaw v0.1.0 - C++ AI Agent Framework" << endl;
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << endl;

    if (argc > 1 && string(argv[1]) == "--help") {
        cout << "\n用法: roboclaw [命令] [选项]" << endl;
        cout << "  --help     显示帮助信息" << endl;
        cout << "  --version  显示版本信息" << endl;
        return 0;
    }

    if (argc > 1 && string(argv[1]) == "--version") {
        cout << "\nRoboClaw version 0.1.0" << endl;
        return 0;
    }

    cout << "\n欢迎使用 RoboClaw！" << endl;
    cout << "当前为开发中版本，更多功能敬请期待..." << endl;

    return 0;
}
```

### 最简CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(RoboClaw VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(roboclaw src/main.cpp)

install(TARGETS roboclaw DESTINATION bin)
```

### 编译运行

```bash
# 1. 进入构建目录
cd build

# 2. 配置项目
cmake ..

# 3. 编译
cmake --build .

# 4. 运行
./roboclaw
./roboclaw --help
./roboclaw --version

# 5. 安装到系统（可选）
sudo cmake --install .
```

---

## 附录

### A. 参考资源

- **OpenClaw/Pi项目**: 原始灵感来源
- **CMake官方文档**: https://cmake.org/documentation/
- **CPR HTTP库**: https://docs.libcpr.cc/
- **nlohmann/json**: https://json.nlohmann.me/

### B. 术语表

| 术语 | 解释 |
|------|------|
| Agent | AI代理，能够自主执行任务的AI系统 |
| LLM | 大语言模型（Large Language Model） |
| Tool Call | 工具调用，LLM请求执行特定操作的指令 |
| Conversation Tree | 对话树，树状结构的对话历史 |
| Branch | 分支，从主对话派生出的独立对话线 |

### C. 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| 1.0.0 | 2025-02-20 | 初始设计文档 |

---

**文档结束**
