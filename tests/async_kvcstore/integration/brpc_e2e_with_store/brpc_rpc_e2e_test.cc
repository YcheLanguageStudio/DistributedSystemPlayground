#if __has_include(<brpc/channel.h>) && __has_include(<brpc/controller.h>) && __has_include(<brpc/server.h>)
#include <brpc/channel.h>
#include <brpc/controller.h>
#include <brpc/server.h>
#define YCHE_HAS_REAL_BRPC 1
#else
#define YCHE_HAS_REAL_BRPC 0
#endif

#include <bthread/bthread.h>
#include <gtest/gtest.h>

#include <string>

#if YCHE_HAS_REAL_BRPC && __has_include("tests/async_kvcstore/integration/brpc_e2e_with_store/kv_brpc_e2e.pb.h")
#include "async_kvcstore/adapters/brpc_runtime.h"
#include "tests/async_kvcstore/integration/brpc_e2e_with_store/kv_brpc_e2e.pb.h"
#include "yche/kv/adapters/async_store.h"
#include "yche/kv/store.h"
#include "yche/kv/types.h"
#define YCHE_HAS_E2E_PROTO 1
#elif __has_include("tests/async_kvcstore/integration/brpc_e2e_with_store/kv_brpc_e2e.pb.h")
#include "tests/async_kvcstore/integration/brpc_e2e_with_store/kv_brpc_e2e.pb.h"
#define YCHE_HAS_E2E_PROTO 1
#else
#define YCHE_HAS_E2E_PROTO 0
#endif

namespace {

#if YCHE_HAS_REAL_BRPC && YCHE_HAS_E2E_PROTO

struct TaskArg {
    yche::kv::adapters::AsyncKvStore* store{nullptr};
    std::string key;
    std::string value;
    bool ok{false};
    std::string out_value;
    std::string error;
};

void* SetAndGetOnBthread(void* p) {
    auto* arg = static_cast<TaskArg*>(p);
    std::string out;
    if (!arg->store->set(arg->key, arg->value, 0, 100)) {
        arg->error = "set failed";
        return nullptr;
    }
    if (!arg->store->get(arg->key, 100, out)) {
        arg->error = "get failed";
        return nullptr;
    }
    arg->ok = true;
    arg->out_value = std::move(out);
    return nullptr;
}

class KvServiceImpl final : public yche::kv::e2e::KvService {
public:
    KvServiceImpl() : store_(MakeStore()), async_store_(store_) { InstallRuntime(async_store_); }

    void SetAndGet(google::protobuf::RpcController* cntl_base,
                   const yche::kv::e2e::KvRequest* req,
                   yche::kv::e2e::KvResponse* resp,
                   google::protobuf::Closure* done) override {
        brpc::ClosureGuard done_guard(done);
        auto* cntl = static_cast<brpc::Controller*>(cntl_base);
        (void)cntl;

        TaskArg arg;
        arg.store = &async_store_;
        arg.key = req->key();
        arg.value = req->value();
        bthread_t tid{};
        if (bthread_start_background(&tid, nullptr, SetAndGetOnBthread, &arg) != 0) {
            resp->set_ok(false);
            resp->set_error("failed to start bthread");
            return;
        }
        void* join_ret = nullptr;
        if (bthread_join(tid, &join_ret) != 0) {
            resp->set_ok(false);
            resp->set_error("failed to join bthread");
            return;
        }
        (void)join_ret;
        resp->set_ok(arg.ok);
        resp->set_value(arg.out_value);
        if (!arg.ok) {
            resp->set_error(arg.error);
        }
    }

private:
    static yche::kv::KvStore MakeStore() {
        yche::kv::KvStoreOptions opt;
        opt.backend = yche::kv::KvStoreOptions::Backend::Memory;
        opt.pool_size = 8;
        return yche::kv::KvStore(opt);
    }

    static void InstallRuntime(yche::kv::adapters::AsyncKvStore& async_store) {
        yche::kv::adapters::brpc::InstallThreadPoolRuntime(async_store, 8);
        yche::kv::adapters::brpc::InstallBrpcExecutorRuntime(async_store);
    }

    yche::kv::KvStore store_;
    yche::kv::adapters::AsyncKvStore async_store_;
};

TEST(BrpcRpcE2ETest, ClientServerRoundTripUsesKvAdapter) {
    KvServiceImpl service;
    brpc::Server server;
    ASSERT_EQ(0, server.AddService(&service, brpc::SERVER_DOESNT_OWN_SERVICE));

    brpc::ServerOptions options;
    options.idle_timeout_sec = -1;
    ASSERT_EQ(0, server.Start("127.0.0.1:0", &options));

    brpc::Channel channel;
    brpc::ChannelOptions copt;
    ASSERT_EQ(0, channel.Init(server.listen_address(), &copt));

    yche::kv::e2e::KvService_Stub stub(&channel);
    yche::kv::e2e::KvRequest req;
    req.set_key("rpc_key");
    req.set_value("rpc_val");
    yche::kv::e2e::KvResponse resp;
    brpc::Controller cntl;
    stub.SetAndGet(&cntl, &req, &resp, nullptr);

    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
    ASSERT_TRUE(resp.ok()) << resp.error();
    ASSERT_EQ("rpc_val", resp.value());

    server.Stop(0);
    server.Join();
}

#else

TEST(BrpcRpcE2ETest, SkippedWithoutRequiredHeaders) {
    GTEST_SKIP() << "Required generated proto or real brpc headers are unavailable. "
                    "Ensure proto generation and Apache brpc override are enabled.";
}

#endif

}  // namespace
