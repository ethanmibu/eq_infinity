#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODE="${1:-apply}"

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format not found. Install it first." >&2
  exit 1
fi

FILES=()
while IFS= read -r file; do
  FILES+=("${file}")
done < <(
  if command -v rg >/dev/null 2>&1; then
    rg --files "${ROOT_DIR}/src" "${ROOT_DIR}/tests" \
      --glob '*.{h,hpp,cpp,cc}' \
      2>/dev/null || true
  else
    find "${ROOT_DIR}/src" "${ROOT_DIR}/tests" -type f \( \
      -name '*.h' -o -name '*.hpp' -o -name '*.cpp' -o -name '*.cc' \
    \) 2>/dev/null || true
  fi
)

if [ "${#FILES[@]}" -eq 0 ]; then
  echo "No C/C++ source files found."
  exit 0
fi

case "${MODE}" in
  apply)
    clang-format -i "${FILES[@]}"
    echo "Formatted ${#FILES[@]} file(s)."
    ;;
  check)
    clang-format --dry-run --Werror "${FILES[@]}"
    echo "Formatting check passed for ${#FILES[@]} file(s)."
    ;;
  *)
    echo "Usage: $0 [apply|check]" >&2
    exit 1
    ;;
esac
