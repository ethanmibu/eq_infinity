#!/usr/bin/env bash
set -euo pipefail

if ! command -v brew >/dev/null 2>&1; then
  echo "Homebrew not found. Install Homebrew first: https://brew.sh" >&2
  exit 1
fi

brew update
brew install cmake ninja clang-format || true

echo "macOS setup complete."
