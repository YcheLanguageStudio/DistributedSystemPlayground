#include "test_runtime.h"

namespace yche::kv::test_support {

void InstallTestRuntime(KvStore& store) {
    store.set_executor([](std::function<void()> f) {
        if (f) {
            f();
        }
    });
    store.set_waiter([](std::function<void()> yield) {
        if (yield) {
            yield();
        }
    });
}

}  // namespace yche::kv::test_support
