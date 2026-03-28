#pragma once

#include "kv_connection.h"

#include <hiredis.h>

#include <string>

namespace yche::kv::internal {

class RedisConnection final : public KvConnection {
public:
    RedisConnection();
    ~RedisConnection() override;

    RedisConnection(const RedisConnection&) = delete;
    RedisConnection& operator=(const RedisConnection&) = delete;

    bool connect(const std::string& host, int port) override;
    bool get(const std::string& key, std::string& value, KvError* err) override;
    bool set(const std::string& key, const std::string& value, int expire_ms, KvError* err) override;

private:
    redisContext* ctx_{nullptr};
};

}  // namespace yche::kv::internal
