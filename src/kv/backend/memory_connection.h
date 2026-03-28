#pragma once

#include "kv_connection.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace yche::kv::internal {

struct SharedMemoryStore {
    std::mutex mu;
    std::unordered_map<std::string, std::string> map;
};

class MemoryConnection final : public KvConnection {
public:
    explicit MemoryConnection(std::shared_ptr<SharedMemoryStore> store);

    bool connect(const std::string& host, int port) override;
    bool get(const std::string& key, std::string& value, KvError* err) override;
    bool set(const std::string& key, const std::string& value, int expire_ms, KvError* err) override;

private:
    std::shared_ptr<SharedMemoryStore> store_;
};

}  // namespace yche::kv::internal
