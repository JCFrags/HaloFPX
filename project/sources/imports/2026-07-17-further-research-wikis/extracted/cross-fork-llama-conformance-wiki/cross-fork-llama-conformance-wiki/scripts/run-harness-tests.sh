#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)

if ! python3 -c 'import pytest' >/dev/null 2>&1; then
  printf '%s\n' 'error: pytest is not installed in the active Python environment' >&2
  printf '%s\n' 'install pytest, then rerun this script; the harness itself is loaded directly from harness/src' >&2
  exit 2
fi

PYTHONPATH="$root/harness/src${PYTHONPATH:+:$PYTHONPATH}" \
  python3 -m pytest "$root/harness/tests"
