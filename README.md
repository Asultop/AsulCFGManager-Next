# AsulCFGManager-Next

<div align="center">

**AsulCFGManager-Next (ACM-Next)** 是一款基于 Qt/C++ 构建的 CS2 配置文件管理与部署工具。

[![Qt Version](https://img.shields.io/badge/Qt-6.0%2B-blue.svg)](https://www.qt.io/)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

</div>

## 项目简介

AsulCFGManager-Next 提供了一套完整的 CS2 配置文件解决方案，支持表单可视化编辑、Lua 脚本扩展、多语言界面以及一键部署功能。程序采用模块化架构，核心包括自定义表单解析引擎（AFormParser）、Steam SDK 集成以及现代化的用户界面。

## 技术架构

### 核心技术栈

| 组件 | 版本 | 说明 |
|------|------|------|
| Qt | 6.0+ / 5.x | 跨平台 C++ GUI 框架 |
| C++ Standard | C++17 | 现代 C++ 特性支持 |
| ElaWidgetTools | - | 自定义 Qt 控件库，提供现代化 UI 组件 |
| AFormParser | 内置 | `.asul` 表单解析与 CFG 导出引擎 |
| Lua | 5.4+ | 脚本扩展支持 |
| QsLog | - | 日志框架 |
| ValveFileVDF | 1.1.1 | Valve 数据格式解析器 |
| AVF-SDK | - | 文件校验 SDK |

### 模块结构

```
AsulCFGManager-Next/
├── Sources/                      # 应用程序源代码
│   ├── COM_AboutWidget/          # 关于页面
│   ├── COM_DeployWidget/         # 部署功能模块
│   ├── COM_DeployPanelWindow/    # 部署面板窗口
│   ├── COM_HomeWidget/           # 主页模块
│   ├── COM_IconWidget/          # 图标管理模块
│   ├── COM_SettingWidget/       # 设置页面
│   ├── COM_SplashWindow/        # 启动画面
│   ├── COM_ExampleWidget/       # 示例模块
│   ├── Global/                   # 全局设置与函数
│   ├── SystemKit/                # 系统底层组件
│   ├── ToolKit/                  # 工具组件（Steam SDK等）
│   ├── CTL_AsulComboBox/         # 自定义控件
│   └── MainEntry/                # 程序入口
├── AFormParser/                  # 表单解析引擎
│   ├── include/                  # 头文件
│   ├── src/                      # 源代码
│   ├── docs/                     # 开发文档
│   ├── tools/                    # 命令行工具
│   ├── samples/                  # 示例文件
│   └── tests/                    # 单元测试
├── 3rd/                          # 第三方依赖
│   ├── AFormSDK/                 # AFormParser SDK
│   ├── ElaWidgetTools/           # UI控件库
│   ├── AVF-SDK/                  # 文件校验SDK
│   ├── QsLog/                    # 日志库
│   ├── ValveFileVDF-1.1.1/       # VDF解析器
│   └── lua/                      # Lua运行时
└── PropertySetting/              # 项目配置
```

## 功能特性

### 表单解析与配置管理

- **AFormParser 引擎**：解析 `.asul` 自定义表单格式
- **CFG 导出**：将表单导出为游戏可读的 `.cfg` 配置文件
- **Lua 脚本集成**：支持在表单中嵌入 Lua 脚本实现复杂逻辑
- **模板引擎**：支持变量插值与表达式计算

### Steam 集成

- **用户查询**：集成 Steam SDK 查询用户信息
- **头像获取**：自动获取并缓存用户头像
- **云端同步**：支持 Steam 云配置同步（规划中）

### 用户界面

- **现代化设计**：采用 Mica 亚克力等视觉效果
- **主题同步**：支持跟随 Windows 系统主题
- **多语言支持**：简体中文、English
- **启动画面**：展示初始化进度

### 开发工具

- **legacy_to_form**：旧版格式转换工具
- **form_to_cfg**：表单转 CFG 导出工具
- **form_tree_viewer**：表单树结构可视化工具

## 构建指南

### 环境要求

- CMake >= 3.16
- Qt 6.0+ 或 Qt 5.x (Core, Gui, Network, Widgets)
- 支持 C++17 的编译器 (GCC/MinGW, MSVC, Clang)
- Windows 10/11 (当前主要目标平台)

### 构建步骤

1. **配置项目属性**

编辑 `PropertySetting/Program_Property.json` 文件：

```json
{
    "programName": "ACM-Next",
    "targetName": "AsulCFGManager-Next",
    "programOrganization": "Asul",
    "programVersion": "1.0.0",
    "programDescription": "CS2 CFG 管理",
    "programAuthor": "Asul",
    "programLicense": "MIT",
    "programRepository": "https://github.com/AsulTop/AsulCFGManager-Next"
}
```

2. **CMake 配置与构建**

```powershell
cmake -S . -B build
cmake --build build
```

3. **运行程序**

构建产物位于 `build/` 目录。

### 第三方依赖

项目已包含所有第三方依赖的源码或预编译库，无需额外下载：

| 依赖 | 位置 | 说明 |
|------|------|------|
| ElaWidgetTools | `3rd/ElaWidgetTools/` | 需预先编译 |
| AVF-SDK | `3rd/AVF-SDK/` | 预编译库 |
| AFormParser | `3rd/AFormSDK/` | 随项目构建 |
| QsLog | `3rd/QsLog/` | 源码集成 |
| ValveFileVDF | `3rd/ValveFileVDF-1.1.1/` | 源码集成 |
| Lua | `3rd/lua/` | 源码集成 |

## AFormParser 使用指南

AFormParser 是本项目的核心组件，用于解析 `.asul` 表单文件。

### 最小使用示例

```cpp
#include "AFormParser/AFormParser.hpp"

AFormParser::ParseError err;
auto doc = AFormParser::Document::from(formText, &err);
if (!doc) {
    // 处理错误: err.line / err.column / err.message
}
QString cfg = doc->toCFG();
```

### 命令行工具

```powershell
# 转换旧版格式
./legacy_to_form.exe samples/Function_Preference.asul

# 导出为 CFG
./form_to_cfg.exe samples/Function_Preference_Form.asul

# 可视化表单树
./form_tree_viewer.exe samples/Function_Preference_Form.asul

# 运行功能验证
./verify_form_features.exe
```

详细文档请参阅 [AFormParser/docs/](AFormParser/docs/)。

## 项目结构规范

### 命名约定

- **类/模块前缀**：`T_` 表示页面/窗口组件，`F_` 表示功能组件，`C_` 表示控件
- **文件命名**：遵循驼峰式或下划线分隔式
- **目录命名**：组件目录使用 `COM_` 前缀

### 代码规范

- C++17 标准
- Qt 信号槽机制用于组件通信
- 单例模式管理全局状态 (`GlobalSettings`, `AsulApplication`)

## 许可证

本项目基于 [MIT License](LICENSE) 开源。

## 致谢

- [Qt Framework](https://www.qt.io/) - 跨平台 GUI 框架
- [ElaWidgetTools](https://github.com/AsulTop/ElaWidgetTools) - 现代 Qt 控件库
- [QsLog](https://github.com/KDAB/KDAB-Cmake) - 日志框架
- [ValveFileVDF](https://github.com/ 属性) - VDF 解析库
- [Lua](https://www.lua.org/) - 轻量级脚本语言

## 联系方式

- **作者**：Asul
- **仓库**：[AsulTop/AsulCFGManager-Next](https://github.com/AsulTop/AsulCFGManager-Next)

---

*AsulCFGManager-Next - 让 CS2 配置管理更加高效便捷。*
