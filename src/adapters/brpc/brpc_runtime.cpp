#include "brpc_runtime.h"

#include "yche/kv/types.h"

#include <bthread/bthread.h>

#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace yche::kv::adapters::brpc {

namespace {

class ThreadPool {
public:
    explicit ThreadPool(size_t n) : stop_(false) {
        workers_.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            workers_.emplace_back([this] { WorkerLoop(); });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> g(mu_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& t : workers_) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    void Submit(std::function<void()> f) {
        {
            std::lock_guard<std::mutex> g(mu_);
            if (stop_) {
                return;
            }
            q_.push_back(std::move(f));
        }
        cv_.notify_one();
    }

private:
    void WorkerLoop() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lk(mu_);
                cv_.wait(lk, [&] { return stop_ || !q_.empty(); });
                if (stop_ && q_.empty()) {
                    return;
                }
                task = std::move(q_.front());
                q_.pop_front();
            }
            if (task) {
                task();
            }
        }
    }

    std::mutex mu_;
    std::condition_variable cv_;
    std::deque<std::function<void()>> q_;
    std::vector<std::thread> workers_;
    bool stop_;
};

}  // namespace

void InstallBrpcRuntime(KvStore& store, size_t worker_threads) {
    const size_t n = worker_threads == 0 ? 8 : worker_threads;
    static std::once_flag once;
    static std::unique_ptr<ThreadPool> pool_ptr;
    std::call_once(once, [&] { pool_ptr = std::make_unique<ThreadPool>(n); });

    store.set_executor([](std::function<void()> f) { pool_ptr->Submit(std::move(f)); });
    store.set_waiter([](std::function<void()>) { (void)bthread_yield(); });
}

KvStore MakeRedisStoreForBrpc(const std::string& host,
                              int port,
                              size_t conn_pool_size,
                              size_t worker_threads) {
    KvStoreOptions opt;
    opt.backend = KvStoreOptions::Backend::Redis;
    opt.redis_host = host;
    opt.redis_port = port;
    opt.pool_size = conn_pool_size == 0 ? 8 : conn_pool_size;
    KvStore store(std::move(opt));
    InstallBrpcRuntime(store, worker_threads);
    return store;
}

}  // namespace yche::kv::adapters::brpc
