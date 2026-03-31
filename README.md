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
| [`src/kv/include/yche/kv/`](src/kv/include/yche/kv/) | **对外头文件**：`KvStore`（同步核心接口）、`KvError`、`KvStoreOptions`，以及 [`adapters/async_store.h`](src/kv/include/yche/kv/adapters/async_store.h) / [`runtime_hooks.h`](src/kv/include/yche/kv/adapters/runtime_hooks.h)（`AsyncKvStore` + `Executor` / `WaitCallback`）。 |
| [`src/kv/`](src/kv/) | **KV 核心实现**：`KvStore` 负责同步 `get`/`set` 与连接池；`AsyncKvStore` 负责“丢给执行器 + waiter 协作等待”的适配层。 |
| [`tests/async_kvcstore/adapters/`](tests/async_kvcstore/adapters/) | **测试/冒烟侧运行时适配**：`InstallThreadPoolRuntime`（pthread 线程池样例）与 `InstallBrpcExecutorRuntime`（brpc/bthread executor 样例）作用于 `AsyncKvStore`。 |

Bazel 目标：`//src/kv:kv_core`、`//src/kv:kv_adapters`、`//tests:kv_brpc_test_adapter`。

[`bindings/python/`](bindings/python/) 提供 **pybind11** 包 **`yche_kv`**（详见 [docs/bindings_python.md](docs/bindings_python.md)）。

---

## `tests/` 目录分层

**C++（GoogleTest）**：

- **`kvc_store`**：`//tests:kv_store_test`（[`tests/kvc_store/kv_store_test.cc`](tests/kvc_store/kv_store_test.cc)）。
- **`async_kvcstore/integration`**：`//tests:async_kvcstore_integration_test`（[`tests/async_kvcstore/integration/brpc_adapter_integration_test.cc`](tests/async_kvcstore/integration/brpc_adapter_integration_test.cc)）。
- **`async_kvcstore/integration/brpc_e2e_with_store`**：`//tests/async_kvcstore/integration/brpc_e2e_with_store:brpc_rpc_e2e_test`（[`tests/async_kvcstore/integration/brpc_e2e_with_store/brpc_rpc_e2e_test.cc`](tests/async_kvcstore/integration/brpc_e2e_with_store/brpc_rpc_e2e_test.cc)）。
- **`async_kvcstore/thread_pool`**：`//tests:async_kvcstore_thread_pool_test`（[`tests/async_kvcstore/thread_pool/async_store_thread_pool_test.cc`](tests/async_kvcstore/thread_pool/async_store_thread_pool_test.cc)）。

**Python**：目标 **`//tests:kv_store_py_test`**（[`tests/kvc_store/kv_store_py_test.py`](tests/kvc_store/kv_store_py_test.py)），`unittest` 场景与 C++ 单测对齐；`install_test_runtime()` 仅保留兼容入口（当前核心 `KvStore` 已是同步语义）。

| 场景 | 断言要点 |
|------|----------|
| 正常读写 | `set` 后 `get` 得到相同值，`last_error == KvError::OK`。 |
| 键不存在 | `get` 未命中，`last_error == KvError::KEY_NOT_FOUND`。 |
| 无运行时适配 | 直接使用核心 `KvStore` 读取不存在键：`get` 失败，`last_error == KvError::KEY_NOT_FOUND`。 |

GoogleTest 通过 **`MODULE.bazel` 中的 `http_archive`（`com_google_googletest`）** 与 [`third_party/googletest.BUILD`](third_party/googletest.BUILD) 集成（避免 BCR 包在 Bazel 9 下仍使用已移除的原生 `cc_*` 规则）；详见 [docs/build.md](docs/build.md)「测试说明」。

---

## 如何构建

在仓库根目录执行（需已安装 **Bazel**，建议 6+；本仓库在 **Bazel 9** 下验证）：

```bash
# 核心库
bazel build //src/kv:kv_core

# brpc 风格适配（测试侧，默认依赖 third_party 中的 bthread stub）
bazel build //tests:kv_brpc_test_adapter

# 测试与冒烟（C++ / Python）
bazel build //tests:kv_store_test //tests:async_kvcstore_integration_test //tests:async_kvcstore_thread_pool_test //smoke:kv_smoke
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

- **KV 语义**：核心层 `KvStore::get` / `set`（同步）与适配层 `AsyncKvStore`（executor/waiter）都已被测试覆盖。
- **单测路径**：GoogleTest 下 `//tests:kv_store_test`、`//tests:async_kvcstore_integration_test`、`//tests:async_kvcstore_thread_pool_test` 与 `//tests/async_kvcstore/integration/brpc_e2e_with_store:brpc_rpc_e2e_test` 覆盖核心层、adaptor 安装、thread pool 以及真实 brpc RPC 交互。
- **Python 绑定**：`yche_kv`（pybind11）与 C++ 单测场景一致的 **`//tests:kv_store_py_test`**，以及 **`//smoke:kv_smoke_py`**。
- **冒烟路径**：`InstallThreadPoolRuntime` + `InstallBrpcExecutorRuntime` 下，在子 bthread 中完成一次 `set`/`get` 校验（内存后端）。

---

## 其他

- Git 提交模板：根目录 [`.gitmessage`](.gitmessage)，启用：`git config commit.template .gitmessage`。
- Bazel 安装提示：[`tools/install_bazelisk.sh`](tools/install_bazelisk.sh)。
