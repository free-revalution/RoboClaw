# 社交软件连接指南

## 支持的平台

RoboPartner 支持连接以下社交平台：

- **Telegram** (推荐) - 全球广泛使用的即时通讯应用
- **钉钉** - 企业级协作平台
- **飞书** - 字节跳动推出的企业协作平台

## 快速开始

### 方式一：使用交互式连接向导（推荐）

在 RoboPartner 中执行:
```
/link
```

按照提示选择平台并输入相应的配置信息。

### 方式二：使用命令行参数

```bash
# 连接 Telegram
robopartner social --platform telegram --token <your_bot_token>

# 连接钉钉
robopartner social --platform dingtalk --app-key <app_key> --app-secret <app_secret>

# 连接飞书
robopartner social --platform feishu --app-id <app_id> --app-secret <app_secret>

# 列出已连接的平台
robopartner social --list

# 断开连接
robopartner social --disconnect <platform_name>
```

## 平台配置详解

### 1. Telegram 连接

#### 1.1 获取 Bot Token

1. 在 Telegram 中搜索 `@BotFather`
2. 发送 `/newbot` 创建新机器人
3. 按提示输入机器人名称和用户名
4. 复制获得的 Token（格式：`1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`）

#### 1.2 配置步骤

```bash
# 启动 RoboPartner
robopartner

# 使用连接命令
>>> /link
选择平台: Telegram
输入 Bot Token: 1234567890:ABCdefGHIjklMNOpqrsTUVwxyz
```

或使用命令行：
```bash
robopartner social --platform telegram --token 1234567890:ABCdefGHIjklMNOpqrsTUVwxyz
```

#### 1.3 使用示例

连接成功后，您可以通过 Telegram 发送指令给 RoboPartner：

```
# 基础指令
/status - 查看系统状态
/agents - 列出可用的 Agents

# 代码分析
帮我分析 src/main.cpp

# 代码生成
用 C++ 写一个串口通信模块

# Agent 管理
列出本地 Agents
启动 Claude Code 处理嵌入式任务

# 硬件控制（如已配置）
前进 50% 速度 2 秒
停止
```

### 2. 钉钉连接

#### 2.1 获取应用凭证

1. 登录 [钉钉开放平台](https://open.dingtalk.com/)
2. 创建企业内部应用
3. 获取 `AppKey` 和 `AppSecret`
4. 设置消息接收地址（如需接收回调）

#### 2.2 配置步骤

```bash
robopartner social --platform dingtalk --app-key <your_app_key> --app-secret <your_app_secret>
```

#### 2.3 使用方式

- 在钉钉中添加机器人到群聊
- 通过 @机器人 发送指令
- 支持文字消息和部分 Markdown 格式

### 3. 飞书连接

#### 3.1 获取应用凭证

1. 登录 [飞书开放平台](https://open.feishu.cn/)
2. 创建企业自建应用
3. 获取 `App ID` 和 `App Secret`
4. 配置事件订阅（如需接收消息）

#### 3.2 配置步骤

```bash
robopartner social --platform feishu --app-id <your_app_id> --app-secret <your_app_secret>
```

## 配置文件

连接信息会保存在配置文件中：

- **位置**: `~/.robopartner/social.json`
- **格式**: JSON

示例：
```json
{
  "telegram": {
    "enabled": true,
    "bot_token": "1234567890:ABCdefGHIjklMNOpqrsTUVwxyz",
    "chat_id": "123456789"
  },
  "dingtalk": {
    "enabled": false
  },
  "feishu": {
    "enabled": false
  }
}
```

## 常见问题

### Q: Telegram Bot 没有响应？

A: 请检查：
1. Bot Token 是否正确
2. 机器人是否已启动（向 Bot 发送 `/start`）
3. 网络连接是否正常

### Q: 如何获取 Telegram Chat ID？

A: 向机器人发送任意消息后，RoboPartner 会自动记录 Chat ID。或使用：
```
https://api.telegram.org/bot<YourBOTToken>/getUpdates
```

### Q: 钉钉/飞书连接失败？

A: 请确保：
1. AppKey/AppSecret 或 AppID/AppSecret 正确
2. 应用已发布并启用
3. 企业账号有相应权限

### Q: 支持哪些指令？

A: 支持 RoboPartner 的所有核心功能：
- 文件操作（Read, Write, Edit）
- Shell 命令执行
- 代码分析和生成
- Agent 管理和协作
- 硬件控制（如已配置）

## 安全建议

1. **保护 Token**: 不要将 Bot Token 或应用密钥提交到版本控制系统
2. **使用环境变量**: 敏感信息可通过环境变量传递
3. **限制权限**: 为机器人配置最小必要权限
4. **定期轮换**: 定期更换 Token 和密钥

## 故障排查

### 启用调试日志

```bash
# 设置环境变量
export ROBOPARTNER_DEBUG=1

# 查看详细日志
robopartner --verbose
```

### 测试连接

```bash
# 测试 Telegram 连接
robopartner social --test telegram

# 测试钉钉连接
robopartner social --test dingtalk

# 测试飞书连接
robopartner social --test feishu
```

## 高级配置

### 自定义命令前缀

默认情况下，Telegram 机器人支持 `/` 前缀的命令。可在配置文件中自定义：

```json
{
  "telegram": {
    "enabled": true,
    "bot_token": "...",
    "command_prefix": "!"
  }
}
```

### 设置 Webhook（推荐用于生产环境）

对于生产环境，建议使用 Webhook 而非轮询：

```bash
robopartner social --webhook --url https://your-domain.com/webhook
```

## 卸载和断开

```bash
# 断开特定平台
robopartner social --disconnect telegram

# 清除所有社交平台连接
robopartner social --clear

# 删除配置文件
rm ~/.robopartner/social.json
```

## 更多资源

- [Telegram Bot API 文档](https://core.telegram.org/bots/api)
- [钉钉开放平台文档](https://open.dingtalk.com/document/)
- [飞书开放平台文档](https://open.feishu.cn/document/)
- [RoboPartner 主文档](../README.md)
