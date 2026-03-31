"""Python UT for yche_kv (mirrors tests/kvc_store/kv_store_test.cc scenarios)."""

import unittest

from yche_kv import Backend, KvError, KvStore, KvStoreOptions


class TestKvStore(unittest.TestCase):
    def test_set_get_ok(self) -> None:
        opt = KvStoreOptions()
        opt.backend = Backend.Memory
        opt.pool_size = 4
        store = KvStore(opt)
        store.install_test_runtime()
        self.assertTrue(store.set("k1", "v1", expire_ms=0, timeout_ms=50))
        ok, val = store.get("k1", timeout_ms=50)
        self.assertTrue(ok)
        self.assertEqual(val, "v1")
        self.assertEqual(store.last_error(), KvError.OK)

    def test_get_missing(self) -> None:
        opt = KvStoreOptions()
        opt.backend = Backend.Memory
        store = KvStore(opt)
        store.install_test_runtime()
        ok, _val = store.get("missing", timeout_ms=50)
        self.assertFalse(ok)
        self.assertEqual(store.last_error(), KvError.KEY_NOT_FOUND)

    def test_no_runtime(self) -> None:
        opt = KvStoreOptions()
        opt.backend = Backend.Memory
        store = KvStore(opt)
        ok, _val = store.get("k", timeout_ms=50)
        self.assertFalse(ok)
        self.assertEqual(store.last_error(), KvError.KEY_NOT_FOUND)


if __name__ == "__main__":
    unittest.main()
