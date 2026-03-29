#include "brpc_runtime.h"

#include "yche/kv/store.h"
#include "yche/kv/types.h"

#include <bthread/bthread.h>
#include <gtest/gtest.h>

#include <string>

namespace {

struct RunArg {
    yche::kv::KvStore* store{nullptr};
    bool ok{false};
    std::string error;
};

void* RunGetSet(void* p) {
    auto* a = static_cast<RunArg*>(p);
    yche::kv::KvStore& st = *a->store;
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

TEST(BrpcAdapterTest, BthreadGetSetWithMemoryBackend) {
    yche::kv::KvStoreOptions opt;
    opt.backend = yche::kv::KvStoreOptions::Backend::Memory;
    opt.pool_size = 4;
    yche::kv::KvStore store(opt);
    yche::kv::adapters::brpc::InstallBrpcRuntime(store, 4);

    RunArg arg;
    arg.store = &store;
    bthread_t tid{};
    ASSERT_EQ(bthread_start_background(&tid, nullptr, RunGetSet, &arg), 0);
    void* join_ret = nullptr;
    ASSERT_EQ(bthread_join(tid, &join_ret), 0);
    (void)join_ret;
    EXPECT_TRUE(arg.ok) << arg.error;
}
