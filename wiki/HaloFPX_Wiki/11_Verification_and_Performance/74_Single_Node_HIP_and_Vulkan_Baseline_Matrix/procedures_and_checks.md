---
section_id: "74"
title: "Single-node baseline procedures and checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
  software_versions: []
  hardware_revisions: ["nimo-1", "nimo-2"]
related_sections: ["18", "22", "23", "29", "31", "33", "36", "73", "78", "79"]
---

# Procedures and checks

Commands are non-destructive and unprivileged. Sensor files may require existing ACLs. Replace angle-bracket placeholders from a checked-in run manifest; never improvise values after a trial starts.

## 1. Freeze source and build once

~~~bash
git clone https://github.com/charlie12345/ROCmFPX.git halofpx-baseline
cd halofpx-baseline
git checkout --detach a5605a72768c6562241b248e268e33dc92787394
test -z "$(git status --porcelain)"
GGML_HIP_ROCWMMA_FATTN=OFF CMAKE_HIP_ARCHITECTURES=gfx1151 \
  ./scripts/build-strix-rocmfp4-mtp.sh
sha256sum build-strix-rocmfp4/bin/{llama-bench,llama-server,llama-perplexity}
build-strix-rocmfp4/bin/llama-bench --list-devices
~~~

**[VERIFIED]** The script configures HIP and Vulkan in the same release build and targets gfx1151 [S74-04]. **[RECOMMENDATION]** Use identical binary hashes on both nodes and record compiler, CMake cache, linked libraries, tree, and binary hashes. Keep the script's maintainer-local optional rocWMMA path off unless separately pinned.

## 2. Preflight each node

~~~bash
set -o pipefail
date -u --iso-8601=seconds
hostnamectl
uname -a
cat /proc/cmdline
lscpu --json
free -b
rocminfo
hipconfig --full
vulkaninfo --summary
cat /sys/module/amdgpu/version 2>/dev/null || true
for f in /sys/class/drm/card*/device/uevent /sys/class/drm/card*/device/hwmon/hwmon*/name; do
  printf '\n### %s\n' "$f"; cat "$f"
done
~~~

Capture package versions, BIOS/SMBIOS, firmware hashes, amdgpu logs, governor/EPP, affinity, active services, ambient, sensor labels/units, and named firmware power mode per Sections 18, 22, and 23. Abort comparison if binary/model hashes or controls differ.

## 3. Hash and load-gate artifacts

~~~bash
sha256sum <target.gguf> <draft-or-mtp.gguf-if-used> <prompt-token-file> <quality-corpus>
build-strix-rocmfp4/bin/llama-cli -m <target.gguf> --no-conversation \
  -n 0 -c <context> --device <HIP0-or-Vulkan0> 2>&1 | tee <run>/load.log
~~~

Preserve the loader log and verify architecture, context, quant, K/V allocation, selected device/backend, and absence of unexplained fallback.

## 4. Engine prompt/decode sweep

Use exact names from --list-devices; do not assume them. Run one device at a time.

~~~bash
BIN=build-strix-rocmfp4/bin/llama-bench
MODEL=<absolute-model-path>
DEVICE=<exact-HIP-or-Vulkan-device>

"$BIN" -m "$MODEL" -dev "$DEVICE" -ngl -1 -r 10 -o jsonl \
  -p 512,4096 -n 0 -d 0,4096,32768 \
  -b 512,2048 -ub 128,512 -ctk f16 -ctv f16 -fa off,on \
  > <run>/engine.jsonl 2> <run>/engine.stderr.log

"$BIN" -m "$MODEL" -dev "$DEVICE" -ngl -1 -r 10 -o jsonl \
  -p 0 -n 256 -d 0,4096,32768 \
  -b 512,2048 -ub 128,512 -ctk f16 -ctv f16 -fa off,on \
  > <run>/decode.jsonl 2> <run>/decode.stderr.log
~~~

**[VERIFIED]** These flags exist at the pinned commit; JSONL retains individual repetitions [S74-01]. Split sweeps into randomized thermal blocks. Depth establishes prefilled KV; configured context alone is not a deep-context test. Repeat for each approved quant/KV row without relabeling outputs.

## 5. End-to-end TTFT and ILT

~~~bash
build-strix-rocmfp4/bin/llama-server \
  -m <target.gguf> -dev <exact-device> -ngl all -c <context> \
  -b <batch> -ub <ubatch> -ctk <K-type> -ctv <V-type> \
  -fa <on-or-off> --parallel 1 --metrics --host 127.0.0.1 --port 8080 \
  > <run>/server.log 2>&1 & SERVER_PID=$!

