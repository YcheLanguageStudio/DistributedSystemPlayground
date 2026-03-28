#pragma once

#include "yche/kv/error.h"

#include <string>

namespace yche::kv::internal {

class KvConnection {
public:
    virtual ~KvConnection() = default;

    virtual bool connect(const std::string& host, int port) = 0;
    virtual bool get(const std::string& key, std::string& value, KvError* err) = 0;
    virtual bool set(const std::string& key, const std::string& value, int expire_ms, KvError* err) = 0;
};

}  // namespace yche::kv::internal
