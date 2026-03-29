#!/usr/bin/env bash
# Convenience wrapper for common Bazel build / test / smoke commands.
# Run from repository root: ./build.sh [options]
# Override Bazel binary: BAZEL=/path/to/bazelisk ./build.sh ...
set -euo pipefail

BAZEL="${BAZEL:-bazel}"

DEFAULT_TARGETS=(
  "//src/kv:kv_core"
  "//src/adapters/brpc:kv_brpc_adapter"
)

WITH_PYTHON=0
WITH_WHEEL=0
WITH_BUILD_TESTS=0
RUN_TESTS=0
RUN_SMOKE=0
DO_ALL=0

usage() {
  cat <<'EOF'
Usage: ./build.sh [options]

  Default: bazel build //src/kv:kv_core //src/adapters/brpc:kv_brpc_adapter

Options:
  -h, --help          Show this help
  -p, --python        Also build //bindings/python:yche_kv and //bindings/python:yche_kv_whl_stable (.whl)
  -w, --wheel, --whl Also build //bindings/python:yche_kv_whl_stable (.whl for pip install)
  -b, --build-tests   Also build C++/Python test and smoke binaries (does not run them)
  -t, --test          Run bazel test //tests/...
  -s, --smoke         Run //smoke:kv_smoke and //smoke:kv_smoke_py
  -a, --all           Build kv + brpc + python + all test/smoke targets, then run tests and smoke

Environment:
  BAZEL   Bazel executable (default: bazel)

Examples:
  ./build.sh
  ./build.sh -w              # default targets + .whl (transitively builds extension)
  ./build.sh -p              # Python package + .whl (same as before)
  ./build.sh -p -b
  ./build.sh -t
  ./build.sh -p -t -s
  ./build.sh --all
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h | --help)
      usage
      exit 0
      ;;
    -p | --python)
      WITH_PYTHON=1
      ;;
    -w | --wheel | --whl)
      WITH_WHEEL=1
      ;;
    -b | --build-tests)
      WITH_BUILD_TESTS=1
      ;;
    -t | --test)
      RUN_TESTS=1
      ;;
    -s | --smoke)
      RUN_SMOKE=1
      ;;
    -a | --all)
      DO_ALL=1
      ;;
    *)
      echo "Unknown option: $1" >&2
      echo "Try: $0 --help" >&2
      exit 1
      ;;
  esac
  shift
done

if [[ "$DO_ALL" -eq 1 ]]; then
  WITH_PYTHON=1
  WITH_BUILD_TESTS=1
  RUN_TESTS=1
  RUN_SMOKE=1
fi

BUILD_TARGETS=("${DEFAULT_TARGETS[@]}")
if [[ "$WITH_PYTHON" -eq 1 ]]; then
  BUILD_TARGETS+=("//bindings/python:yche_kv")
fi
if [[ "$WITH_PYTHON" -eq 1 ]] || [[ "$WITH_WHEEL" -eq 1 ]]; then
  BUILD_TARGETS+=("//bindings/python:yche_kv_whl_stable")
fi
if [[ "$WITH_BUILD_TESTS" -eq 1 ]]; then
  BUILD_TARGETS+=(
    "//tests:kv_store_test"
    "//tests:brpc_adapter_test"
    "//tests:kv_store_py_test"
    "//smoke:kv_smoke"
    "//smoke:kv_smoke_py"
  )
fi

echo "+ $BAZEL build ${BUILD_TARGETS[*]}"
$BAZEL build "${BUILD_TARGETS[@]}"

# compile_commands.json uses -Iexternal/+http_archive+hiredis; clangd needs a root //external link.
# Same layout as hedron's refresh_compile_commands (see docs/ide_indexing.md).
if [[ ! -e external ]] && [[ -L build/bazel-out ]]; then
  echo "+ ln -sfn build/bazel-out/../../../external external"
  ln -sfn build/bazel-out/../../../external external
fi

if [[ "$RUN_TESTS" -eq 1 ]]; then
  echo "+ $BAZEL test //tests/..."
  $BAZEL test "//tests/..."
fi

if [[ "$RUN_SMOKE" -eq 1 ]]; then
  echo "+ $BAZEL run //smoke:kv_smoke"
  $BAZEL run "//smoke:kv_smoke"
  echo "+ $BAZEL run //smoke:kv_smoke_py"
  $BAZEL run "//smoke:kv_smoke_py"
fi
