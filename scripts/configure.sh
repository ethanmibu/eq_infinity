#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build}"

if ! command -v cmake >/dev/null 2>&1; then
  echo "cmake not found. Install CMake 3.21+ first." >&2
  exit 1
fi

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" "$@"
echo "Configured at: ${BUILD_DIR}"
