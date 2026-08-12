#!/usr/bin/env bash
set -euo pipefail

root="/home/connorb/halofpx-lab/open-pin-01"
out="${root}/rpc-smoke/worker"
binary="${root}/candidate/build-open-pin-01-rpc/bin/rpc-server"
mkdir -p "${out}"

if ss -ltnH 'sport = :50053' | grep -q .; then
    echo 'port 50053 already in use' >&2
    exit 90
fi
if [[ "$(systemctl is-active llm-usb4-worker.service || true)" != inactive ]]; then
    echo 'deployed worker is not inactive' >&2
    exit 91
fi
sha256sum "${binary}" > "${out}/input-sha256.txt"
ss -ltnp > "${out}/listeners-before.txt"
grep -E 'MemAvailable|SwapTotal|SwapFree' /proc/meminfo > "${out}/memory-before.txt"
rocm-smi --showtemp --showuse --showmemuse > "${out}/rocm-before.txt" 2>&1 || true
printf '%q ' "${binary}" --host 10.44.0.1 --port 50053 --device ROCm0 > "${out}/argv.txt"
printf '\n' >> "${out}/argv.txt"
nohup "${binary}" --host 10.44.0.1 --port 50053 --device ROCm0 > "${out}/worker.log" 2>&1 &
pid=$!
echo "${pid}" > "${out}/worker.pid"
for _ in $(seq 1 30); do
    kill -0 "${pid}" 2>/dev/null || break
    ss -ltnH 'sport = :50053' | grep -q . && exit 0
    sleep 1
done
echo 'worker did not become ready' >&2
kill -TERM "${pid}" 2>/dev/null || true
exit 92
