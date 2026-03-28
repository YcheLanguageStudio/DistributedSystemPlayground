#include "yche/kv/error.h"
#include "yche/kv/store.h"
#include "yche/kv/types.h"

#include "support/test_runtime.h"

#include <cassert>
#include <cstdlib>
#include <string>

using yche::kv::KvError;
using yche::kv::KvStore;
using yche::kv::KvStoreOptions;
using yche::kv::test_support::InstallTestRuntime;

int main() {
    {
        KvStoreOptions opt;
        opt.backend = KvStoreOptions::Backend::Memory;
        opt.pool_size = 4;
        KvStore store(opt);
        InstallTestRuntime(store);

        std::string v;
        assert(store.set("k1", "v1", 0, 50));
        assert(store.get("k1", 50, v));
        assert(v == "v1");
        assert(store.last_error() == KvError::OK);
    }
    {
        KvStoreOptions opt;
        opt.backend = KvStoreOptions::Backend::Memory;
        KvStore store(opt);
        InstallTestRuntime(store);

        std::string v;
        assert(!store.get("missing", 50, v));
        assert(store.last_error() == KvError::KEY_NOT_FOUND);
    }
    {
        KvStoreOptions opt;
        opt.backend = KvStoreOptions::Backend::Memory;
        KvStore store(opt);

        std::string v;
        assert(!store.get("k", 50, v));
        assert(store.last_error() == KvError::UNKNOWN);
    }
    return EXIT_SUCCESS;
}
