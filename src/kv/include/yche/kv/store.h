#pragma once

#include "yche/kv/adapters/runtime_hooks.h"
#include "yche/kv/error.h"
#include "yche/kv/types.h"

#include <memory>
#include <string>

namespace yche::kv {

class KvStore {
public:
    explicit KvStore(KvStoreOptions options = {});
    ~KvStore();

    KvStore(const KvStore&) = delete;
    KvStore& operator=(const KvStore&) = delete;
    KvStore(KvStore&&) noexcept;
    KvStore& operator=(KvStore&&) noexcept;

    void set_executor(Executor e);
    void set_waiter(WaitCallback w);

    bool get(const std::string& key, int timeout_ms, std::string& value);
    bool set(const std::string& key, const std::string& value, int expire_ms, int timeout_ms);

    KvError last_error() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace yche::kv
