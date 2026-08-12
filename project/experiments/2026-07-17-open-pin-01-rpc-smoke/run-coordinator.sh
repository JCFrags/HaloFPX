#!/usr/bin/env bash
set -uo pipefail

root="/home/connorb/halofpx-lab/open-pin-01"
out="${root}/rpc-smoke/coordinator"
binary="${root}/candidate/build-open-pin-01-rpc/bin/llama-server"
model="/opt/llm-usb4-cluster/models/qwen-official/qwen--qwen3-4b-gguf__bc640142c66e1fdd12af0bd68f40445458f3869b/Qwen3-4B-Q8_0.gguf"
port=18081
mkdir -p "${out}"
exec > >(tee "${out}/harness.log") 2>&1

cleanup() {
    if [[ -s "${out}/server.pid" ]]; then
        pid="$(cat "${out}/server.pid")"
        kill -TERM "${pid}" 2>/dev/null || true
        for _ in $(seq 1 30); do
            kill -0 "${pid}" 2>/dev/null || break
            sleep 1
        done
        kill -KILL "${pid}" 2>/dev/null || true
        wait "${pid}" 2>/dev/null || true
    fi
}
trap cleanup EXIT INT TERM

echo "timestamp=$(date --iso-8601=seconds)"
echo "host=$(hostname)"
sha256sum "${binary}" "${model}" | tee "${out}/input-sha256.txt"
git -C "${root}/candidate" rev-parse HEAD^{commit} HEAD^{tree} | tee "${out}/git-objects.txt"
git -C "${root}/candidate" status --porcelain=v1 | tee "${out}/git-status-before.txt"
systemctl is-active llm-usb4-coordinator.service || true
ss -ltnp | tee "${out}/listeners-before.txt"
grep -E 'MemAvailable|SwapTotal|SwapFree' /proc/meminfo | tee "${out}/memory-before.txt"
rocm-smi --showtemp --showuse --showmemuse | tee "${out}/rocm-before.txt" || true
if ! timeout 3 bash -c '</dev/tcp/10.44.0.1/50053'; then
    echo 'RPC worker is unreachable' >&2
    exit 90
fi

argv=(
  "${binary}" --rpc 10.44.0.1:50053 --device RPC0,ROCm0
  --tensor-split 1,1 --split-mode layer
  --model "${model}" --host 127.0.0.1 --port "${port}"
  --ctx-size 4096 --parallel 1 --threads 16 --threads-batch 16
  --n-gpu-layers 999 --flash-attn on --cache-type-k f16 --cache-type-v f16
  --seed 1234 --temp 0
)
printf '%q ' "${argv[@]}" | tee "${out}/argv.txt"
printf '\n' >> "${out}/argv.txt"
"${argv[@]}" > "${out}/server.log" 2>&1 &
pid=$!
echo "${pid}" > "${out}/server.pid"

ready=0
for _ in $(seq 1 120); do
    kill -0 "${pid}" 2>/dev/null || break
    if curl --silent --show-error --fail "http://127.0.0.1:${port}/health" > "${out}/health.json" 2>"${out}/health-curl.err"; then
        ready=1
        break
    fi
    sleep 1
done
echo "ready=${ready}"
if [[ "${ready}" != 1 ]]; then
    tail -200 "${out}/server.log" || true
    exit 91
fi
curl --silent --show-error --fail "http://127.0.0.1:${port}/props" > "${out}/props.json"
curl --silent --show-error --fail "http://127.0.0.1:${port}/slots" > "${out}/slots-before.json"
cat > "${out}/request.json" <<'JSON'
{"prompt":"Reply with exactly this text and nothing else: HALOFPX_RPC_SMOKE_OK","n_predict":32,"temperature":0,"seed":1234,"cache_prompt":false,"stream":false}
JSON
curl --silent --show-error --fail -H 'Content-Type: application/json' \
  --data-binary "@${out}/request.json" \
  "http://127.0.0.1:${port}/completion" > "${out}/response.json"
request_rc=$?
echo "request_rc=${request_rc}"
curl --silent --show-error --fail "http://127.0.0.1:${port}/slots" > "${out}/slots-after.json" || true
grep -E 'MemAvailable|SwapTotal|SwapFree' /proc/meminfo | tee "${out}/memory-after-request.txt"
rocm-smi --showtemp --showuse --showmemuse | tee "${out}/rocm-after-request.txt" || true
cleanup
trap - EXIT INT TERM
sleep 1
ss -ltnp > "${out}/listeners-after.txt"
git -C "${root}/candidate" status --porcelain=v1 | tee "${out}/git-status-after.txt"
journalctl -k --since '-10 minutes' --no-pager | grep -Ei 'amdgpu|xgmi|nvme|I/O error|oom|out of memory|segfault' > "${out}/kernel-watch.txt" || true
if ss -ltnH "sport = :${port}" | grep -q .; then
    echo 'coordinator listener remains' >&2
    exit 92
fi
echo "overall_rc=${request_rc}"
exit "${request_rc}"
