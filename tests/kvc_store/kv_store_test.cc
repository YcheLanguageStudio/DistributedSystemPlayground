#include "yche/kv/error.h"
#include "yche/kv/adapters/async_store.h"
#include "yche/kv/store.h"
#include "yche/kv/types.h"

#include <gtest/gtest.h>

#include <functional>
#include <string>

using yche::kv::KvError;
using yche::kv::KvStore;
using yche::kv::KvStoreOptions;
namespace {

void InstallTestRuntime(yche::kv::adapters::AsyncKvStore& store) {
    store.set_executor([](std::function<void()> f) {
        if (f) {
            f();
        }
    });
    store.set_waiter([](std::function<void()> yield) {
        if (yield) {
            yield();
        }
    });
}

}  // namespace

TEST(KvStoreTest, SetGetOk) {
    KvStoreOptions opt;
    opt.backend = KvStoreOptions::Backend::Memory;
    opt.pool_size = 4;
    KvStore store(opt);
    yche::kv::adapters::AsyncKvStore async_store(store);
    InstallTestRuntime(async_store);

    std::string v;
    EXPECT_TRUE(async_store.set("k1", "v1", 0, 50));
    EXPECT_TRUE(async_store.get("k1", 50, v));
    EXPECT_EQ(v, "v1");
    EXPECT_EQ(async_store.last_error(), KvError::OK);
}

TEST(KvStoreTest, GetMissingKey) {
    KvStoreOptions opt;
    opt.backend = KvStoreOptions::Backend::Memory;
    KvStore store(opt);
    yche::kv::adapters::AsyncKvStore async_store(store);
    InstallTestRuntime(async_store);

    std::string v;
    EXPECT_FALSE(async_store.get("missing", 50, v));
    EXPECT_EQ(async_store.last_error(), KvError::KEY_NOT_FOUND);
}

TEST(KvStoreTest, CoreStoreGetWithoutRuntime) {
    KvStoreOptions opt;
    opt.backend = KvStoreOptions::Backend::Memory;
    KvStore store(opt);

    std::string v;
    EXPECT_FALSE(store.get("k", 50, v));
    EXPECT_EQ(store.last_error(), KvError::KEY_NOT_FOUND);
}
