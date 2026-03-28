#!/usr/bin/env bash
# Install Bazelisk as `bazel` into ~/.local/bin (no sudo) or PREFIX=/usr/local (with sudo).
# Usage:
#   ./tools/install_bazelisk.sh
#   PREFIX=/usr/local sudo ./tools/install_bazelisk.sh
set -euo pipefail

VERSION="${BAZELISK_VERSION:-1.25.0}"
ARCH="$(uname -m)"
case "${ARCH}" in
  x86_64) ASSET="bazelisk-linux-amd64" ;;
  aarch64|arm64) ASSET="bazelisk-linux-arm64" ;;
  *)
    echo "Unsupported arch: ${ARCH}" >&2
    exit 1
    ;;
esac

URL="https://github.com/bazelbuild/bazelisk/releases/download/v${VERSION}/${ASSET}"
PREFIX="${PREFIX:-${HOME}/.local/bin}"

mkdir -p "${PREFIX}"
TMP="$(mktemp)"
trap 'rm -f "${TMP}"' EXIT

echo "Downloading ${URL} ..."
if ! curl -fsSL -o "${TMP}" "${URL}"; then
  echo "Primary download failed. If you are behind a firewall, try a mirror or download the asset in a browser:" >&2
  echo "  ${URL}" >&2
  echo "Save it as ${PREFIX}/bazel and: chmod +x ${PREFIX}/bazel" >&2
  exit 1
fi

install -m 0755 "${TMP}" "${PREFIX}/bazel"
echo "Installed: ${PREFIX}/bazel"
"${PREFIX}/bazel" version

if [[ ":${PATH}:" != *":${PREFIX}:"* ]]; then
  echo >&2
  echo "Add to PATH (e.g. in ~/.bashrc or ~/.zshrc):" >&2
  echo "  export PATH=\"${PREFIX}:\${PATH}\"" >&2
fi
