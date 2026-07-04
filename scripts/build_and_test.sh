#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if ! command -v gcc >/dev/null 2>&1; then
  echo "gcc is required"
  exit 1
fi

if ! command -v curl-config >/dev/null 2>&1; then
  echo "libcurl development headers required (curl-config not found)"
  exit 1
fi

export CFLAGS="${CFLAGS:-} $(curl-config --cflags)"
export LDFLAGS="${LDFLAGS:-} $(curl-config --libs)"

make clean all test

echo "Build and tests completed successfully."
