# Bazel 输出目录结构与含义

本仓库在根目录 [`.bazelrc`](../.bazelrc) 中配置了 **`--symlink_prefix=build/`**。理解「仓库里看得见的路径」与「Bazel 真正的输出根」有助于定位二进制、中间产物与日志。

## 1. 两套路径：工作区内的链接 vs 输出基座

| 概念 | 位置（本仓库默认） | 含义 |
|------|-------------------|------|
| **工作区（workspace）** | 克隆下来的 Git 目录 | 源码、`BUILD.bazel`、`MODULE.bazel` 所在处；**不是**编译产物的唯一存放地。 |
| **便捷符号链接前缀** | 仓库下的 **`build/`** | Bazel 在每次成功构建后，把常用入口链到「当前配置下的输出树」，方便你直接跑 `./build/bin/...`，而不用记哈希路径。 |
| **输出基座（output base）** | Linux 上多为 **`~/.cache/bazel/_bazel_<uid>/<hash>/`** | Bazel 的**沙箱、action 缓存、execroot、大量真实文件**都在这里；**不在**仓库内的 `build/` 目录里。 |

因此：**`build/` 主要是「快捷方式视图」**；**重度存储与执行发生在 output base**。

## 2. `build/` 下常见条目（`--symlink_prefix=build/`）

名称可能随 Bazel 版本略有差异，常见包括：

| 路径（相对仓库根） | 典型含义 |
|-------------------|----------|
| **`build/bin/`** | 当前构建配置下可执行文件与相关运行文件的便捷入口（例如 `build/bin/smoke/kv_smoke`）。 |
| **`build/out/`** | 常指向本次配置下的 **`bazel-out`** 所代表的输出树（编译产物、对象文件等组织方式由 Bazel 管理）。 |
| **`build/testlogs/`** | 测试运行的日志汇总入口。 |
| **以工作区命名的链接** | 指向 **execroot**（执行构建时「挂载」源码与生成文件的根），用于调试路径问题。 |

这些链接指向的是 **output base 内部**某棵输出树，**不要**把 `build/` 当成「唯一真相」去备份；清理 **`bazel clean`** 或切换配置后，重新构建会更新链接目标。

## 3. 与经典 `bazel-*` 布局的对应关系

未设置 `symlink_prefix` 时，习惯上会在仓库根看到 **`bazel-bin`**、**`bazel-out`**、**`bazel-testlogs`** 等。本仓库把它们收拢到 **`build/`** 下，含义不变：

- 原 **`bazel-bin`** 一类「可运行产物」视图 → 本仓库中主要看 **`build/bin`**（以及文档中给出的具体目标路径说明）。
- 原 **`bazel-out`** → 与 **`build/out`** 所链出的输出树概念一致（**hedron 生成 `compile_commands.json` 时会检查根目录的 `bazel-out`**，因此刷新命令使用单独的 `--config=compile-commands`，见 [IDE 与代码索引](ide_indexing.md)）。

## 4. Output base 里大致有什么（为何与 `build/` 不同）

在 output base 中（路径较长，一般不必手改），常见包括：

- **execroot**：某次构建视角下的「执行根」，源码与生成文件如何拼在一起由 Bazel 决定。
- **沙箱（sandbox）**：具体动作往往在隔离目录中执行（见 [coverage.md](coverage.md) 中的沙箱说明）。
- **Action cache**：增量构建复用编译结果。

若想把 output base 固定到本机某目录，可在**个人**配置中使用 **`startup --output_base=绝对路径`**；**不建议**把机器相关绝对路径提交进仓库。

## 5. 清理与迁移提示

- 从「根目录一堆 `bazel-*`」迁到本仓库布局后，若残留旧链接，可 **`bazel clean`** 后再构建。
- 日常 **`git status`** 一般不应跟踪 `build/`（若未忽略，请把 `build/` 加入 `.gitignore`）；本仓库通过 **`build/`** 集中生成物视图，避免污染仓库根。

更短的命令速查仍见 [build.md](build.md)。
