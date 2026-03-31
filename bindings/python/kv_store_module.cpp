#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <string>

#include "yche/kv/error.h"
#include "yche/kv/store.h"
#include "yche/kv/types.h"

namespace py = pybind11;
using yche::kv::KvError;
using yche::kv::KvStore;
using yche::kv::KvStoreOptions;

namespace {

void InstallTestRuntime(KvStore& /*store*/) {
    // Kept for API compatibility: core KvStore is synchronous now.
}

}  // namespace

PYBIND11_MODULE(_yche_kv, m) {
    m.doc() = "yche KV store (pybind11)";

    py::enum_<KvError>(m, "KvError")
        .value("OK", KvError::OK)
        .value("TIMEOUT", KvError::TIMEOUT)
        .value("CONN_POOL_EMPTY", KvError::CONN_POOL_EMPTY)
        .value("KEY_NOT_FOUND", KvError::KEY_NOT_FOUND)
        .value("BACKEND_ERROR", KvError::BACKEND_ERROR)
        .value("UNKNOWN", KvError::UNKNOWN)
        .export_values();

    py::enum_<KvStoreOptions::Backend>(m, "Backend")
        .value("Memory", KvStoreOptions::Backend::Memory)
        .value("Redis", KvStoreOptions::Backend::Redis)
        .export_values();

    py::class_<KvStoreOptions>(m, "KvStoreOptions")
        .def(py::init<>())
        .def_readwrite("backend", &KvStoreOptions::backend)
        .def_readwrite("redis_host", &KvStoreOptions::redis_host)
        .def_readwrite("redis_port", &KvStoreOptions::redis_port)
        .def_readwrite("pool_size", &KvStoreOptions::pool_size);

    py::class_<KvStore>(m, "KvStore")
        .def(py::init([](const KvStoreOptions& opt) { return std::make_unique<KvStore>(opt); }))
        .def("install_test_runtime", [](KvStore& s) { InstallTestRuntime(s); })
        .def(
            "set",
            [](KvStore& self, const std::string& key, const std::string& value, int expire_ms,
               int timeout_ms) { return self.set(key, value, expire_ms, timeout_ms); },
            py::arg("key"), py::arg("value"), py::arg("expire_ms") = 0, py::arg("timeout_ms"))
        .def(
            "get",
            [](KvStore& self, const std::string& key, int timeout_ms) {
                std::string value;
                const bool ok = self.get(key, timeout_ms, value);
                return py::make_tuple(ok, value);
            },
            py::arg("key"), py::arg("timeout_ms"))
        .def("last_error", &KvStore::last_error);
}
