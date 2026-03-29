# Python 绑定（pybind11）

[`bindings/python/`](../bindings/python/) 将 C++ [`KvStore`](../src/kv/include/yche/kv/store.h) 暴露为 Python 包 **`yche_kv`**（扩展模块 **`_yche_kv`**，由 [pybind11_bazel](https://registry.bazel.build/modules/pybind11_bazel) 构建）。

## 依赖

- 根目录 [`MODULE.bazel`](../MODULE.bazel) 中的 **`pybind11_bazel`** 与 **`rules_python`**（Python 工具链）。
- 绑定仅链接 **`//src/kv:kv_core`**，不依赖 brpc。

## 构建

```bash
bazel build //bindings/python:yche_kv
# 或只构建 .so
bazel build //bindings/python:_yche_kv
```

### Wheel（`.whl`）

使用 **`rules_python`** 的 **`py_wheel`**（[`bindings/python/BUILD.bazel`](../bindings/python/BUILD.bazel) 中目标 **`yche_kv_whl`**）打 **二进制** 包：

```bash
bazel build //bindings/python:yche_kv_whl
```

产物路径见 Bazel 输出（本仓库默认 **`--symlink_prefix=build/`**，例如 **`build/bin/bindings/python/yche_kv-0.1.0-cp311-cp311-manylinux2014_x86_64.whl`**）。当前默认配置为 **CPython 3.11** 的 **`cp311`** 标签与 **`manylinux2014_x86_64`**（Linux）或 **`macosx_11_0_arm64`**（macOS）；与当前 **Python 工具链版本** 不一致时需调整 `python_tag` / `abi` / `platform`。

安装示例：`pip install --force-reinstall build/bin/bindings/python/yche_kv-*.whl`（路径以实际构建输出为准）。

根目录 [`build.sh`](../build.sh) 的 **`-p` / `--python`** 会同时构建 **`yche_kv`** 与 **`yche_kv_whl`**。

## API 概要

- **`KvStoreOptions`**：`backend`（`Backend.Memory` / `Backend.Redis`）、`redis_host`、`redis_port`、`pool_size`。
- **`KvStore`**：`install_test_runtime()`（与 C++ 单测相同的同步 Executor/WaitCallback）、`set(key, value, expire_ms=0, timeout_ms)`、`get(key, timeout_ms)` → `(ok: bool, value: str)`、`last_error()` → `KvError`。
- 与 C++ 一样，**必须先** `install_test_runtime()`（或将来接入其它运行时适配），否则 `get`/`set` 会失败且 `last_error` 多为 **`UNKNOWN`**。

## 测试与冒烟

| Bazel 目标 | 说明 |
|------------|------|
| **`//tests:kv_store_py_test`** | [`unittest`](https://docs.python.org/3/library/unittest.html)，场景对齐 **`//tests:kv_store_test`**。 |
| **`//smoke:kv_smoke_py`** | Python 版快速 **`set`/`get`**，进程退出码 **0** 表示通过。 |

```bash
bazel test //tests:kv_store_py_test
bazel run //smoke:kv_smoke_py
```
