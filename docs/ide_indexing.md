# IDE 与代码索引：`compile_commands.json`、clangd 与 Cursor

「能跳转、能补全、少红线」依赖**语言服务器知道与你真实编译相同的 include 路径与宏**。本仓库用 **clangd** + **`compile_commands.json`**（可选 **[`.clangd`](../.clangd)** 兜底）。本文说明**原理**与**操作步骤**。

## 1. 两种「索引」不要混为一谈

| 机制 | 作用 | 依赖 |
|------|------|------|
| **Cursor / 编辑器工作区索引** | 全文搜索、文件树、部分 AI 上下文 | 扫描工作区文件；受 [`.cursorignore`](../.cursorignore) 等影响 |
| **clangd（LSP）** | 跳转到定义、补全、诊断、重命名等 **C++ 语义** | 需要**编译数据库**或等价的编译参数 |

C++ 的语义分析不能仅靠「扫源码」，必须与**真实编译命令**一致；因此 **`compile_commands.json`** 是核心。

## 2. `compile_commands.json` 是什么

**编译数据库（Compilation Database）** 是一个 JSON 列表，每条记录大致包含：

- **`file`**：源文件路径  
- **`arguments` / `command`**：编译该文件时使用的完整编译器命令（含 **`-I`**、**`-D`**、**`-std`** 等）

**clangd** 根据当前打开的文件，在数据库里找到对应条目，用**同一套参数**调用 Clang 前端做解析，从而与真实构建**一致**（在数据库正确的前提下）。

