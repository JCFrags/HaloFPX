#!/usr/bin/env bash
set -Eeuo pipefail
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
source "$SCRIPT_DIR/common.sh"

OUT=${1:-results/perf-$(date -u +%Y%m%dT%H%M%SZ)}
DURATION=${DURATION:-30}
PID=${PID:-}
mkdir -p "$OUT"

pids=()
cleanup() { for p in "${pids[@]:-}"; do kill "$p" 2>/dev/null || true; done; }
trap cleanup EXIT

if command -v mpstat >/dev/null; then
    mpstat -P ALL 1 "$DURATION" > "$OUT/mpstat.txt" & pids+=("$!")
fi
if [[ -n $PID ]] && command -v pidstat >/dev/null; then
    pidstat -t -p "$PID" 1 "$DURATION" > "$OUT/pidstat.txt" & pids+=("$!")
fi
if command -v perf >/dev/null && [[ ${EUID:-$(id -u)} -eq 0 ]]; then
    perf stat -a -o "$OUT/perf-stat.txt" \
      -e cycles,instructions,cache-misses,context-switches,cpu-migrations,irq:irq_handler_entry \
      -- sleep "$DURATION" & pids+=("$!")
else
    warn 'perf system-wide collection skipped (requires perf and root)'
fi

cp /proc/interrupts "$OUT/interrupts-before.txt"
cp /proc/softirqs "$OUT/softirqs-before.txt"
cp /proc/net/softnet_stat "$OUT/softnet-before.txt"
ip -s link show > "$OUT/netdev-before.txt"
nstat -asz > "$OUT/nstat-before.txt" 2>/dev/null || true

sleep "$DURATION"
wait || true

cp /proc/interrupts "$OUT/interrupts-after.txt"
cp /proc/softirqs "$OUT/softirqs-after.txt"
cp /proc/net/softnet_stat "$OUT/softnet-after.txt"
ip -s link show > "$OUT/netdev-after.txt"
nstat -asz > "$OUT/nstat-after.txt" 2>/dev/null || true
log "performance capture complete: $OUT"
