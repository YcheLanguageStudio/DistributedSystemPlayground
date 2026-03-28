#include "brpc_runtime.h"

#include "yche/kv/store.h"
#include "yche/kv/types.h"

#include <bthread/bthread.h>

#include <cstdlib>
#include <string>

namespace {

struct RunArg {
    yche::kv::KvStore* store;
    int exit_code{0};
};

void* RunSmoke(void* p) {
    auto* a = static_cast<RunArg*>(p);
    yche::kv::KvStore& st = *a->store;
    std::string v;
    if (!st.set("smoke_key", "smoke_val", 0, 100)) {
        a->exit_code = 2;
        return nullptr;
    }
    if (!st.get("smoke_key", 100, v)) {
        a->exit_code = 3;
        return nullptr;
    }
    if (v != "smoke_val") {
        a->exit_code = 4;
        return nullptr;
    }
    a->exit_code = 0;
    return nullptr;
}

}  // namespace

int main() {
    yche::kv::KvStoreOptions opt;
    opt.backend = yche::kv::KvStoreOptions::Backend::Memory;
    opt.pool_size = 4;
    yche::kv::KvStore store(opt);
    yche::kv::adapters::brpc::InstallBrpcRuntime(store);

    RunArg arg;
    arg.store = &store;
    bthread_t tid{};
    if (bthread_start_background(&tid, nullptr, RunSmoke, &arg) != 0) {
        return 10;
    }
    void* join_ret = nullptr;
    if (bthread_join(tid, &join_ret) != 0) {
        return 11;
    }
    (void)join_ret;
    return arg.exit_code;
}
