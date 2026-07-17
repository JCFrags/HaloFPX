#!/usr/bin/env bash
set -euo pipefail
usage(){ echo "usage: $0 server [BIND_IP] | client HOST OUT_DIR [BIND_IP]" >&2; exit 2; }
[[ $# -ge 1 ]] || usage
MODE=$1
if [[ $MODE == server ]]; then
  BIND=${2:-}
  args=(-s --interval 1)
  [[ -n $BIND ]] && args+=(-B "$BIND")
  exec iperf3 "${args[@]}"
elif [[ $MODE == client ]]; then
  [[ $# -ge 3 ]] || usage
  HOST=$2; OUT=$3; BIND=${4:-}; mkdir -p "$OUT"
  REPS=${REPS:-5}; DURATION=${DURATION:-60}; OMIT=${OMIT:-10}
  printf 'host=%s\nreps=%s\nduration_s=%s\nomit_s=%s\nbind=%s\n' "$HOST" "$REPS" "$DURATION" "$OMIT" "$BIND" > "$OUT/config.txt"
  iperf3 --version > "$OUT/iperf3-version.txt" 2>&1
  base=(-c "$HOST" -t "$DURATION" -O "$OMIT" -i 1 -J --get-server-output)
  [[ -n $BIND ]] && base+=(-B "$BIND")
  bidir=0; iperf3 --help 2>&1 | grep -q -- '--bidir' && bidir=1
  for p in 1 4 8; do
    for rep in $(seq 1 "$REPS"); do
      iperf3 "${base[@]}" -P "$p" > "$OUT/a-to-b-p${p}-r${rep}.json"
      iperf3 "${base[@]}" -P "$p" -R > "$OUT/b-to-a-p${p}-r${rep}.json"
      if [[ $bidir -eq 1 ]]; then iperf3 "${base[@]}" -P "$p" --bidir > "$OUT/bidirectional-p${p}-r${rep}.json"; fi
    done
  done
  ip -details -statistics link > "$OUT/ip-link-after.txt"
  ( cd "$OUT" && find . -type f ! -name MANIFEST.sha256 -print0 | sort -z | xargs -0 sha256sum > MANIFEST.sha256 )
else usage; fi
