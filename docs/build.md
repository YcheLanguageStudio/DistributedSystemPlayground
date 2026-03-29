# 构建与运行

本仓库使用 **Bazel**（bzlmod）与 **C++17**。已在 **Bazel 9** + `rules_cc` 下验证通过。

**专题文档**：输出目录释义见 [bazel_output.md](bazel_output.md)；覆盖率原理与操作见 [coverage.md](coverage.md)；clangd / 编译数据库与 Cursor 索引见 [ide_indexing.md](ide_indexing.md)。

## 依赖与环境

| 项目 | 说明 |
|------|------|
| Bazel | 建议 **6+**；当前 CI/本机验证为 **9.x**。若使用 [Bazelisk](https://github.com/bazelbuild/bazelisk)，仓库根目录曾提供 `.bazelversion`（可选，用于固定版本）。 |
| C++ 编译器 | GCC/Clang，支持 **`-std=c++17`**。 |
| 网络 | 首次构建需下载 **hiredis** 源码包（`MODULE.bazel` 中 `http_archive`）及 BCR 模块。 |
| Redis | **可选**。默认单测与冒烟使用 **内存后端**；仅在使用 `KvStoreOptions::Backend::Redis` 或 `MakeRedisStoreForBrpc` 时需要可用的 Redis。 |
| Apache brpc | **可选**。当前通过 **`third_party/brpc_stub_module`** 提供最小 `bthread_*` 实现以便默认可编译；若需真实 brpc，将官方仓库置于 `third_party/brpc` 并修改根 `MODULE.bazel` 中的 `local_path_override`（版本需与 brpc 的 `MODULE.bazel` 一致）。 |

## 输出目录（`build/`）

完整释义见 [bazel_output.md](bazel_output.md)。以下为速览。

根目录 [`.bazelrc`](../.bazelrc) 中配置了 **`--symlink_prefix=build/`**：构建成功后，**便捷符号链接**会出现在 **`build/`** 下（例如 `build/bin`、`build/out`、`build/testlogs`，以及指向 execroot 的工作区名链接），不再在仓库根目录生成 `bazel-*`。

- **二进制**：例如冒烟可执行文件为 `build/bin/smoke/kv_smoke`（与原先 `./bazel-bin/...` 对应）。
- **真实沙箱与编译缓存**：仍在 Bazel 的 **output base**（Linux 上多为 `~/.cache/bazel/_bazel_<用户>/...`），**不在**仓库内的 `build/`。若希望把整个 output base 也挪到本机固定路径，可在**个人**配置里使用启动项 `startup --output_base=绝对路径`（不推荐把机器相关绝对路径提交进仓库）。

从旧布局（根目录 `bazel-*`）迁移后，可执行一次 `bazel clean` 再构建，以去掉根目录遗留链接。

## IDE / 索引（Cursor、C++）

步骤与原理见 [ide_indexing.md](ide_indexing.md)。以下为命令摘要。

- **Cursor / 语义索引**：默认会索引工作区；`build/` 仅为链接与构建视图，对阅读源码帮助不大。仓库根目录的 [`.cursorignore`](../.cursorignore) 已忽略 **`build/`**，减少干扰、加快索引。
- **`compile_commands.json`（clangd）**：本仓库使用 [hedron compile commands](https://github.com/hedronvision/bazel-compile-commands-extractor)（见根目录 [`MODULE.bazel`](../MODULE.bazel)、[`BUILD.bazel`](../BUILD.bazel)）。生成或更新根目录的 `compile_commands.json`：

  ```bash
  bazel run --config=compile-commands //:refresh_compile_commands
  ```

  **说明**：生成器会检查仓库根目录下的 **`bazel-out`** 符号链接。日常构建使用 [`.bazelrc`](../.bazelrc) 里的 `--symlink_prefix=build/`，因此刷新时请加上 **`--config=compile-commands`**（临时改用 `bazel-*` 链接布局）。也可用上游目标 `bazel run --config=compile-commands @hedron_compile_commands//:refresh_all`（同样需该 config）。

  生成的 `compile_commands.json` 已在 [`.gitignore`](../.gitignore) 中忽略；需要提交时可用 `git add -f compile_commands.json`。

## 代码覆盖率（C++）

原理、沙箱与排错见 [coverage.md](coverage.md)。根目录 [`.bazelrc`](../.bazelrc) 中定义了 **`--config=coverage`**（采集覆盖率、`instrumentation_filter` 指向 `//src/kv` 与 `//tests`、合并 **lcov** 报告）。

```bash
# 示例：跑单测并生成合并报告（输出路径见命令结尾的 INFO）
bazel coverage --config=coverage //tests:kv_store_test
```

也可使用包装脚本：在 **`--config=coverage`** 与 **`GCOV`** 配对之外，成功结束后会**自动生成** **`./coverage-html/index.html`**（需 **`lcov`**；不需要时用 **`SKIP_COVERAGE_HTML=1`**）。

```bash
chmod +x tools/coverage.sh   # 仅需一次
./tools/coverage.sh //tests:kv_store_test
```

**GCOV 与编译器须一致**：若 **`gcc`** 与默认 **`gcov`** 主版本不一致（例如 `gcc` 为 9、`gcov` 指向 13），会出现 **`version 'A95*', prefer 'B33*'`** 与 **`gcov` 段错误**。优先使用 **`./tools/coverage.sh`**（会按 **`${CC:-gcc}`** 推导配套 **`gcov`** 并传入沙箱）；或手动指定，例如：

```bash
GCOV=/usr/bin/x86_64-linux-gnu-gcov-9 bazel coverage --config=coverage //tests:kv_store_test
```

详见 [coverage.md](coverage.md)。合并后的 lcov 路径见日志中的 **`INFO: LCOV coverage report is located at ...`**；**`./tools/coverage.sh`** 会在 Bazel 结束后生成 **`coverage-html/index.html`**（纯手打 **`bazel coverage`** 时则需自行 **`genhtml`**，见 [coverage.md](coverage.md)）。

## 常用命令（速查）

```bash
# 构建 KV 核心库
bazel build //src/kv:kv_core

# 构建 brpc 风格运行时适配（pthread 池 + bthread_yield）
bazel build //src/adapters/brpc:kv_brpc_adapter

# 运行全部单元测试（见下方「运行 UT」）
bazel test //tests/...

# 构建并运行冒烟（见下方「运行冒烟」）
bazel run //smoke:kv_smoke
```

## 运行单元测试（UT）

单元测试在 [`tests/`](../tests/) 下，由 Bazel 的 **`cc_test`** 目标驱动；当前用例为 **`//tests:kv_store_test`**（内存后端 + 同步 `test_runtime` 适配，不依赖 Redis / 真实 brpc）。

在**仓库根目录**执行：

```bash
cd /path/to/yche-distsys-playground

# 只跑 KV 单测目标（推荐日常）
bazel test //tests:kv_store_test

# 跑 tests 包下所有 cc_test
bazel test //tests/...

# 失败时看详细输出（含 stdout/stderr）
bazel test //tests:kv_store_test --test_output=all

# 需要时打印测试日志路径
bazel test //tests:kv_store_test --test_summary=detailed
```

**如何判断通过**：命令退出码为 **0**，且终端出现类似 `//tests:kv_store_test PASSED`。失败时 Bazel 会打印失败原因并返回非 0。

**说明**：单测源码为 [`tests/kv_store_test.cc`](../tests/kv_store_test.cc)；辅助桩为 [`tests/support/test_runtime.*`](../tests/support/test_runtime.h)。

## 运行冒烟（smoke）

冒烟在 [`smoke/`](../smoke/) 下，目标为 **`//smoke:kv_smoke`**：在 **bthread stub** 里对 **内存后端** 的 `KvStore` 做一次 `set`/`get` 校验（依赖 [`//src/adapters/brpc:kv_brpc_adapter`](../src/adapters/brpc/)），用于快速验证「运行时适配 + KV 链路」能跑通。

在**仓库根目录**执行：

```bash
cd /path/to/yche-distsys-playground

# 构建并执行冒烟（Bazel 会编译依赖后运行二进制）
bazel run //smoke:kv_smoke

# 显式看进程退出码（0 表示通过）
bazel run //smoke:kv_smoke; echo "exit=$?"

# 也可先构建再直接跑产物（路径以 Bazel 输出为准；便捷链接在 build/bin，见下方「输出目录」）
bazel build //smoke:kv_smoke
./build/bin/smoke/kv_smoke; echo "exit=$?"
```

**如何判断通过**：进程退出码为 **0**；非 0 表示逻辑或构建失败（源码见 [`smoke/kv_smoke.cc`](../smoke/kv_smoke.cc)）。

## 目录与头文件

- **对外 C++ 头文件**位于 [`src/kv/include/`](../src/kv/include/)（例如 `yche/kv/store.h`），通过 `//src/kv:kv_core` 的 `includes` 暴露。
- **设计说明**见 [design_kv.md](design_kv.md)。

## 测试说明（GoogleTest）

在 **Bazel 9** 下，BCR 中部分 `googletest` 发行包仍使用已移除的 **原生 `cc_*` 规则**，与本仓库的 `rules_cc` 组合可能分析失败。当前 [`tests/kv_store_test.cc`](../tests/kv_store_test.cc) 使用 **`assert` 的轻量单测**；后续可在上游 `googletest` 与 Bazel 9 完全对齐后恢复 gtest。

## 第三方与校验

- **hiredis**：`MODULE.bazel` 中固定 **v1.2.0** 的 `sha256`；若上游 tarball 变更导致校验失败，需更新 `sha256` 与 `strip_prefix`。
- **hiredis BUILD**：[`third_party/hiredis.BUILD`](../third_party/hiredis.BUILD) 使用 `rules_cc` 的 `cc_library`，并为 `async.c` 所 `#include "dict.c"` 声明 **`textual_hdrs`**。

## Bazel 安装提示

若尚未安装 Bazel，可使用仓库内脚本（将 **Bazelisk** 安装为 `~/.local/bin/bazel`）：

```bash
./tools/install_bazelisk.sh
```

详见脚本内注释。
