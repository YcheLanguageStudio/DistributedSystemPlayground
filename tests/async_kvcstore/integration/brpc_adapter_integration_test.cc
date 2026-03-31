#include "async_kvcstore/adapters/brpc_runtime.h"

#include "yche/kv/adapters/async_store.h"
#include "yche/kv/types.h"

#include <bthread/bthread.h>
#include <gtest/gtest.h>

#include <string>

namespace {

struct RunArg {
    yche::kv::adapters::AsyncKvStore* store{nullptr};
    bool ok{false};
    std::string error;
};

void* RunGetSet(void* p) {
    auto* a = static_cast<RunArg*>(p);
    yche::kv::adapters::AsyncKvStore& st = *a->store;
    std::string v;
    if (!st.set("t_key", "t_val", 0, 100)) {
        a->error = "set failed";
        return nullptr;
    }
    if (!st.get("t_key", 100, v)) {
        a->error = "get failed";
        return nullptr;
    }
    if (v != "t_val") {
        a->error = "value mismatch";
        return nullptr;
    }
    a->ok = true;
    return nullptr;
}

}  // namespace

TEST(BrpcAdapterIntegrationTest, BthreadGetSetWithMemoryBackend) {
    yche::kv::KvStoreOptions opt;
    opt.backend = yche::kv::KvStoreOptions::Backend::Memory;
    opt.pool_size = 4;
    yche::kv::KvStore store(opt);
    yche::kv::adapters::AsyncKvStore async_store(store);
    yche::kv::adapters::brpc::InstallThreadPoolRuntime(async_store, 4);
    yche::kv::adapters::brpc::InstallBrpcExecutorRuntime(async_store);

    RunArg arg;
    arg.store = &async_store;
    bthread_t tid{};
    ASSERT_EQ(bthread_start_background(&tid, nullptr, RunGetSet, &arg), 0);
    void* join_ret = nullptr;
    ASSERT_EQ(bthread_join(tid, &join_ret), 0);
    (void)join_ret;
    EXPECT_TRUE(arg.ok) << arg.error;
}
