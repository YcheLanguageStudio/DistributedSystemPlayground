# GoogleTest / GoogleMock (v1.17.0 tarball) built with rules_cc for Bazel 9+.
load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

cc_library(
    name = "gtest",
    srcs = glob(
        include = [
            "googletest/src/*.cc",
            "googletest/src/*.h",
            "googletest/include/gtest/**/*.h",
            "googlemock/src/*.cc",
            "googlemock/include/gmock/**/*.h",
        ],
        exclude = [
            "googletest/src/gtest-all.cc",
            "googletest/src/gtest_main.cc",
            "googlemock/src/gmock-all.cc",
            "googlemock/src/gmock_main.cc",
        ],
    ),
    hdrs = glob([
        "googletest/include/gtest/*.h",
        "googlemock/include/gmock/*.h",
    ]),
    copts = select({
        "@platforms//os:windows": [],
        "//conditions:default": ["-pthread"],
    }),
    includes = [
        "googlemock",
        "googlemock/include",
        "googletest",
        "googletest/include",
    ],
    linkopts = select({
        "@platforms//os:windows": [],
        "//conditions:default": ["-pthread"],
    }),
)

cc_library(
    name = "gtest_main",
    srcs = ["googlemock/src/gmock_main.cc"],
    deps = [":gtest"],
)
