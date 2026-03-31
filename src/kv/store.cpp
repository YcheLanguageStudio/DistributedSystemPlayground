#include "yche/kv/store.h"

#include "conn_pool.h"

#include <utility>

namespace yche::kv {

struct KvStore::Impl {
    KvStoreOptions options_;
    internal::ConnPool pool_;
    KvError last_error_{KvError::OK};

    explicit Impl(KvStoreOptions options) : options_(std::move(options)), pool_(options_) {}
};

KvStore::KvStore(KvStoreOptions options) : impl_(std::make_unique<Impl>(std::move(options))) {}

KvStore::~KvStore() = default;

KvStore::KvStore(KvStore&&) noexcept = default;
KvStore& KvStore::operator=(KvStore&&) noexcept = default;

bool KvStore::get(const std::string& key, int timeout_ms, std::string& value) {
    KvError err = KvError::UNKNOWN;
    auto* c = impl_->pool_.acquire(timeout_ms);
    if (!c) {
        impl_->last_error_ = KvError::CONN_POOL_EMPTY;
        return false;
    }
    c->get(key, value, &err);
    impl_->pool_.release(c);
    impl_->last_error_ = err;
    return err == KvError::OK;
}

bool KvStore::set(const std::string& key, const std::string& val, int expire_ms, int timeout_ms) {
    KvError err = KvError::UNKNOWN;
    auto* c = impl_->pool_.acquire(timeout_ms);
    if (!c) {
        impl_->last_error_ = KvError::CONN_POOL_EMPTY;
        return false;
    }
    c->set(key, val, expire_ms, &err);
    impl_->pool_.release(c);
    impl_->last_error_ = err;
    return err == KvError::OK;
}

KvError KvStore::last_error() const { return impl_->last_error_; }

}  // namespace yche::kv
