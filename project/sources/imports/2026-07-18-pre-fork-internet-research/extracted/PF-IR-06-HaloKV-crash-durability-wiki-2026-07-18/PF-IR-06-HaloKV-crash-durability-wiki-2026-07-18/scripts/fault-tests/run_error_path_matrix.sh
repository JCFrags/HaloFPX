#!/usr/bin/env bash
set -euo pipefail
TEST_DIR=${1:-}
if [[ -z $TEST_DIR ]]; then
  echo "usage: $0 TEST_DIRECTORY" >&2
  exit 64
fi
mkdir -p -- "$TEST_DIR"
HERE=$(cd -- "$(dirname -- "$0")/.." && pwd)
BUILD=${TMPDIR:-/tmp}/pf-ir-06-error-build
mkdir -p -- "$BUILD"
cc -std=c11 -O2 -Wall -Wextra -Werror "$HERE/publish_probe.c" -o "$BUILD/publish_probe"
cc -std=c11 -O2 -Wall -Wextra -Werror -fPIC -shared "$HERE/inject_short_write.c" -ldl -pthread -o "$BUILD/inject_short_write.so"
cc -std=c11 -O2 -Wall -Wextra -Werror -fPIC -shared "$HERE/inject_stage_fault.c" -ldl -pthread -o "$BUILD/inject_stage_fault.so"

cleanup() { rm -f -- "$TEST_DIR"/.pf-ir-06.tmp.*; }
trap cleanup EXIT

run_expect_failure() {
  local label=$1 final=$2
  shift 2
  local log="$BUILD/$label.log"
  rm -f -- "$TEST_DIR/$final" "$TEST_DIR"/.pf-ir-06.tmp.* "$log"
  set +e
  "$@" >"$log" 2>&1
  local rc=$?
  set -e
  if [[ $rc -eq 0 ]]; then
    echo "$label: expected failure" >&2; cat "$log" >&2; exit 1
  fi
  if grep -q 'ACK_AFTER_DIRECTORY_FSYNC' "$log"; then
    echo "$label: emitted ACK on failure" >&2; cat "$log" >&2; exit 1
  fi
  echo "$label: PASS (rc=$rc)"
}

rm -f -- "$TEST_DIR/short" "$TEST_DIR"/.pf-ir-06.tmp.*
PF_WRITE_CAP=3 LD_PRELOAD="$BUILD/inject_short_write.so" \
  "$BUILD/publish_probe" "$TEST_DIR" short '0123456789abcdef' >/dev/null
cmp -s <(printf %s '0123456789abcdef') "$TEST_DIR/short"
echo 'positive-short-write: PASS'

rm -f -- "$TEST_DIR/eintr" "$TEST_DIR"/.pf-ir-06.tmp.*
PF_INJECT_AT=0 PF_INJECT_ERRNO=EINTR LD_PRELOAD="$BUILD/inject_short_write.so" \
  "$BUILD/publish_probe" "$TEST_DIR" eintr 'retry-after-eintr' >/dev/null
cmp -s <(printf %s 'retry-after-eintr') "$TEST_DIR/eintr"
echo 'EINTR-retry: PASS'

run_expect_failure zero-write zero \
  env PF_INJECT_AT=0 LD_PRELOAD="$BUILD/inject_short_write.so" \
  "$BUILD/publish_probe" "$TEST_DIR" zero 'must-not-publish'
[[ ! -e $TEST_DIR/zero ]] || { echo 'zero-write: final unexpectedly exists' >&2; exit 1; }

for err in ENOSPC EIO; do
  final="pwrite-${err,,}"
  run_expect_failure "$final" "$final" \
    env PF_STAGE_OP=pwrite PF_STAGE_ERRNO=$err LD_PRELOAD="$BUILD/inject_stage_fault.so" \
    "$BUILD/publish_probe" "$TEST_DIR" "$final" 'must-not-publish'
  [[ ! -e $TEST_DIR/$final ]] || { echo "$final: final unexpectedly exists" >&2; exit 1; }
done

for err in ENOSPC EIO; do
  final="fdatasync-${err,,}"
  run_expect_failure "$final" "$final" \
    env PF_STAGE_OP=fdatasync PF_STAGE_ERRNO=$err LD_PRELOAD="$BUILD/inject_stage_fault.so" \
    "$BUILD/publish_probe" "$TEST_DIR" "$final" 'must-not-publish'
  [[ ! -e $TEST_DIR/$final ]] || { echo "$final: final unexpectedly exists" >&2; exit 1; }
done

printf %s old-generation >"$TEST_DIR/replace-target"
set +e
env PF_REPLACE=1 PF_STAGE_OP=renameat PF_STAGE_ERRNO=EIO \
  LD_PRELOAD="$BUILD/inject_stage_fault.so" \
  "$BUILD/publish_probe" "$TEST_DIR" replace-target 'new-generation' >"$BUILD/renameat-eio.log" 2>&1
rc=$?
set -e
[[ $rc -ne 0 ]] || { echo 'renameat-EIO: expected failure' >&2; exit 1; }
! grep -q 'ACK_AFTER_DIRECTORY_FSYNC' "$BUILD/renameat-eio.log"
cmp -s <(printf %s old-generation) "$TEST_DIR/replace-target"
echo 'renameat-EIO-old-generation-preserved: PASS'

printf %s old-generation >"$TEST_DIR/dirsync-target"
set +e
env PF_REPLACE=1 PF_STAGE_OP=fsync PF_STAGE_ERRNO=EIO \
  LD_PRELOAD="$BUILD/inject_stage_fault.so" \
  "$BUILD/publish_probe" "$TEST_DIR" dirsync-target 'new-complete-generation' >"$BUILD/dirsync-eio.log" 2>&1
rc=$?
set -e
[[ $rc -ne 0 ]] || { echo 'dirsync-EIO: expected failure' >&2; exit 1; }
! grep -q 'ACK_AFTER_DIRECTORY_FSYNC' "$BUILD/dirsync-eio.log"
cmp -s <(printf %s new-complete-generation) "$TEST_DIR/dirsync-target"
echo 'directory-fsync-EIO-no-ack-complete-visible-object: PASS'

echo 'Error-path matrix completed. This injector is not a delayed-writeback, journal, block-layer, or power-cut test.'
