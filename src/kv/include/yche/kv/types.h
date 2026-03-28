#pragma once

#include <cstddef>
#include <string>

namespace yche::kv {

struct KvStoreOptions {
    enum class Backend { Memory, Redis };

    Backend backend = Backend::Memory;
    std::string redis_host = "127.0.0.1";
    int redis_port = 6379;
    size_t pool_size = 8;
};

}  // namespace yche::kv
