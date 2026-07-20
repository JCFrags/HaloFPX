#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 6 ]]; then
    echo "usage: $0 OUT_ROOT BLOCK SERVER_BINARY REQUEST_BODY RETAINED PORT" >&2
    exit 2
fi

out_root=$1
block=$2
server_binary=$3
request_body=$4
retained=$5
port=$6
block_root="${out_root}/${block}"
server_unit="halofpx-p07-server-${block}"
model=/opt/llm-usb4-cluster/models/rcmorano_saricles-minimax-m2.7-reap-172b-a10b-rocmfpx/dba517197f2854f3d362529e13abddcdcad6c10b/saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf
mptcpize=/opt/llm-usb4-cluster/mptcpd/bin/mptcpize
expected_content_sha256=3c9bcc5624f4a1d6558c51be89e2443ffba4b2206d10b632c89b7ef64fdd026f
expected_request_sha256=f5b273f95a852d5e34252715bc953fa4eb101273d262b9d778c16138d7c00b7c
monitor_pid=

mkdir -p "${block_root}"
if [[ ! ${retained} =~ ^[1-9][0-9]*$ || ! ${port} =~ ^[0-9]+$ ]]; then
    echo "RETAINED must be positive and PORT must be numeric" >&2
    exit 2
fi
actual_request_sha256=$(sha256sum "${request_body}" | awk '{ print $1 }')
if [[ ${actual_request_sha256} != "${expected_request_sha256}" ]]; then
    echo "request body does not match the qualified primary workload" >&2
    exit 2
fi

cleanup() {
    set +e
    systemctl stop "${server_unit}.service" >"${block_root}/server-stop.txt" 2>&1
    if [[ -n ${monitor_pid} ]]; then
        kill "${monitor_pid}" 2>/dev/null
        wait "${monitor_pid}" 2>/dev/null
    fi
    journalctl -u "${server_unit}.service" --no-pager >"${block_root}/server-journal.txt" 2>&1
    ip -s link show >"${block_root}/links-after.txt" 2>&1
    ss -Mit >"${block_root}/mptcp-after.txt" 2>&1
    /opt/rocm/bin/rocm-smi --showuse --showmemuse --showtemp --showpower >"${block_root}/rocm-after.txt" 2>&1 || true
}
trap cleanup EXIT

date --iso-8601=ns >"${block_root}/started-at.txt"
uname -a >"${block_root}/uname.txt"
sha256sum "${server_binary}" "${request_body}" >"${block_root}/inputs.sha256"
stat --printf='%n\nsize=%s\ninode=%i\nmtime=%y\n' "${model}" >"${block_root}/model-stat.txt"
env | LC_ALL=C sort >"${block_root}/runner-environment.txt"
ip -s link show >"${block_root}/links-before.txt"
ss -Mit >"${block_root}/mptcp-before.txt"
/opt/rocm/bin/rocm-smi --showuse --showmemuse --showtemp --showpower >"${block_root}/rocm-before.txt" 2>&1 || true

(
    while true; do
        date --iso-8601=ns
        /opt/rocm/bin/rocm-smi --showuse --showmemuse --showtemp --showpower 2>&1 || true
        sleep 2
    done
) >"${block_root}/rocm-telemetry.txt" 2>&1 &
monitor_pid=$!

systemd-run --unit="${server_unit}" --collect \
    --property=Environment=HSA_ENABLE_SDMA=0 \
    "${mptcpize}" run "${server_binary}" \
    --rpc 10.44.0.1:50053 \
    --device RPC0,ROCm0 \
    --tensor-split 1,1 \
    --split-mode layer \
    --model "${model}" \
    --alias "halofpx-p07-${block}" \
    --host 127.0.0.1 \
    --port "${port}" \
    --ctx-size 4096 \
    --parallel 1 \
    --threads 16 \
    --threads-batch 16 \
    --n-gpu-layers 999 \
    --flash-attn on \
    --cache-type-k q8_0 \
    --cache-type-v q8_0 \
    --batch-size 512 \
    --ubatch-size 512 \
    --fit off \
    --no-mmap \
    --direct-io \
    --seed 1234 \
    --temp 0 \
    --no-webui \
    --offline \
    --metrics

ready=0
for _ in $(seq 1 180); do
    if curl --fail --silent --show-error "http://127.0.0.1:${port}/health" >"${block_root}/health.json" 2>"${block_root}/health-error.txt"; then
        ready=1
        break
    fi
    sleep 2
done
if [[ ${ready} -ne 1 ]]; then
    echo "server did not become healthy" >&2
    exit 1
fi

curl --fail --silent --show-error \
    --output "${block_root}/warmup.json" \
    --write-out '%{http_code} %{time_total}\n' \
    -H 'Content-Type: application/json' \
    --data-binary "@${request_body}" \
    "http://127.0.0.1:${port}/completion" >"${block_root}/warmup.curl"

for i in $(seq 1 "${retained}"); do
    curl --fail --silent --show-error \
        --output "${block_root}/retained-${i}.json" \
        --write-out '%{http_code} %{time_total}\n' \
        -H 'Content-Type: application/json' \
        --data-binary "@${request_body}" \
        "http://127.0.0.1:${port}/completion" >"${block_root}/retained-${i}.curl"
    jq -e '.timings.prompt_n == 1129 and .timings.predicted_n == 128' \
        "${block_root}/retained-${i}.json" >/dev/null
done

for response in "${block_root}"/retained-*.json; do
    jq -j '.content' "${response}" | sha256sum
done >"${block_root}/content.sha256"
sort -u "${block_root}/content.sha256" >"${block_root}/content.unique.sha256"
if [[ $(wc -l <"${block_root}/content.unique.sha256") -ne 1 ]]; then
    echo "retained response content hashes diverged" >&2
    exit 1
fi
actual_content_sha256=$(awk '{ print $1 }' "${block_root}/content.unique.sha256")
if [[ ${actual_content_sha256} != "${expected_content_sha256}" ]]; then
    echo "retained response content hash does not match the qualified exact output" >&2
    exit 1
fi

curl --fail --silent --show-error "http://127.0.0.1:${port}/metrics" >"${block_root}/metrics-after.txt"
date --iso-8601=ns >"${block_root}/completed-at.txt"
