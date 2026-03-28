#pragma once

#include "backend/kv_connection.h"

#include "yche/kv/types.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

namespace yche::kv::internal {

struct PooledConn {
    std::unique_ptr<KvConnection> conn;
    bool in_use{false};
};

class ConnPool {
public:
    explicit ConnPool(KvStoreOptions options);
    ~ConnPool();

    ConnPool(const ConnPool&) = delete;
    ConnPool& operator=(const ConnPool&) = delete;

    KvConnection* acquire(int timeout_ms);
    void release(KvConnection* raw);

private:
    KvStoreOptions options_;
    std::vector<PooledConn> conns_;
    std::mutex mu_;
    std::condition_variable cv_;
};

}  // namespace yche::kv::internal