本仓库用 [hedron compile commands](https://github.com/hedronvision/bazel-compile-commands-extractor) 从 Bazel 提取命令，目标定义在根目录 [`BUILD.bazel`](../BUILD.bazel) 的 **`refresh_compile_commands`** 中。

## 3. 生成与更新（推荐流程）

在**仓库根目录**执行：

```bash
bazel run --config=compile-commands //:refresh_compile_commands
```

**为何需要 `--config=compile-commands`？**  
日常构建使用 [`.bazelrc`](../.bazelrc) 中的 **`--symlink_prefix=build/`**，生成器仍期望在根目录看到经典布局下的 **`bazel-out`** 链接；该 config 会临时改用 **`bazel-`** 前缀，与 [hedron 文档](https://github.com/hedronvision/bazel-compile-commands-extractor) 行为一致。

生成结果：**仓库根目录的 `compile_commands.json`**（体积可能较大，默认在 [`.gitignore`](../.gitignore) 中忽略）。

**何时需要重新生成？**

- 改了 **`BUILD.bazel` / 依赖 / 编译选项**
- 切换了工具链或 Bazel 配置后跳转/诊断明显不对

## 4. `.clangd` 的作用（兜底）

若尚未生成 `compile_commands.json`，或个别文件未出现在数据库中，根目录 **[`.clangd`](../.clangd)** 可为 clangd **追加** `-I` 等标志（例如 `src/kv/include`），使 **`#include "yche/kv/..."`** 能解析。

**注意**：有 **`compile_commands.json`** 时，clangd **优先**使用数据库；`.clangd` 的 `CompileFlags.Add` 通常会**合并**进去。完整、准确的语义仍以**定期刷新 compile_commands** 为准。

## 5. Cursor 中与索引相关的设置

- **`compile_commands.json`** 若被 [`.cursorignore`](../.cursorignore) 列出，可能减少无关扫描；**clangd 仍可从磁盘读取该文件**（与扩展实现有关；若遇异常可暂时从 cursorignore 中移除该项做对比）。
- **忽略 `build/`**：`build/` 仅为 Bazel 便捷链接（见 [bazel_output.md](bazel_output.md)），对阅读源码帮助有限，忽略可减轻无关索引。

## 6. 让 clangd 重新加载

修改 `compile_commands.json` 或 `.clangd` 后：

1. 命令面板（`Ctrl+Shift+P` / `Cmd+Shift+P`）→ **`Clangd: Restart language server`**  
2. 仍异常时 → **`Developer: Reload Window`**

需安装 **LLVM 官方的 clangd 扩展**；若同时启用微软 **C/C++** 扩展，可能冲突，建议只保留一套 C++ 语言服务。

## 7. 原理小结

- **Bazel** 为每个编译单元计算真实命令；**hedron** 把这些命令导出为 **`compile_commands.json`**。  
- **clangd** 用与编译器一致的参数解析 AST，故跳转/诊断与 **Bazel 构建**对齐。  
- **`.clangd`** 在无数据库或缺项时提供最小 include，属于**补充**，不能替代长期维护的 **compile_commands**。

更多构建命令速查见 [build.md](build.md)；Bazel 输出目录含义见 [bazel_output.md](bazel_output.md)。

## 8. 排错：`#include "yche/kv/..."` 报 file not found

这类路径**不是**磁盘绝对路径，而是相对 **include 根**（本仓库为 **`src/kv/include`**）的路径。要让语言服务识别，必须在「编译参数」里出现 **`-I src/kv/include`**（或等价绝对路径）。

| 现象 | 常见原因 |
|------|----------|
| 新克隆仓库就有红线 | **`compile_commands.json` 在 [`.gitignore`](../.gitignore) 里**，不会随 Git 提交；未执行 **`bazel run --config=compile-commands //:refresh_compile_commands`** 则根目录没有该文件，clangd 只能靠 **[`.clangd`](../.clangd)**，部分场景仍异常。 |
| 有 `compile_commands.json` 仍报错 | **工作区根目录不是仓库根**（多文件夹工作区时 `compile_commands` 不在当前根）；或 **未装 clangd 扩展** / 仍由 **微软 C/C++** 用错误配置做诊断。 |
| 仅头文件红线 | 数据库里对 **`store.h`** 的条目应带 **`-Isrc/kv/include`**；若过期，**重新生成** `compile_commands.json`。 |

本仓库在 **[`.vscode/settings.json`](../.vscode/settings.json)** 中为 clangd 指定 **`--compile-commands-dir=${workspaceFolder}`**，并为 **`C_Cpp.default.compileCommands`** 指向根目录的 **`compile_commands.json`**，避免 Cursor/VS Code 找不到数据库。生成数据库后执行 **Clangd: Restart language server**。

更多见上文 **「让 clangd 重新加载」**。

## 9. 索引修复记录（仓库内已落地的变更）

以下变更用于解决 **`#include "yche/kv/..."`** 等在 IDE 中「找不到头文件」、跳转/补全异常等问题，**不改变**源码里的 include 写法；仍需在本地生成 **`compile_commands.json`**（见上文第 3 节）。

| 变更 | 文件 | 作用 |
|------|------|------|
| **clangd 显式使用仓库根的数据库** | [`.vscode/settings.json`](../.vscode/settings.json) | 设置 **`clangd.arguments`**：`--compile-commands-dir=${workspaceFolder}`，避免多根工作区或未自动探测到根目录 **`compile_commands.json`** 时 clangd 落空。 |
| **微软 C/C++ 扩展默认使用同一数据库** | [`.vscode/settings.json`](../.vscode/settings.json) | **`C_Cpp.default.compileCommands`** 指向 **`${workspaceFolder}/compile_commands.json`**，与 clangd 共用一套编译参数，减少与 Bazel 不一致的 IntelliSense。 |
| **IntelliSense 配置绑定 compile_commands** | [`.vscode/c_cpp_properties.json`](../.vscode/c_cpp_properties.json) | 单配置 **`compileCommands`** 指向根目录 **`compile_commands.json`**，避免依赖手工维护 **`includePath`**。 |
| **无数据库时的 include 兜底**（既有） | [`.clangd`](../.clangd) | **`CompileFlags.Add`** 含 **`-I src/kv/include`** 等；在未生成 **`compile_commands.json`** 或个别文件无条目时，仍尽量解析 **`yche/kv/...`**。 |
| **排错说明**（既有） | 本文第 8 节 | 归纳「未生成数据库 / 工作区根不对 / 头文件条目过期」等场景与处理。 |

**说明**：**`compile_commands.json`** 仍由 [`.gitignore`](../.gitignore) 忽略、不随仓库提交；新克隆后须执行 **`bazel run --config=compile-commands //:refresh_compile_commands`**，再 **重启 clangd** 或 **Reload Window**。
