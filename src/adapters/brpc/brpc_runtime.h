#pragma once

#include "yche/kv/store.h"

#include <cstddef>
#include <string>

namespace yche::kv::adapters::brpc {

// Installs a pthread worker pool as Executor and bthread_yield-based WaitCallback.
void InstallBrpcRuntime(KvStore& store, size_t worker_threads = 32);

// Redis backend + runtime hooks (returns a store ready for use from bthreads).
KvStore MakeRedisStoreForBrpc(const std::string& host,
                              int port,
                              size_t conn_pool_size = 32,
                              size_t worker_threads = 32);

}  // namespace yche::kv::adapters::brpc
