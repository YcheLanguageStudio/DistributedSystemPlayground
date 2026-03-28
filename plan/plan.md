# Bazel C++17 KV 工程布局（src / tests / smoke + 对外 include + pybind 预留）

## 1. 设计目标

- **src / tests / smoke 三分**：源码与对外头文件在 **`src/`**；**`tests/`** 仅验证与测试桩；**`smoke/`** 仅短路径集成冒烟。
- **KV 实现**：核心逻辑自研（连接池、超时、错误码、Executor/WaitCallback 注入，对齐 `docs/design_kv.md`）；**Redis 访问** 使用 **hiredis**；**内存后端** 供无 Redis 的单测与可选冒烟。
- **对外 include**：**`src/kv/include/yche/kv/`**（公共头文件已置于 **`src/kv/include/`**，与 `glob`/`rules_cc` 兼容）；协程侧仅暴露 **`runtime_hooks.h`**（无 brpc 头文件）。
- **薄适配层**：**`src/adapters/brpc/`**，独立 `cc_library`，pthread 线程池 + `bthread_yield`。
- **构建**：Bazel，**C++17**（`.bazelrc`）。
- **后续 pybind**：**`bindings/python/`** 占位，仅依赖 **`//src/kv:kv_core`**。

## 2. 目录树（与仓库一致）

```text
src/kv/include/yche/kv/       # error.h, types.h, store.h, adapters/runtime_hooks.h
src/kv/                       # BUILD, store.cpp, conn_pool.*, backend/*
src/adapters/brpc/            # BUILD, brpc_runtime.*
tests/                        # BUILD, support/, kv_store_test.cc
smoke/                        # BUILD, kv_smoke.cc
bindings/python/              # （可选）后续 py_extension
third_party/brpc_stub_module/ # 默认可构建的 bthread stub；可换真实 Apache brpc
```

## 3. brpc 与 Bazel 9

- 根 **`MODULE.bazel`** 使用 **`local_path_override(module_name = "brpc", path = "third_party/brpc_stub_module")`**。
- **Bazel 9** 需在各 `BUILD` 中 **`load("@rules_cc//cc:defs.bzl", ...)`**，不得使用已移除的原生 `cc_*`。
- 构建与运行说明见 **[docs/build.md](../docs/build.md)**。

## 4. 验收

- `bazel test //tests:...`（内存后端）
- `bazel run //smoke:kv_smoke`

## 5. 与 design_kv.md 的映射

| 文档概念 | 落地位置 |
|---------|----------|
| SafeCache / get / set / 注入 | `KvStore` + `runtime_hooks.h` + `src/kv/store.cpp` |
| ConnPool + Redis | `src/kv/conn_pool.*` + `backend/redis_connection.cpp` |
| brpc 适配 | `src/adapters/brpc/brpc_runtime.*` |
| 单测适配 | `tests/support/test_runtime.*` |
