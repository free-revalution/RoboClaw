# 社交软件连接与 Agent 协作设计

**日期**: 2025-02-22
**状态**: 设计已验证
**作者**: Claude (with user validation)

## 概述

为 RoboClaw 添加两大核心功能：
1. **社交软件连接** - 通过 `/link` 指令连接 WhatsApp、飞书、钉钉、Telegram 等
2. **Agent 协作** - 发现并控制本地 Agent（Claude Code、Cursor、OpenClaw 等）实现专业化任务处理

## 技术选型

### 社交软件连接
- **首选**: Telegram Bot API（稳定、免费、C++ 库成熟）
- **备选**: 钉钉/飞书开放平台（国内用户友好）
- **暂不推荐**: WhatsApp（需要商业账户）

### Agent 协作
- **模式**: 混合协作模式 - RoboClaw 主控协调，外部 Agent 专注实现

## 架构设计

### 整体架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                        用户交互层                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐ │
│  │   CLI 终端   │  │  社交软件     │  │   浏览器控制        │ │
│  │              │  │  /link 连接   │  │   (已实现)          │ │
│  └──────────────┘  └──────────────┘  └──────────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│                    RoboClaw 主控层                           │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │            任务协调器 (TaskCoordinator)                   │  │
│  │  - 接收用户指令                                           │  │
│  │  - 分析任务类型和能力需求                                 │  │
│  │  - 决定：自主处理 OR 委托给外部 Agent                    │  │
│  └──────────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                     社交软件连接层                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │ Telegram     │  │    钉钉      │  │    飞书      │       │
│  │  Adapter     │  │   Adapter    │  │   Adapter    │       │
│  └──────────────┘  └──────────────┘  └──────────────┘       │
├─────────────────────────────────────────────────────────────────┤
│                     Agent 协作层                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │  Claude Code │  │   Cursor     │  │  OpenClaw    │       │
│  │  Bridge      │  │   Bridge     │  │  Bridge      │       │
│  └──────────────┘  └──────────────┘  └──────────────┘       │
└─────────────────────────────────────────────────────────────────┘
```

## 核心接口定义

### 社交软件适配器接口

```cpp
// 消息结构
struct SocialMessage {
    string platform_id;      // 平台标识 (telegram, dingtalk, feishu)
    string chat_id;          // 会话/群组 ID
    string user_id;          // 发送者 ID
    string content;          // 消息内容
    string message_id;       // 消息唯一 ID
    int64_t timestamp;       // 时间戳
    json metadata;           // 平台特定元数据
};

// 统一适配器接口
class ISocialAdapter {
public:
    virtual bool connect(const json& config) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    // 消息接收
    virtual vector<SocialMessage> receiveMessages() = 0;

    // 消息发送
    virtual bool sendMessage(const string& chat_id, const string& content) = 0;
    virtual bool sendFile(const string& chat_id, const string& file_path) = 0;

    // 命令处理
    virtual string getCommandPrefix() const = 0;
};
```

### Agent Bridge 接口

```cpp
// Agent 能力定义
struct AgentCapability {
    string id;              // 能力 ID
    string name;            // 能力名称
    string category;        // 分类: coding, debugging, testing, analysis
    vector<string> tags;    // 标签: cpp, python, web, embedded
    int proficiency;        // 熟练度 (0-100)
};

class IAgentBridge {
public:
    virtual bool launch(const string& agent_id) = 0;
    virtual bool sendTask(const string& task_description, const json& context) = 0;
    virtual json waitForResult(int timeout_ms) = 0;
    virtual bool terminate() = 0;
    virtual bool isRunning() const = 0;
};
```

## 数据流设计

### 社交软件 → RoboPartner

```
用户在 Telegram 发送: "帮我分析 src/main.cpp"
    ↓
Telegram Adapter 接收消息
    ↓
转换为 SocialMessage 格式
    ↓
传递给任务协调器
    ↓
任务协调器分析: 这是代码分析任务
    ↓
决策: RoboPartner 自主处理 (有 Read 工具)
    ↓
调用 Read 工具读取文件
    ↓
调用 LLM 分析代码
    ↓
生成回复
    ↓
Telegram Adapter 发送回 Telegram
```

### RoboPartner → 外部 Agent

```
用户在 Telegram 发送: "用 C++ 写一个串口通信模块"
    ↓
任务协调器分析: 这是嵌入式 C++ 开发
    ↓
查询能力矩阵: Claude Code 对嵌入式 C++ 熟练度高
    ↓
决策: 委托给 Claude Code
    ↓
ClaudeCodeBridge 启动 Claude Code
    ↓
传递任务上下文
    ↓
Claude Code 生成代码
    ↓
RoboPartner 整合结果 (创建文件、更新 CMakeLists.txt)
    ↓
发送完成通知
```

## 配置文件结构

```json
{
  "social_adapters": {
    "telegram": {
      "enabled": true,
      "bot_token": "bot_token_here",
      "allowed_users": ["user1", "user2"],
      "command_prefix": "/"
    },
    "dingtalk": {
      "enabled": false,
      "app_key": "...",
      "app_secret": "..."
    }
  },
  "agent_bridges": {
    "claude_code": {
      "enabled": true,
      "vscode_path": "/path/to/code",
      "workspace": "/path/to/project"
    }
  },
  "task_routing": {
    "auto_delegate": true,
    "confidence_threshold": 70,
    "fallback_to_self": true
  }
}
```

## 实施阶段

### 阶段 1：Telegram 连接（P0，2-3 天）
- 实现 TelegramBotAdapter
- /link 指令基础功能
- 消息收发测试
- 配置管理

### 阶段 2：任务协调器增强（P0，2-3 天）
- 任务分析算法
- 能力矩阵定义
- 路由决策逻辑

### 阶段 3：Claude Code Bridge（P1，3-4 天）
- VSCode Extension IPC 通信
- 任务传递和结果接收

### 阶段 4：国内平台支持（P1，3-4 天）
- 钉钉适配器
- 飞书适配器

### 阶段 5：其他 Agent Bridge（P2，2-3 天）
- Cursor Bridge
- OpenClaw Bridge

**总计：约 12-17 天**

## 目录结构

```
src/
├── social/                    # 社交软件适配器
│   ├── social_adapter.h       # 统一适配器接口
│   ├── telegram_adapter.h/cpp # Telegram 实现
│   ├── dingtalk_adapter.h/cpp  # 钉钉实现
│   └── feishu_adapter.h/cpp    # 飞书实现
├── agent/                     # Agent 协作
│   ├── task_coordinator.h/cpp  # 任务协调器
│   ├── agent_bridge.h         # Agent Bridge 接口
│   ├── claude_code_bridge.h/cpp
│   ├── cursor_bridge.h/cpp
│   └── openclaw_bridge.h/cpp
└── cli/                       # CLI 命令
    └── link_command.h/cpp      # /link 指令实现
```

## 依赖库

- **Telegram**: tgbot-cpp (C++ Telegram Bot API)
- **钉钉**: libcurl (HTTP 通信)
- **飞书**: libcurl (HTTP 通信)
- **VSCode IPC**: platform-specific (Unix socket / named pipes)
