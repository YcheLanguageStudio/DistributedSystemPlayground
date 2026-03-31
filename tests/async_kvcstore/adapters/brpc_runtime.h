#pragma once

#include "yche/kv/adapters/async_store.h"

#include <cstddef>

namespace yche::kv::adapters::brpc {

// Installs a pthread worker pool as Executor and std::this_thread::yield waiter.
void InstallThreadPoolRuntime(adapters::AsyncKvStore& store, size_t worker_threads = 32);

// Installs a bthread-based executor and bthread_yield waiter.
void InstallBrpcExecutorRuntime(adapters::AsyncKvStore& store);

}  // namespace yche::kv::adapters::brpc
