#pragma once

namespace yche::kv {

enum class KvError {
    OK = 0,
    TIMEOUT = 1,
    CONN_POOL_EMPTY = 2,
    KEY_NOT_FOUND = 3,
    BACKEND_ERROR = 4,
    UNKNOWN = 99,
};

}  // namespace yche::kv
