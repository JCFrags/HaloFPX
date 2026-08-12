#!/usr/bin/env bash
set -Eeuo pipefail
command -v iperf3 >/dev/null || { echo 'iperf3 is required' >&2; exit 1; }
mkdir -p results/iperf-servers
PORT0=${PORT0:-5201}
PORT1=${PORT1:-5202}
cleanup() { kill "${p0:-}" "${p1:-}" 2>/dev/null || true; }
trap cleanup EXIT INT TERM
iperf3 -s -p "$PORT0" > "results/iperf-servers/port${PORT0}.log" 2>&1 & p0=$!
iperf3 -s -p "$PORT1" > "results/iperf-servers/port${PORT1}.log" 2>&1 & p1=$!
echo "servers running as $p0 and $p1; Ctrl-C to stop"
wait
