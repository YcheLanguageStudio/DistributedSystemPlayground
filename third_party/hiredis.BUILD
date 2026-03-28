load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "hiredis",
    srcs = [
        "alloc.c",
        "async.c",
        "hiredis.c",
        "net.c",
        "read.c",
        "sds.c",
        "sockcompat.c",
    ],
    hdrs = glob(["*.h"]),
    # async.c does `#include "dict.c"` (implementation pulled into one TU).
    textual_hdrs = ["dict.c"],
    copts = ["-std=c99"],
    includes = ["."],
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
)
