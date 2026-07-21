#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 3 ]]; then
    echo "usage: $0 PID EVIDENCE_ROOT DURATION_SECONDS" >&2
    exit 2
fi

pid=$1
root=$2
duration=$3

[[ $pid =~ ^[1-9][0-9]*$ ]] || { echo "invalid PID" >&2; exit 2; }
[[ $duration =~ ^[1-9][0-9]*$ ]] || { echo "invalid duration" >&2; exit 2; }
[[ -d $root && ! -L $root ]] || { echo "evidence root must be an existing directory" >&2; exit 2; }
sudo -n kill -0 "$pid" || { echo "target PID is not running" >&2; exit 2; }

ip -s -j link show thunderbolt0 > "$root/net-before.json"
ip -s -j link show thunderbolt1 >> "$root/net-before.json" 2>/dev/null || true
nstat -az > "$root/nstat-before.txt"
ss -tinm > "$root/ss-before.txt"
date +'%s%N' > "$root/monitor-start-realtime-ns.txt"
cut -d ' ' -f 1 /proc/uptime > "$root/monitor-start-uptime-seconds.txt"

pidstat -h -u -w -t -p "$pid" 1 "$duration" > "$root/pidstat.txt" 2>&1 &
pidstat_pid=$!

sudo -n timeout --signal=INT "${duration}s" perf trace \
    -p "$pid" --duration 0.05 \
    -e recvfrom,sendto,poll,ppoll,epoll_wait,futex \
    > "$root/perf-trace.txt" 2>&1 &
perf_pid=$!

sudo -n trace-cmd record -o "$root/amdgpu.dat" \
    -e amdgpu:amdgpu_sched_run_job \
    -e amdgpu:amdgpu_cs \
    sleep "$duration" > "$root/trace-cmd.txt" 2>&1 &
trace_pid=$!

(
    samples=$((duration * 10))
    for ((i = 0; i < samples; ++i)); do
        printf '%s,' "$(date +'%s%N')"
        tr '\n' ',' < /sys/class/drm/card0/device/gpu_busy_percent
        tr '\n' ',' < /sys/class/drm/card0/device/mem_info_vram_used
        tr '\n' ',' < /sys/class/drm/card0/device/mem_info_gtt_used
        printf '\n'
        sleep 0.1
    done
) > "$root/gpu-samples.csv" &
gpu_pid=$!

wait "$pidstat_pid" || true
wait "$perf_pid" || true
wait "$trace_pid" || true
wait "$gpu_pid" || true

date +'%s%N' > "$root/monitor-end-realtime-ns.txt"
cut -d ' ' -f 1 /proc/uptime > "$root/monitor-end-uptime-seconds.txt"
ip -s -j link show thunderbolt0 > "$root/net-after.json"
ip -s -j link show thunderbolt1 >> "$root/net-after.json" 2>/dev/null || true
nstat -az > "$root/nstat-after.txt"
ss -tinm > "$root/ss-after.txt"
sudo -n trace-cmd report -i "$root/amdgpu.dat" > "$root/amdgpu-report.txt"
