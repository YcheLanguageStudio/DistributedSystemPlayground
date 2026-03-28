#include "yche/kv/store.h"

#include "conn_pool.h"

#include <utility>

namespace yche::kv {

struct KvStore::Impl {
    KvStoreOptions options_;
    internal::ConnPool pool_;
    Executor executor_;
    WaitCallback waiter_;
    KvError last_error_{KvError::OK};

    explicit Impl(KvStoreOptions options) : options_(std::move(options)), pool_(options_) {}
};

KvStore::KvStore(KvStoreOptions options) : impl_(std::make_unique<Impl>(std::move(options))) {}

KvStore::~KvStore() = default;

KvStore::KvStore(KvStore&&) noexcept = default;
KvStore& KvStore::operator=(KvStore&&) noexcept = default;

void KvStore::set_executor(Executor e) { impl_->executor_ = std::move(e); }

void KvStore::set_waiter(WaitCallback w) { impl_->waiter_ = std::move(w); }

bool KvStore::get(const std::string& key, int timeout_ms, std::string& value) {
    if (!impl_->executor_ || !impl_->waiter_) {
        impl_->last_error_ = KvError::UNKNOWN;
        return false;
    }
    KvError err = KvError::UNKNOWN;
    bool done = false;
    std::string local;

    impl_->executor_([&] {
        auto* c = impl_->pool_.acquire(timeout_ms);
        if (!c) {
            err = KvError::CONN_POOL_EMPTY;
            done = true;
            return;
        }
        c->get(key, local, &err);
        impl_->pool_.release(c);
        done = true;
    });

    while (!done) {
        impl_->waiter_([] {});
    }

    impl_->last_error_ = err;
    if (err == KvError::OK) {
        value = std::move(local);
        return true;
    }
    return false;
}

bool KvStore::set(const std::string& key, const std::string& val, int expire_ms, int timeout_ms) {
    if (!impl_->executor_ || !impl_->waiter_) {
        impl_->last_error_ = KvError::UNKNOWN;
        return false;
    }
    KvError err = KvError::UNKNOWN;
    bool done = false;

    impl_->executor_([&] {
        auto* c = impl_->pool_.acquire(timeout_ms);
        if (!c) {
            err = KvError::CONN_POOL_EMPTY;
            done = true;
            return;
        }
        c->set(key, val, expire_ms, &err);
        impl_->pool_.release(c);
        done = true;
    });

    while (!done) {
        impl_->waiter_([] {});
    }

    impl_->last_error_ = err;
    return err == KvError::OK;
}

KvError KvStore::last_error() const { return impl_->last_error_; }

}  // namespace yche::kv
