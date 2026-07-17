#!/usr/bin/env bash
# Application-network calibration helper using two already-configured interfaces.
# Requires iperf3 servers on remote ports 5201 and 5202. It does not configure interfaces.
set -euo pipefail
usage() {
  cat <<EOF
Usage: $0 LOCAL_IP_1 REMOTE_IP_1 LOCAL_IP_2 REMOTE_IP_2 [SECONDS] [OUTDIR]

Example server commands on node B:
  iperf3 -s -p 5201
  iperf3 -s -p 5202

The client binds by local IP so routing must already pin each path correctly.
Verify the route/interface mapping before relying on results.
EOF
}
[ "$#" -ge 4 ] || { usage; exit 2; }
L1=$1; R1=$2; L2=$3; R2=$4; SECONDS=${5:-20}; OUT=${6:-measurements/iperf3-$(date +%Y%m%dT%H%M%S)}
command -v iperf3 >/dev/null || { echo "iperf3 is required" >&2; exit 1; }
mkdir -p "$OUT"
echo "Path 1 alone"
iperf3 -c "$R1" -B "$L1" -p 5201 -t "$SECONDS" -J > "$OUT/path1.json"
echo "Path 2 alone"
iperf3 -c "$R2" -B "$L2" -p 5202 -t "$SECONDS" -J > "$OUT/path2.json"
echo "Both paths concurrently"
iperf3 -c "$R1" -B "$L1" -p 5201 -t "$SECONDS" -J > "$OUT/both-path1.json" & P1=$!
iperf3 -c "$R2" -B "$L2" -p 5202 -t "$SECONDS" -J > "$OUT/both-path2.json" & P2=$!
wait "$P1" "$P2"
echo "Reverse-direction tests"
iperf3 -c "$R1" -B "$L1" -p 5201 -t "$SECONDS" -R -J > "$OUT/reverse-path1.json"
iperf3 -c "$R2" -B "$L2" -p 5202 -t "$SECONDS" -R -J > "$OUT/reverse-path2.json"
echo "Raw outputs written to $OUT. These are transport checks, not model-runtime measurements."
