#!/usr/bin/env bash
# Runs `bazel coverage --config=coverage`, then generates HTML under build/coverage-html/ (genhtml).
# - GCOV: matches ${CC:-gcc} when unset (see docs/coverage.md).
# - COVERAGE_HTML_DIR: output directory for HTML (default: build/coverage-html).
# - SKIP_COVERAGE_HTML=1: skip genhtml (e.g. CI without lcov).
# - COVERAGE_NOCACHE=1: pass --nocache_test_results so tests re-run (avoids stale coverage when Bazel shows "Executed 0 out of 1").
set -euo pipefail

if [[ -z "${GCOV:-}" ]]; then
  _cc="$(command -v "${CC:-gcc}")"
  _cc_real="$(realpath "$_cc")"
  _dir="$(dirname "$_cc_real")"
  _base="$(basename "$_cc_real")"
  _gcov="${_dir}/${_base//gcc/gcov}"
  if [[ -x "$_gcov" ]]; then
    export GCOV="$_gcov"
  fi
fi

cov_args=(--config=coverage)
if [[ "${COVERAGE_NOCACHE:-}" == "1" ]]; then
  cov_args+=(--nocache_test_results)
fi
bazel coverage "${cov_args[@]}" "$@"

if [[ "${SKIP_COVERAGE_HTML:-}" == "1" ]]; then
  echo "SKIP_COVERAGE_HTML=1: skipping HTML report."
  exit 0
fi

if ! command -v genhtml >/dev/null 2>&1; then
  echo "genhtml not found (install lcov, e.g. sudo apt install lcov). Set SKIP_COVERAGE_HTML=1 to skip." >&2
  exit 1
fi

OUT_DIR="${COVERAGE_HTML_DIR:-build/coverage-html}"
ROOT="$(bazel info execution_root)"
REPORT="${ROOT}/bazel-out/_coverage/_coverage_report.dat"

if [[ ! -f "$REPORT" ]]; then
  echo "Merged lcov report not found: $REPORT" >&2
  exit 1
fi

genhtml -o "$OUT_DIR" "$REPORT"
echo "Coverage HTML: ${OUT_DIR}/index.html"
