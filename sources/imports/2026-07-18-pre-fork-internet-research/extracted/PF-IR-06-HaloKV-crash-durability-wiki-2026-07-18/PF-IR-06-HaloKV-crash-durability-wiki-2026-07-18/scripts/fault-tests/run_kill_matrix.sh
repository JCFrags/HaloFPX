#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR=${1:-}
if [[ -z $ROOT_DIR ]]; then echo "usage: $0 TEST_DIRECTORY" >&2; exit 64; fi
mkdir -p "$ROOT_DIR"
HERE=$(cd -- "$(dirname -- "$0")/.." && pwd)
BUILD=${TMPDIR:-/tmp}/pf-ir-06-build
mkdir -p "$BUILD"
cc -O2 -Wall -Wextra -Werror "$HERE/publish_probe.c" -o "$BUILD/publish_probe"

payload='generation-payload-0123456789'
for point in after_create after_write after_file_sync after_publish after_dir_sync; do
  name="obj-$point"
  rm -f -- "$ROOT_DIR/$name" "$ROOT_DIR"/.pf-ir-06.tmp.* 2>/dev/null || true
  echo "== $point =="
  set +e
  PF_FAILPOINT=$point "$BUILD/publish_probe" "$ROOT_DIR" "$name" "$payload"
  rc=$?
  set -e
  echo "exit=$rc"
  find "$ROOT_DIR" -maxdepth 1 -printf '%f %s bytes\n' | sort
  if [[ $point == after_dir_sync && -f $ROOT_DIR/$name ]]; then
    cmp -s <(printf %s "$payload") "$ROOT_DIR/$name" || { echo "content mismatch"; exit 1; }
  fi
done

echo "Process-kill matrix completed. This is not a power-cut durability test."
