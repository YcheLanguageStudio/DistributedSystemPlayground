#include "async_kvcstore/adapters/brpc_runtime.h"

#if __has_include(<bthread/bthread.h>)
#include <bthread/bthread.h>
#else
using bthread_t = unsigned long;
inline int bthread_start_background(bthread_t*, const void*, void* (*fn)(void*), void* arg) {
    (void)fn;
    (void)arg;
    return -1;
}
inline int bthread_yield() { return 0; }
#endif

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

void InstallThreadPoolRuntime(adapters::AsyncKvStore& store, size_t worker_threads) {
    const size_t n = worker_threads == 0 ? 8 : worker_threads;
    static std::once_flag once;
    static std::unique_ptr<ThreadPool> pool_ptr;
    std::call_once(once, [&] { pool_ptr = std::make_unique<ThreadPool>(n); });

    store.set_executor([](std::function<void()> f) { pool_ptr->Submit(std::move(f)); });
    store.set_waiter([](std::function<void()>) { std::this_thread::yield(); });
}

namespace {

void* RunBthreadTask(void* arg) {
    auto task = static_cast<std::function<void()>*>(arg);
    if (task && *task) {
        (*task)();
    }
    delete task;
    return nullptr;
}

}  // namespace

void InstallBrpcExecutorRuntime(adapters::AsyncKvStore& store) {
    store.set_executor([](std::function<void()> f) {
        auto* task = new std::function<void()>(std::move(f));
        bthread_t tid{};
        if (bthread_start_background(&tid, nullptr, RunBthreadTask, task) == 0) {
            return;
        }
        // Fallback for unexpected bthread start failures.
        (*task)();
        delete task;
    });
    store.set_waiter([](std::function<void()>) { (void)bthread_yield(); });
}

}  // namespace yche::kv::adapters::brpc
