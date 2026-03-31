#include "async_kvcstore/adapters/brpc_runtime.h"

#include "yche/kv/adapters/async_store.h"
#include "yche/kv/store.h"
#include "yche/kv/types.h"

#include <gtest/gtest.h>

#include <string>

TEST(AsyncKvStoreThreadPoolTest, ThreadPoolRuntimeSetGet) {
    yche::kv::KvStoreOptions opt;
    opt.backend = yche::kv::KvStoreOptions::Backend::Memory;
    opt.pool_size = 4;

    yche::kv::KvStore core_store(opt);
    yche::kv::adapters::AsyncKvStore async_store(core_store);
    yche::kv::adapters::brpc::InstallThreadPoolRuntime(async_store, 4);

    std::string value;
    EXPECT_TRUE(async_store.set("tp_key", "tp_val", 0, 100));
    EXPECT_TRUE(async_store.get("tp_key", 100, value));
    EXPECT_EQ("tp_val", value);
}
