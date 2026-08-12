#!/usr/bin/env bash
set -euo pipefail

BIN="${LLAMA_BENCH_BIN:-}"
MODEL="${MODEL:-}"
DEVICE="${DEVICE:-}"
OUT_DIR="${OUT_DIR:-benchmark-results/$(date -u +%Y%m%dT%H%M%SZ)}"
REPEATS="${REPEATS:-3}"

usage() {
    cat <<'USAGE'
Usage: LLAMA_BENCH_BIN=/path/llama-bench MODEL=/path/model.gguf [DEVICE=ROCm0|Vulkan0] benchmark-gate.sh
USAGE
}
[[ -x "$BIN" && -f "$MODEL" ]] || { usage >&2; exit 2; }
mkdir -p "$OUT_DIR"
sha256sum "$BIN" "$MODEL" >"$OUT_DIR/SHA256SUMS"
"$BIN" --version >"$OUT_DIR/llama-version.txt" 2>&1 || true
"$BIN" --list-devices >"$OUT_DIR/devices.txt" 2>&1 || true
{
    echo "kernel=$(uname -r)"
    echo "device=${DEVICE:-auto}"
    echo "repeats=$REPEATS"
    echo "model=$MODEL"
    echo "command=$BIN -m $MODEL -p 512 -n 128 -ngl 999 -fa 1"
} >"$OUT_DIR/metadata.txt"

args=(-m "$MODEL" -p 512 -n 128 -ngl 999 -fa 1)
if [[ -n "$DEVICE" ]]; then args+=(-dev "$DEVICE"); fi
for ((i=1; i<=REPEATS; i++)); do
    "$BIN" "${args[@]}" 2>&1 | tee "$OUT_DIR/run-$i.txt"
done

echo "Results: $OUT_DIR"
