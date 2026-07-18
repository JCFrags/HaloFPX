#!/usr/bin/env bash
set -uo pipefail

out="/home/connorb/halofpx-lab/open-pin-01/rpc-smoke/worker"
pid_file="${out}/worker.pid"
if [[ -s "${pid_file}" ]]; then
    pid="$(cat "${pid_file}")"
    if kill -0 "${pid}" 2>/dev/null; then
        kill -TERM "${pid}" 2>/dev/null || true
        for _ in $(seq 1 30); do
            kill -0 "${pid}" 2>/dev/null || break
            sleep 1
        done
        kill -KILL "${pid}" 2>/dev/null || true
    fi
fi
sleep 1
ss -ltnp > "${out}/listeners-after.txt"
grep -E 'MemAvailable|SwapTotal|SwapFree' /proc/meminfo > "${out}/memory-after.txt"
rocm-smi --showtemp --showuse --showmemuse > "${out}/rocm-after.txt" 2>&1 || true
journalctl -k --since '-10 minutes' --no-pager | grep -Ei 'amdgpu|xgmi|nvme|I/O error|oom|out of memory|segfault' > "${out}/kernel-watch.txt" || true
if ss -ltnH 'sport = :50053' | grep -q .; then
    echo 'worker listener remains' >&2
    exit 93
fi
