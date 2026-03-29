#include "yche/kv/error.h"
#include "yche/kv/store.h"
#include "yche/kv/types.h"

#include "support/test_runtime.h"

#include <gtest/gtest.h>

#include <string>

using yche::kv::KvError;
using yche::kv::KvStore;
using yche::kv::KvStoreOptions;
using yche::kv::test_support::InstallTestRuntime;

TEST(KvStoreTest, SetGetOk) {
    KvStoreOptions opt;
    opt.backend = KvStoreOptions::Backend::Memory;
    opt.pool_size = 4;
    KvStore store(opt);
    InstallTestRuntime(store);

    std::string v;
    EXPECT_TRUE(store.set("k1", "v1", 0, 50));
    EXPECT_TRUE(store.get("k1", 50, v));
    EXPECT_EQ(v, "v1");
    EXPECT_EQ(store.last_error(), KvError::OK);
}

TEST(KvStoreTest, GetMissingKey) {
    KvStoreOptions opt;
    opt.backend = KvStoreOptions::Backend::Memory;
    KvStore store(opt);
    InstallTestRuntime(store);

    std::string v;
    EXPECT_FALSE(store.get("missing", 50, v));
    EXPECT_EQ(store.last_error(), KvError::KEY_NOT_FOUND);
}

TEST(KvStoreTest, GetWithoutRuntimeYieldsUnknown) {
    KvStoreOptions opt;
    opt.backend = KvStoreOptions::Backend::Memory;
    KvStore store(opt);

    std::string v;
    EXPECT_FALSE(store.get("k", 50, v));
    EXPECT_EQ(store.last_error(), KvError::UNKNOWN);
}
