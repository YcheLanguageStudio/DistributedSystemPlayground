# yche-distsys-playground

基于 **Bazel** 与 **C++17** 的分布式/KV 样例工程：提供可注入执行器与等待回调的 **KV 访问层**，以及面向 **bthread/brpc 风格运行时** 的薄适配；单测与冒烟默认使用**内存后端**，不依赖本机 Redis 或完整 Apache brpc。

更细的构建参数、依赖说明与命令示例见 [docs/build.md](docs/build.md)；设计背景见 [docs/design_kv.md](docs/design_kv.md)。

| 专题 | 文档 |
|------|------|
| Bazel 输出目录（`build/` 等）含义 | [docs/bazel_output.md](docs/bazel_output.md) |
| 覆盖率原理、`gcov`/lcov、沙箱与操作 | [docs/coverage.md](docs/coverage.md) |
| clangd / `compile_commands.json` / Cursor 索引步骤 | [docs/ide_indexing.md](docs/ide_indexing.md) |
| Python 绑定（pybind11）与 `yche_kv` | [docs/bindings_python.md](docs/bindings_python.md) |

---

## `src/` 目录与功能

| 路径 | 作用 |
|------|------|
| [`src/kv/include/yche/kv/`](src/kv/include/yche/kv/) | **对外头文件**：`KvStore`、`KvError`、`KvStoreOptions`，以及 [`runtime_hooks.h`](src/kv/include/yche/kv/adapters/runtime_hooks.h) 中的 `Executor` / `WaitCallback`（不依赖 brpc）。 |
| [`src/kv/`](src/kv/) | **KV 核心实现**：`KvStore` 的 `get`/`set` 在注入的运行时上异步提交任务，连接池与阻塞逻辑在**注入的线程池**（Executor）中执行；`ConnPool` 管理连接占用；**内存后端** 与 **Redis（hiredis）** 两种实现。 |
| [`src/adapters/brpc/`](src/adapters/brpc/) | **运行时适配**：`InstallBrpcRuntime` 使用 **pthread 线程池** 作为 `Executor`，`WaitCallback` 中调用 **`bthread_yield`**（与 [docs/design_kv.md](docs/design_kv.md) 中的思路一致）；`MakeRedisStoreForBrpc` 用于构造 Redis 后端并安装适配。 |

Bazel 目标：`//src/kv:kv_core`、`//src/adapters/brpc:kv_brpc_adapter`。

[`bindings/python/`](bindings/python/) 提供 **pybind11** 包 **`yche_kv`**（详见 [docs/bindings_python.md](docs/bindings_python.md)）。

---

## `tests/` 下验证的用例

**C++（GoogleTest）**：

- **`//tests:kv_store_test`**（[`tests/kv_store_test.cc`](tests/kv_store_test.cc)）：**内存后端** + [`tests/support/test_runtime`](tests/support/test_runtime.h)（同步 `Executor` / `WaitCallback`），**不**链接 brpc。
- **`//tests:brpc_adapter_test`**（[`tests/brpc_adapter_test.cc`](tests/brpc_adapter_test.cc)）：**内存后端** + [`InstallBrpcRuntime`](src/adapters/brpc/brpc_runtime.h)，在 **stub `bthread`** 中执行一次 `set`/`get`（依赖 `//src/adapters/brpc:kv_brpc_adapter` 与 `@brpc`）。

**Python**：目标 **`//tests:kv_store_py_test`**（[`tests/kv_store_py_test.py`](tests/kv_store_py_test.py)），`unittest` 场景与 C++ 单测对齐，调用 **`yche_kv.install_test_runtime()`**。

| 场景 | 断言要点 |
|------|----------|
| 正常读写 | `set` 后 `get` 得到相同值，`last_error == KvError::OK`。 |
| 键不存在 | `get` 未命中，`last_error == KvError::KEY_NOT_FOUND`。 |
| 未安装运行时 | 未调用 `InstallTestRuntime` 时 `get` 失败，`last_error == KvError::UNKNOWN`。 |

GoogleTest 通过 **`MODULE.bazel` 中的 `http_archive`（`com_google_googletest`）** 与 [`third_party/googletest.BUILD`](third_party/googletest.BUILD) 集成（避免 BCR 包在 Bazel 9 下仍使用已移除的原生 `cc_*` 规则）；详见 [docs/build.md](docs/build.md)「测试说明」。

---

## 如何构建

在仓库根目录执行（需已安装 **Bazel**，建议 6+；本仓库在 **Bazel 9** 下验证）：

```bash
# 核心库
bazel build //src/kv:kv_core

# brpc 风格适配（默认依赖 third_party 中的 bthread stub）
bazel build //src/adapters/brpc:kv_brpc_adapter

# 测试与冒烟（C++ / Python）
bazel build //tests:kv_store_test //tests:brpc_adapter_test //smoke:kv_smoke
bazel build //bindings/python:yche_kv //bindings/python:yche_kv_whl_stable //tests:kv_store_py_test //smoke:kv_smoke_py
```

首次构建需联网拉取 **hiredis** 与 BCR 模块；依赖与 `MODULE.bazel.lock` 说明见 [docs/build.md](docs/build.md)。可选使用根目录 **[`build.sh`](build.sh)** 一键组合构建 / 测试 / 冒烟（见 [docs/build.md](docs/build.md) 中「便捷脚本」）。

生成 **`compile_commands.json`**（clangd）见 [docs/ide_indexing.md](docs/ide_indexing.md)；**C++ 覆盖率**见 [docs/coverage.md](docs/coverage.md)；命令速查仍见 [docs/build.md](docs/build.md)。

---

## 如何运行

```bash
# 单元测试（UT）
bazel test //tests:kv_store_test
bazel test //tests:kv_store_py_test
# 或
bazel test //tests/...

# 冒烟（smoke）
bazel run //smoke:kv_smoke
bazel run //smoke:kv_smoke_py
```

详细参数（如 `--test_output=all`、查看产物路径 `./build/bin/smoke/kv_smoke`）见 [docs/build.md](docs/build.md) 中「运行单元测试」「运行冒烟」两节。

---

## 当前已验证的功能

- **KV 语义**：在注入 `Executor` + `WaitCallback` 的前提下，`KvStore::get` / `set` 与内存后端、连接池协作正确。
- **单测路径**：GoogleTest 下 `//tests:kv_store_test`（同步 `test_runtime`）与 `//tests:brpc_adapter_test`（`InstallBrpcRuntime` + bthread）覆盖内存读写、缺失键、未安装运行时及适配层在 bthread 中的读写。
- **Python 绑定**：`yche_kv`（pybind11）与 C++ 单测场景一致的 **`//tests:kv_store_py_test`**，以及 **`//smoke:kv_smoke_py`**。
- **冒烟路径**：`InstallBrpcRuntime` + **stub `bthread`**（`bthread_start_background` / `bthread_join` / `bthread_yield`）下，在子「bthread」中完成一次 `set`/`get` 校验（内存后端）。
- **未作为默认 CI 验证的内容**：真实 Redis 进程、完整 **Apache brpc** 源码替换（需自行改 `MODULE.bazel` 的 `local_path_override` 并满足 brpc 的依赖与 registry）。

---

## 其他

- Git 提交模板：根目录 [`.gitmessage`](.gitmessage)，启用：`git config commit.template .gitmessage`。
- Bazel 安装提示：[`tools/install_bazelisk.sh`](tools/install_bazelisk.sh)。
