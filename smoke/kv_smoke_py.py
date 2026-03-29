#!/usr/bin/env python3
"""Python smoke: memory backend + test runtime, one set/get (like C++ kv_smoke)."""

import sys

from yche_kv import Backend, KvStore, KvStoreOptions


def main() -> int:
    opt = KvStoreOptions()
    opt.backend = Backend.Memory
    opt.pool_size = 4
    store = KvStore(opt)
    store.install_test_runtime()
    if not store.set("smoke", "ok", expire_ms=0, timeout_ms=500):
        return 1
    ok, val = store.get("smoke", timeout_ms=500)
    if not ok or val != "ok":
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
