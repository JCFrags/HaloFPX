#!/usr/bin/env bash
set -Eeuo pipefail
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
ROOT=$(cd -- "$SCRIPT_DIR/.." && pwd)
source "$SCRIPT_DIR/common.sh"

require_cmd iperf3
require_cmd python3
OUT=${1:-results/dual-$(date -u +%Y%m%dT%H%M%SZ)}
mkdir -p "$OUT"
LOCAL0=${LOCAL0:-10.44.0.1}
PEER0=${PEER0:-10.44.0.2}
LOCAL1=${LOCAL1:-10.44.1.1}
PEER1=${PEER1:-10.44.1.2}
PORT0=${PORT0:-5201}
PORT1=${PORT1:-5202}
PARALLEL=${PARALLEL:-4}
DURATION=${DURATION:-30}
OMIT=${OMIT:-3}
RUN_BASELINES=${RUN_BASELINES:-1}
REVERSE=${REVERSE:-0}
rev=()
[[ $REVERSE == 1 ]] && rev=(-R)

run_one() {
    local local_ip=$1 peer=$2 port=$3 output=$4
    iperf3 -c "$peer" -B "$local_ip" -p "$port" -P "$PARALLEL" \
        -t "$DURATION" -O "$OMIT" -J "${rev[@]}" > "$output"
}

ip -s link show > "$OUT/netdev-before.txt"
nstat -asz > "$OUT/nstat-before.txt" 2>/dev/null || true

if [[ $RUN_BASELINES == 1 ]]; then
    log 'running isolated link 0 baseline'
    run_one "$LOCAL0" "$PEER0" "$PORT0" "$OUT/baseline-link0.json"
    log 'running isolated link 1 baseline'
    run_one "$LOCAL1" "$PEER1" "$PORT1" "$OUT/baseline-link1.json"
fi

log 'running both path-bound flows concurrently'
run_one "$LOCAL0" "$PEER0" "$PORT0" "$OUT/dual-link0.json" & p0=$!
run_one "$LOCAL1" "$PEER1" "$PORT1" "$OUT/dual-link1.json" & p1=$!
status=0
wait "$p0" || status=1
wait "$p1" || status=1

ip -s link show > "$OUT/netdev-after.txt"
nstat -asz > "$OUT/nstat-after.txt" 2>/dev/null || true

python3 "$ROOT/tools/summarize_iperf.py" "$OUT" | tee "$OUT/summary.md"
exit "$status"