TELEMETRY_PID=""
cleanup_trial() {
  if [ -n "$TELEMETRY_PID" ] && kill -0 "$TELEMETRY_PID" 2>/dev/null; then
    kill -TERM "$TELEMETRY_PID" 2>/dev/null || true
    wait "$TELEMETRY_PID" 2>/dev/null || true
  fi
  if kill -0 "$SERVER_PID" 2>/dev/null; then
    kill -INT "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
  fi
}
trap cleanup_trial EXIT INT TERM

curl --fail http://127.0.0.1:8080/health

(
  while kill -0 "$SERVER_PID" 2>/dev/null; do
    date -u +%Y-%m-%dT%H:%M:%S.%N%z
    grep -E '^(VmRSS|VmHWM|VmSwap):' "/proc/$SERVER_PID/status"
    grep -E '^(Rss|Pss|Private|Swap):' "/proc/$SERVER_PID/smaps_rollup" 2>/dev/null || true
    for h in /sys/class/drm/card*/device/hwmon/hwmon*; do
      grep -H . "$h"/{name,temp*_input,power*_average,power*_input,freq*_input,fan*_input} 2>/dev/null || true
    done
    sleep 1
  done
) > <run>/telemetry.log 2> <run>/telemetry.stderr.log & TELEMETRY_PID=$!

curl --fail http://127.0.0.1:8080/metrics > <run>/metrics-before.txt
~~~

For MTP=none omit speculation. For a source-verified compatible artifact add:

~~~text
--spec-type draft-mtp --spec-draft-model <mtp.gguf> --spec-draft-device <exact-device> --spec-draft-n-max <1|2|4|8>
~~~

Use a pinned streaming client that writes monotonic request start, headers, first content token, every content token, finish, status, token counts, response timings, and raw events. Manual smoke only:

~~~bash
curl --fail-with-body --no-buffer -sS http://127.0.0.1:8080/v1/completions \
  -H 'Content-Type: application/json' \
  -d @<canonical-request-with-stream-true.json> > <run>/stream-smoke.sse
curl --fail http://127.0.0.1:8080/metrics > <run>/metrics-after.txt

kill -INT "$SERVER_PID"
set +e
wait "$SERVER_PID"
SERVER_STATUS=$?
wait "$TELEMETRY_PID"
TELEMETRY_STATUS=$?
set -e
trap - EXIT INT TERM
printf 'server_exit=%s\ntelemetry_exit=%s\n' "$SERVER_STATUS" "$TELEMETRY_STATUS" \
  > <run>/process-status.txt
~~~

**[RECOMMENDATION]** Do not derive TTFT/ILT from curl output or server averages. The Section 73 client must preserve raw timestamps. Compute TTFT per request and ILT over content-token arrivals excluding the first; report p50/p95/p99, count, and confidence/percentile method.

## 6. Memory, power, and thermal capture

**[RECOMMENDATION]** Start the fixed-cadence telemetry collector after readiness and before the first measured request, as shown above. Preserve collector stderr and both process exit statuses. An unexpected server or collector exit invalidates the trial; do not erase it with `|| true`. The cleanup trap is only an interruption fallback and does not replace the explicit recorded shutdown path.

Capture wall Wh/W with an independently calibrated logger where available. AMD SMI is an additional capability-dependent query path [S74-09]. **[VERIFIED]** APU amdgpu power may include CPU power; do not add overlapping domains [S74-08]. Missing sensors are missing, not zero. Apply Section 22 burst/warm-up/steady definitions and align throughput, ambient, clocks, power, and throttling.

## 7. Quality and correctness

~~~bash
build-strix-rocmfp4/bin/llama-perplexity -m <model.gguf> -f <pinned-wikitext2-file> \
  -dev <exact-device> -ctk <K-type> -ctv <V-type> -fa <on-or-off> \
  > <run>/perplexity.txt 2>&1
~~~

**[VERIFIED]** Wikitext-2 is upstream convention but results are implementation-dependent [S74-03]. Also run Sections 31/78 deterministic token/logit and task gates. Compare HIP/Vulkan greedy token IDs identically; triage divergence against approved tolerance. For MTP record generated/accepted drafts and quality; speed alone is insufficient.

## 8. Evidence closeout

Hash raw files, record exit statuses/invalidations, make the bundle read-only, and link it from the experiment ledger. Review source/build/model equality, requested versus observed backend/FA/MTP, samples, quality, steady state, and missing telemetry before MEASURED_CANDIDATE.

[S74-01]: sources.md#s74-01
[S74-03]: sources.md#s74-03
[S74-04]: sources.md#s74-04
[S74-08]: sources.md#s74-08
[S74-09]: sources.md#s74-09
