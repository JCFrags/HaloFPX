#!/usr/bin/env bash
set -uo pipefail

lane="${1:?lane: control or candidate}"
cache_type="${2:?cache type}"
port="${3:?loopback port}"
root="/home/connorb/halofpx-lab/open-pin-01"
out_root="${root}/runtime-smoke/${lane}-${cache_type}"
binary="${root}/${lane}/build-open-pin-01/bin/llama-server"
model="/opt/llm-usb4-cluster/models/qwen-official/qwen--qwen3-4b-gguf__bc640142c66e1fdd12af0bd68f40445458f3869b/Qwen3-4B-Q8_0.gguf"
server_log="${out_root}/server.log"
pid_file="${out_root}/server.pid"

mkdir -p "${out_root}"
exec > >(tee "${out_root}/harness.log") 2>&1

cleanup() {
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
        wait "${pid}" 2>/dev/null || true
    fi
}
trap cleanup EXIT INT TERM

echo "timestamp=$(date --iso-8601=seconds)"
echo "host=$(hostname)"
echo "lane=${lane}"
echo "cache_type=${cache_type}"
echo "port=${port}"
echo "binary=${binary}"
echo "model=${model}"
sha256sum "${binary}" "${model}" | tee "${out_root}/input-sha256.txt"
stat -c '%n|%s|%y' "${binary}" "${model}" | tee "${out_root}/input-stat.txt"
git -C "${root}/${lane}" rev-parse HEAD^{commit} HEAD^{tree} | tee "${out_root}/git-objects.txt"
git -C "${root}/${lane}" status --porcelain=v1 | tee "${out_root}/git-status-before.txt"
systemctl is-active llm-usb4-worker.service 2>/dev/null || true
systemctl is-active llm-usb4-coordinator.service 2>/dev/null || true
ss -ltnp | tee "${out_root}/listeners-before.txt"
grep -E 'MemAvailable|SwapTotal|SwapFree' /proc/meminfo | tee "${out_root}/memory-before.txt"
rocm-smi --showtemp --showuse --showmemuse | tee "${out_root}/rocm-before.txt" || true

if ss -ltnH "sport = :${port}" | grep -q .; then
    echo "ERROR: port ${port} is already in use"
    exit 90
fi

argv=(
    "${binary}"
    --model "${model}"
    --host 127.0.0.1
    --port "${port}"
    --ctx-size 4096
    --parallel 1
    --threads 16
    --threads-batch 16
    --device ROCm0
    --n-gpu-layers 999
    --flash-attn on
    --cache-type-k "${cache_type}"
    --cache-type-v "${cache_type}"
    --seed 1234
    --temp 0
)
printf '%q ' "${argv[@]}" | tee "${out_root}/argv.txt"
printf '\n' >> "${out_root}/argv.txt"

"${argv[@]}" >"${server_log}" 2>&1 &
server_pid=$!
echo "${server_pid}" > "${pid_file}"
echo "server_pid=${server_pid}"

ready=0
for _ in $(seq 1 120); do
    if ! kill -0 "${server_pid}" 2>/dev/null; then
        break
    fi
    if curl --silent --show-error --fail "http://127.0.0.1:${port}/health" > "${out_root}/health.json" 2>"${out_root}/health-curl.err"; then
        ready=1
        break
    fi
    sleep 1
done
echo "ready=${ready}"
if [[ "${ready}" != 1 ]]; then
    tail -200 "${server_log}" || true
    exit 91
fi

curl --silent --show-error --fail "http://127.0.0.1:${port}/props" > "${out_root}/props.json"
curl --silent --show-error --fail "http://127.0.0.1:${port}/slots" > "${out_root}/slots-before.json"
cat > "${out_root}/request.json" <<'JSON'
{"prompt":"Reply with exactly this text and nothing else: HALOFPX_RUNTIME_SMOKE_OK","n_predict":32,"temperature":0,"seed":1234,"cache_prompt":false,"stream":false}
JSON
curl --silent --show-error --fail \
    -H 'Content-Type: application/json' \
    --data-binary "@${out_root}/request.json" \
    "http://127.0.0.1:${port}/completion" > "${out_root}/response.json"
request_rc=$?
echo "request_rc=${request_rc}"
curl --silent --show-error --fail "http://127.0.0.1:${port}/slots" > "${out_root}/slots-after.json" || true
grep -E 'MemAvailable|SwapTotal|SwapFree' /proc/meminfo | tee "${out_root}/memory-after-request.txt"
rocm-smi --showtemp --showuse --showmemuse | tee "${out_root}/rocm-after-request.txt" || true

cleanup
trap - EXIT INT TERM
sleep 2
if ss -ltnH "sport = :${port}" | grep -q .; then
    echo "ERROR: listener remains on ${port}"
    exit 92
fi
if kill -0 "${server_pid}" 2>/dev/null; then
    echo "ERROR: process ${server_pid} remains"
    exit 93
fi
git -C "${root}/${lane}" status --porcelain=v1 | tee "${out_root}/git-status-after.txt"
journalctl -k --since '-5 minutes' --no-pager | grep -Ei 'amdgpu|xgmi|nvme|I/O error|oom|out of memory|segfault' > "${out_root}/kernel-watch.txt" || true
echo "overall_rc=${request_rc}"
exit "${request_rc}"
