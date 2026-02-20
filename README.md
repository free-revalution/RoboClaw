# RoboClaw

> 用C++复现OpenClaw项目Pi引擎的极简AI Agent框架

## 项目简介

RoboClaw是一个用C++编写的极简AI Agent框架，灵感来源于OpenClaw项目的Pi引擎。与主流Agent框架（如LangChain、AutoGPT）不同，RoboClaw采用"少即是多"的设计哲学：

- **只有4个工具**：Read（读文件）、Write（写文件）、Edit（编辑文件）、Bash（执行命令）
- **极简系统提示词**：最短的Agent系统提示词
- **自编码能力**：需要新功能时让Agent自己写代码，而不是安装插件
- **树状对话结构**：支持分支，修bug不影响主线

## 核心特性

### 四个基础工具

| 工具 | 功能 |
|------|------|
| **Read** | 读取文件内容，支持分页读取 |
| **Write** | 创建新文件或覆盖现有文件 |
| **Edit** | 精确替换文件内容 |
| **Bash** | 执行shell命令，跨平台支持 |

### 树状对话结构

```
主线: A → B → C → D
            ↓
创建分支:  → E → F (修复bug)
```

- 从任意节点创建新分支
- 分支拥有独立历史
- 修复完成后可以合并回主线

### 多LLM提供商支持

- Anthropic (Claude)
- OpenAI (GPT)
- Google Gemini
- 深度求索
- 字节豆包
- 阿里通义千问

## 开发环境

### 必需软件

- CMake 3.20+
- C++20编译器（GCC 10+, Clang 12+, MSVC 2019+）
- Git

### macOS开发环境

```bash
# 安装Xcode命令行工具
xcode-select --install

# 安装CMake
brew install cmake
```

### 编译运行

```bash
# 克隆项目
git clone https://github.com/yourusername/RoboClaw.git
cd RoboClaw

# 创建构建目录
mkdir build && cd build

# 配置项目（CMake自动下载依赖）
cmake ..

# 编译
cmake --build .

# 运行
./roboclaw --help
```

## 项目结构

```
RoboClaw/
├── CMakeLists.txt          # CMake配置
├── README.md               # 项目说明
├── docs/                   # 文档
│   └── plans/              # 设计文档
├── src/                    # 源代码
│   ├── cli/                # 命令行界面
│   ├── session/            # 会话管理
│   ├── agent/              # Agent引擎
│   ├── tools/              # 工具实现
│   ├── llm/                # LLM接口
│   ├── storage/            # 存储层
│   └── utils/              # 工具类
└── tests/                  # 测试代码
```

## 开发状态

当前版本：**v0.1.0** (开发中)

- [x] 项目框架搭建
- [x] CMake构建配置
- [ ] 工具系统实现
- [ ] LLM接口实现
- [ ] Agent引擎实现
- [ ] 对话树系统实现
- [ ] CLI完善

详细设计文档请查看：[docs/plans/2025-02-20-roboclaw-design.md](docs/plans/2025-02-20-roboclaw-design.md)

## 许可证

MIT License

## 致谢

- OpenClaw项目 - 原始灵感来源
- 所有开源贡献者

---

**注意**: 本项目目前处于早期开发阶段，许多功能尚未实现。欢迎贡献代码！
