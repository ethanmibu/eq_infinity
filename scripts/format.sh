#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format not found. Install it first." >&2
  exit 1
fi

find "${ROOT_DIR}/src" -type f \( -name "*.h" -o -name "*.hpp" -o -name "*.cpp" -o -name "*.cc" \) -print0 \
  | xargs -0 clang-format -i
echo "Formatted source files."
