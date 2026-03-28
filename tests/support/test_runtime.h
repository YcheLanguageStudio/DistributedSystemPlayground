#pragma once

#include "yche/kv/store.h"

namespace yche::kv::test_support {

// Synchronous executor and waiter (design doc test adapter semantics).
void InstallTestRuntime(KvStore& store);

}  // namespace yche::kv::test_support
