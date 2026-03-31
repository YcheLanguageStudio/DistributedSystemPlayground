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

使用 **`rules_python`** 的 **`py_wheel`**（[`bindings/python/BUILD.bazel`](../bindings/python/BUILD.bazel) 中目标 **`yche_kv_whl`**）打 **二进制** 包。推荐同时构建 **`yche_kv_whl_stable`**：在固定相对路径下复制一份 **`dist/yche_kv.whl`**，避免只在日志里找带版本号的文件名。

```bash
bazel build //bindings/python:yche_kv_whl_stable
# 等价于先构建 yche_kv_whl，再生成稳定路径；也可只构建 //bindings/python:yche_kv_whl
```

**产物路径（本仓库 `symlink_prefix`）**

| 构建方式 | 稳定 wheel | 带版本号的 wheel（日志里常见） |
|----------|------------|--------------------------------|
| 默认（未加 `--config=compile-commands`） | **`build/bin/bindings/python/dist/yche_kv.whl`** | **`build/bin/bindings/python/yche_kv-0.1.0-cp311-cp311-….whl`** |
| 带 **`--config=compile-commands`** 的同一次输出布局 | **`build/bazel-bin/bindings/python/dist/yche_kv.whl`** | **`build/bazel-bin/bindings/python/yche_kv-….whl`** |

若只看了 **`build/bin`**，而最近一次构建用了 **`compile-commands`** 配置，请到 **`build/bazel-bin`** 下找同名路径（或始终构建 **`yche_kv_whl_stable`** 后看 Bazel 打印的 **`Target ... up-to-date:`** 行）。

当前默认配置为 **CPython 3.11** 的 **`cp311`** 标签；**Linux x86_64** 为 **`manylinux2014_x86_64`**，**Linux aarch64** 为 **`manylinux2014_aarch64`**，**macOS** 为 **`macosx_11_0_arm64`**。与当前 **Python 工具链版本** 不一致时需调整 `python_tag` / `abi` / `platform`。

安装示例：`pip install --force-reinstall build/bin/bindings/python/dist/yche_kv.whl`（若使用 **`bazel-bin`** 前缀请把 **`build/bin`** 换成 **`build/bazel-bin`**）。

根目录 [`build.sh`](../build.sh) 的 **`-p` / `--python`** 与 **`-w`** 会构建 **`//bindings/python:yche_kv_whl_stable`**。

## API 概要

- **`KvStoreOptions`**：`backend`（`Backend.Memory` / `Backend.Redis`）、`redis_host`、`redis_port`、`pool_size`。
- **`KvStore`**：`install_test_runtime()`（兼容旧接口，当前实现为 no-op）、`set(key, value, expire_ms=0, timeout_ms)`、`get(key, timeout_ms)` → `(ok: bool, value: str)`、`last_error()` → `KvError`。
- 当前 `KvStore` 为同步核心层：`get`/`set` 可直接使用；读取不存在键时 `last_error` 为 **`KEY_NOT_FOUND`**。

## 测试与冒烟

| Bazel 目标 | 说明 |
|------------|------|
| **`//tests:kv_store_py_test`** | [`unittest`](https://docs.python.org/3/library/unittest.html)，场景对齐 **`//tests:kv_store_test`**。 |
| **`//smoke:kv_smoke_py`** | Python 版快速 **`set`/`get`**，进程退出码 **0** 表示通过。 |

```bash
bazel test //tests:kv_store_py_test
bazel run //smoke:kv_smoke_py
```
