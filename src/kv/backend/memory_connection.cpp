#include "memory_connection.h"

namespace yche::kv::internal {

MemoryConnection::MemoryConnection(std::shared_ptr<SharedMemoryStore> store) : store_(std::move(store)) {}

bool MemoryConnection::connect(const std::string&, int) { return static_cast<bool>(store_); }

bool MemoryConnection::get(const std::string& key, std::string& value, KvError* err) {
    std::lock_guard<std::mutex> g(store_->mu);
    auto it = store_->map.find(key);
    if (it == store_->map.end()) {
        *err = KvError::KEY_NOT_FOUND;
        return false;
    }
    value = it->second;
    *err = KvError::OK;
    return true;
}

bool MemoryConnection::set(const std::string& key, const std::string& value, int, KvError* err) {
    std::lock_guard<std::mutex> g(store_->mu);
    store_->map[key] = value;
    *err = KvError::OK;
    return true;
}

}  // namespace yche::kv::internal
