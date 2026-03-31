#pragma once

#include "yche/kv/adapters/runtime_hooks.h"
#include "yche/kv/error.h"
#include "yche/kv/store.h"

#include <string>

namespace yche::kv::adapters {

class AsyncKvStore {
public:
    explicit AsyncKvStore(KvStore& store);

    void set_executor(Executor e);
    void set_waiter(WaitCallback w);

    bool get(const std::string& key, int timeout_ms, std::string& value);
    bool set(const std::string& key, const std::string& value, int expire_ms, int timeout_ms);

    KvError last_error() const;

private:
    KvStore* store_;
    Executor executor_;
    WaitCallback waiter_;
    KvError last_error_{KvError::OK};
};

}  // namespace yche::kv::adapters
