#!/usr/bin/env bash
set -Eeuo pipefail
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
source "$SCRIPT_DIR/common.sh"

require_root
PARENT=${PARENT:-}
STREAM_NAME=${STREAM_NAME:-data}
IN_HOPID=${IN_HOPID:--1}
OUT_HOPID=${OUT_HOPID:--1}
RING_SIZE=${RING_SIZE:-}
THROTTLING=${THROTTLING:-}
[[ -n $PARENT ]] || die 'set PARENT to the XDomain service name, e.g. 0-1.0'

run modprobe thunderbolt_stream
if ! mountpoint -q /sys/kernel/config; then
    run mount -t configfs none /sys/kernel/config
fi
BASE=/sys/kernel/config/thunderbolt/stream
[[ -d $BASE ]] || die 'USB4STREAM ConfigFS is unavailable; requires CONFIG_USB4_STREAM (Linux 7.2 development or backport)'
TARGET=$BASE/$PARENT/$STREAM_NAME
run mkdir -p "$TARGET"

if [[ ${DRY_RUN:-0} == 1 ]]; then
    log "would write in_hopid=$IN_HOPID out_hopid=$OUT_HOPID under $TARGET"
else
    echo "$IN_HOPID" > "$TARGET/in_hopid"
    echo "$OUT_HOPID" > "$TARGET/out_hopid"
    [[ -n $RING_SIZE && -w $TARGET/ring_size ]] && echo "$RING_SIZE" > "$TARGET/ring_size"
    [[ -n $THROTTLING && -w $TARGET/throttling ]] && echo "$THROTTLING" > "$TARGET/throttling"
    find "$BASE/$PARENT" -maxdepth 2 -type f -print -exec cat {} \;
    ls -l /dev/tbstream* 2>/dev/null || true
fi
