#include "conn_pool.h"

#include "backend/memory_connection.h"
#include "backend/redis_connection.h"

#include <algorithm>
#include <chrono>

namespace yche::kv::internal {

ConnPool::ConnPool(KvStoreOptions options) : options_(std::move(options)) {
    std::shared_ptr<SharedMemoryStore> mem_store;
    if (options_.backend == KvStoreOptions::Backend::Memory) {
        mem_store = std::make_shared<SharedMemoryStore>();
    }

    conns_.resize(options_.pool_size);
    for (auto& slot : conns_) {
        if (options_.backend == KvStoreOptions::Backend::Memory) {
            slot.conn = std::make_unique<MemoryConnection>(mem_store);
        } else {
            slot.conn = std::make_unique<RedisConnection>();
        }
        if (!slot.conn->connect(options_.redis_host, options_.redis_port)) {
            slot.conn.reset();
        }
    }
}

ConnPool::~ConnPool() = default;

KvConnection* ConnPool::acquire(int timeout_ms) {
    std::unique_lock<std::mutex> lk(mu_);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(std::max(0, timeout_ms));
    auto has_idle = [this] {
        for (auto& slot : conns_) {
            if (slot.conn && !slot.in_use) {
                return true;
            }
        }
        return false;
    };
    if (timeout_ms <= 0) {
        for (auto& slot : conns_) {
            if (slot.conn && !slot.in_use) {
                slot.in_use = true;
                return slot.conn.get();
            }
        }
        return nullptr;
    }
    while (!has_idle()) {
        if (std::chrono::steady_clock::now() >= deadline) {
            return nullptr;
        }
        if (!cv_.wait_until(lk, deadline, has_idle)) {
            if (!has_idle()) {
                return nullptr;
            }
        }
    }
    for (auto& slot : conns_) {
        if (slot.conn && !slot.in_use) {
            slot.in_use = true;
            return slot.conn.get();
        }
    }
    return nullptr;
}

void ConnPool::release(KvConnection* raw) {
    if (!raw) {
        return;
    }
    std::lock_guard<std::mutex> g(mu_);
    for (auto& slot : conns_) {
        if (slot.conn.get() == raw) {
            slot.in_use = false;
            cv_.notify_one();
            return;
        }
    }
}

}  // namespace yche::kv::internal
