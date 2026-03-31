#include "yche/kv/adapters/async_store.h"

#include <utility>

namespace yche::kv::adapters {

AsyncKvStore::AsyncKvStore(KvStore& store) : store_(&store) {}

void AsyncKvStore::set_executor(Executor e) { executor_ = std::move(e); }

void AsyncKvStore::set_waiter(WaitCallback w) { waiter_ = std::move(w); }

bool AsyncKvStore::get(const std::string& key, int timeout_ms, std::string& value) {
    if (!executor_ || !waiter_) {
        last_error_ = KvError::UNKNOWN;
        return false;
    }
    KvError err = KvError::UNKNOWN;
    bool done = false;
    std::string local;

    executor_([&] {
        if (!store_->get(key, timeout_ms, local)) {
            err = store_->last_error();
            done = true;
            return;
        }
        err = KvError::OK;
        done = true;
    });

    while (!done) {
        waiter_([] {});
    }

    last_error_ = err;
    if (err == KvError::OK) {
        value = std::move(local);
        return true;
    }
    return false;
}

bool AsyncKvStore::set(const std::string& key, const std::string& value, int expire_ms, int timeout_ms) {
    if (!executor_ || !waiter_) {
        last_error_ = KvError::UNKNOWN;
        return false;
    }
    KvError err = KvError::UNKNOWN;
    bool done = false;

    executor_([&] {
        if (!store_->set(key, value, expire_ms, timeout_ms)) {
            err = store_->last_error();
            done = true;
            return;
        }
        err = KvError::OK;
        done = true;
    });

    while (!done) {
        waiter_([] {});
    }

    last_error_ = err;
    return err == KvError::OK;
}

KvError AsyncKvStore::last_error() const { return last_error_; }

}  // namespace yche::kv::adapters
