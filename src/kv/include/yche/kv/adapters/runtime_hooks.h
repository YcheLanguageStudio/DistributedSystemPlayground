#pragma once

#include <functional>

namespace yche::kv {

using Executor = std::function<void(std::function<void()>)>;
using WaitCallback = std::function<void(std::function<void()>)>;

}  // namespace yche::kv
