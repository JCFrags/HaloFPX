#!/usr/bin/env bash
# Build and qualify the legacy and sum-free ROCmFPX MMVQ activation paths on
# one CachyOS Strix Halo target. Run independently on both target nodes.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
MODEL="${MODEL:-}"
MODEL_SHA256="${MODEL_SHA256:-}"
MICROBENCH_ONLY="${MICROBENCH_ONLY:-0}"
OUT_DIR="${OUT_DIR:-/var/tmp/halofpx-rocmfpx-mmvq-sum-free-$(date -u +%Y%m%dT%H%M%SZ)}"
JOBS="${JOBS:-$(nproc)}"
DEVICE="${DEVICE:-ROCm0}"
PROMPT_TOKENS="${PROMPT_TOKENS:-512}"
GEN_TOKENS="${GEN_TOKENS:-128}"
REPEAT="${REPEAT:-3}"
BATCH_SIZE="${BATCH_SIZE:-512}"
UBATCH_SIZE="${UBATCH_SIZE:-512}"
THREADS="${THREADS:-16}"
POLL="${POLL:-50}"
N_GPU_LAYERS="${N_GPU_LAYERS:-999}"
CONTEXT_SIZE="${CONTEXT_SIZE:-4096}"
CACHE_TYPE_K="${CACHE_TYPE_K:-q8_0}"
CACHE_TYPE_V="${CACHE_TYPE_V:-q8_0}"
FLASH_ATTN="${FLASH_ATTN:-on}"
CORRECTNESS_TOKENS="${CORRECTNESS_TOKENS:-64}"
CORRECTNESS_PROMPT="${CORRECTNESS_PROMPT:-Continue this deterministic sequence with short lines: one, two, three, four, five.}"
HSA_GFX_VERSION="${HSA_OVERRIDE_GFX_VERSION:-11.5.1}"
HIP_UNIFIED_MEMORY="${GGML_HIP_ENABLE_UNIFIED_MEMORY:-1}"

fail() {
    echo "error: $*" >&2
    exit 1
}

PROCESS_GUARD="$SCRIPT_DIR/halofpx-rocmfpx-mmvq-process-guard.sh"
[[ -r "$PROCESS_GUARD" ]] || fail "missing process guard: $PROCESS_GUARD"
# shellcheck source=halofpx-rocmfpx-mmvq-process-guard.sh
source "$PROCESS_GUARD"

require_positive_integer() {
    local name="$1"
    local value="$2"
    [[ "$value" =~ ^[1-9][0-9]*$ ]] || fail "$name must be a positive integer (got $value)"
}

[[ "$MICROBENCH_ONLY" == 0 || "$MICROBENCH_ONLY" == 1 ]] ||
    fail "MICROBENCH_ONLY must be 0 or 1"
for pair in \
    "JOBS:$JOBS" "PROMPT_TOKENS:$PROMPT_TOKENS" "GEN_TOKENS:$GEN_TOKENS" \
    "REPEAT:$REPEAT" "BATCH_SIZE:$BATCH_SIZE" "UBATCH_SIZE:$UBATCH_SIZE" \
    "THREADS:$THREADS" "POLL:$POLL" "N_GPU_LAYERS:$N_GPU_LAYERS" \
    "CONTEXT_SIZE:$CONTEXT_SIZE" "CORRECTNESS_TOKENS:$CORRECTNESS_TOKENS"; do
    require_positive_integer "${pair%%:*}" "${pair#*:}"
done
[[ "$HIP_UNIFIED_MEMORY" == 1 ]] ||
    fail "GGML_HIP_ENABLE_UNIFIED_MEMORY must be 1 (the runtime treats any set value as enabled)"
[[ "$FLASH_ATTN" == on || "$FLASH_ATTN" == off || "$FLASH_ATTN" == auto ]] ||
    fail "FLASH_ATTN must be on, off, or auto"
[[ "${CMAKE_HIP_FLAGS:-}${CXXFLAGS:-}${CPPFLAGS:-}" != *GGML_HIP_ROCMFPX_MMVQ_SUM_FREE* ]] ||
    fail "do not inject the A/B control through compiler flags"
[[ -r /etc/os-release ]] || fail "missing /etc/os-release"

# shellcheck disable=SC1091
source /etc/os-release
[[ "${ID:-}" == cachyos ]] || fail "target qualification requires CachyOS (found ${ID:-unknown})"

ACTIVE_LLAMA_PROCESSES="$(halofpx_rocmfpx_mmvq_active_processes)"
if [[ -n "$ACTIVE_LLAMA_PROCESSES" ]]; then
    printf '%s\n' "$ACTIVE_LLAMA_PROCESSES" >&2
    fail "refusing to benchmark while another llama process is active"
fi

[[ -z "$(git -C "$ROOT" status --porcelain)" ]] || fail "source worktree must be clean"
SOURCE_COMMIT="$(git -C "$ROOT" rev-parse HEAD)"
ACTUAL_MODEL_SHA256=""
if [[ "$MICROBENCH_ONLY" == 0 ]]; then
    [[ -n "$MODEL" ]] || fail "MODEL must name an ordinary ROCmFPX GGUF that fits one target"
    [[ -f "$MODEL" ]] || fail "MODEL is not a regular file: $MODEL"
    [[ "$MODEL_SHA256" =~ ^[[:xdigit:]]{64}$ ]] || fail "MODEL_SHA256 must be an exact 64-digit digest"
    ACTUAL_MODEL_SHA256="$(sha256sum "$MODEL" | awk '{print $1}')"
    [[ "${ACTUAL_MODEL_SHA256,,}" == "${MODEL_SHA256,,}" ]] ||
        fail "model digest mismatch: expected $MODEL_SHA256 got $ACTUAL_MODEL_SHA256"
fi

if [[ -d "$OUT_DIR" ]] && [[ -n "$(find "$OUT_DIR" -mindepth 1 -maxdepth 1 -print -quit)" ]]; then
    fail "OUT_DIR already contains files: $OUT_DIR"
fi
mkdir -p "$OUT_DIR/system" "$OUT_DIR/build" "$OUT_DIR/correctness" "$OUT_DIR/microbench" "$OUT_DIR/model-bench"
OUT_DIR="$(cd "$OUT_DIR" && pwd -P)"
STARTED_UTC="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
RUN_STAMP="$(date -u +%Y%m%dT%H%M%SZ)-$$"
BUILD_ROOT="${BUILD_ROOT:-/var/tmp/halofpx-p15-build-${SOURCE_COMMIT:0:12}-$RUN_STAMP}"
[[ ! -e "$BUILD_ROOT" ]] || fail "BUILD_ROOT already exists: $BUILD_ROOT"
mkdir -p "$BUILD_ROOT"
BUILD_ROOT="$(cd "$BUILD_ROOT" && pwd -P)"
HOST_NAME="$(hostname)"
QUALIFICATION_MODE="full-model"
[[ "$MICROBENCH_ONLY" == 0 ]] || QUALIFICATION_MODE="microbench-only"
CURRENT_GATE="system-capture"

finalize() {
    local rc=$?
    trap - EXIT
    set +e
    local finished_utc outcome
    finished_utc="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    outcome="failed"
    [[ $rc -eq 0 ]] && outcome="pass"
    printf '%s\n' \
        "outcome=$outcome" \
        "exit_code=$rc" \
        "last_gate=$CURRENT_GATE" \
        "finished_utc=$finished_utc" \
        >"$OUT_DIR/terminal-status.txt"
    export SOURCE_COMMIT ACTUAL_MODEL_SHA256 MODEL HOST_NAME STARTED_UTC
    export DEVICE PROMPT_TOKENS GEN_TOKENS REPEAT BATCH_SIZE UBATCH_SIZE THREADS POLL
    export N_GPU_LAYERS CONTEXT_SIZE CACHE_TYPE_K CACHE_TYPE_V FLASH_ATTN CORRECTNESS_TOKENS
    export HSA_GFX_VERSION HIP_UNIFIED_MEMORY MICROBENCH_ONLY QUALIFICATION_MODE BUILD_ROOT
    export CURRENT_GATE rc outcome finished_utc
    python3 - "$OUT_DIR" <<'PY'
import hashlib
import json
import os
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
artifacts = {}
for path in sorted(root.rglob("*")):
    if path.is_file() and path.name not in {"manifest.json", "SHA256SUMS"}:
        digest = hashlib.sha256()
        with path.open("rb") as source:
            for chunk in iter(lambda: source.read(1024 * 1024), b""):
                digest.update(chunk)
        artifacts[path.relative_to(root).as_posix()] = digest.hexdigest()

microbench_only = os.environ["MICROBENCH_ONLY"] == "1"
manifest = {
    "schema": "halofpx.rocmfpx-mmvq-sum-free-screen.v2",
    "evidence_status": "raw-not-promoted",
    "terminal_outcome": os.environ["outcome"],
    "exit_code": int(os.environ["rc"]),
    "last_gate": os.environ["CURRENT_GATE"],
    "qualification_mode": os.environ["QUALIFICATION_MODE"],
    "source_commit": os.environ["SOURCE_COMMIT"],
    "source_tree_clean_at_start": True,
    "host": os.environ["HOST_NAME"],
    "build_root": os.environ["BUILD_ROOT"],
    "model": None if microbench_only else {
        "path": os.environ["MODEL"],
        "sha256": os.environ["ACTUAL_MODEL_SHA256"],
        "admission": "see system/model-tensor-census.json",
    },
    "build_modes": {
        "legacy": {"GGML_HIP_ROCMFPX_MMVQ_SUM_FREE": False},
        "candidate": {"GGML_HIP_ROCMFPX_MMVQ_SUM_FREE": True},
    },
    "environment": {
        "device": os.environ["DEVICE"],
        "hsa_override_gfx_version": os.environ["HSA_GFX_VERSION"],
        "ggml_hip_enable_unified_memory": int(os.environ["HIP_UNIFIED_MEMORY"]),
    },
    "benchmark": {
        "prompt_tokens": int(os.environ["PROMPT_TOKENS"]),
        "generation_tokens": int(os.environ["GEN_TOKENS"]),
        "repetitions_per_process": int(os.environ["REPEAT"]),
        "process_order": ["legacy", "candidate", "candidate", "legacy"],
        "batch": int(os.environ["BATCH_SIZE"]),
        "ubatch": int(os.environ["UBATCH_SIZE"]),
        "threads": int(os.environ["THREADS"]),
        "poll": int(os.environ["POLL"]),
        "n_gpu_layers": int(os.environ["N_GPU_LAYERS"]),
        "context_size": int(os.environ["CONTEXT_SIZE"]),
        "cache_type_k": os.environ["CACHE_TYPE_K"],
        "cache_type_v": os.environ["CACHE_TYPE_V"],
        "flash_attention": os.environ["FLASH_ATTN"],
        "warmup": "each test-backend-ops/llama-bench process retains its built-in warmup",
    },
    "correctness": {
        "backend_reference": "test-backend-ops CPU comparison",
        "dense_tokens": [1, 2, 8],
        "mmq_boundary_tokens": [9],
        "routed_tokens": [1, 32],
        "completion_tokens": int(os.environ["CORRECTNESS_TOKENS"]),
        "completion_parity": "byte-exact greedy output; full-model mode only",
    },
    "started_utc": os.environ["STARTED_UTC"],
    "finished_utc": os.environ["finished_utc"],
    "artifacts_sha256": artifacts,
}
(root / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY
    (
        cd "$OUT_DIR" || exit 1
        find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum >SHA256SUMS
    )
    exit "$rc"
}
trap finalize EXIT

uname -a >"$OUT_DIR/system/uname.txt"
cp /etc/os-release "$OUT_DIR/system/os-release.txt"
hostnamectl >"$OUT_DIR/system/hostnamectl.txt" 2>&1 || true
lscpu >"$OUT_DIR/system/lscpu.txt" 2>&1 || true
command -v pacman >/dev/null 2>&1 && pacman -Q >"$OUT_DIR/system/pacman-packages.txt"
command -v hipcc >/dev/null 2>&1 && hipcc --version >"$OUT_DIR/system/hipcc-version.txt" 2>&1
command -v rocminfo >/dev/null 2>&1 && rocminfo >"$OUT_DIR/system/rocminfo.txt" 2>&1 || true
command -v amd-smi >/dev/null 2>&1 && amd-smi static --json >"$OUT_DIR/system/amd-smi-static.json" 2>&1 || true
cmake --version >"$OUT_DIR/system/cmake-version.txt"
python3 --version >"$OUT_DIR/system/python-version.txt" 2>&1

MODEL_SIZE=0
[[ "$MICROBENCH_ONLY" == 1 ]] || MODEL_SIZE="$(stat -c %s "$MODEL")"
printf '%s\n' \
    "source_commit=$SOURCE_COMMIT" "source_tree_clean=true" "host=$HOST_NAME" \
    "os_id=${ID}" "os_pretty_name=${PRETTY_NAME:-}" "qualification_mode=$QUALIFICATION_MODE" \
    "model=$MODEL" "model_size=$MODEL_SIZE" "model_sha256=$ACTUAL_MODEL_SHA256" \
    "device=$DEVICE" "prompt_tokens=$PROMPT_TOKENS" "generation_tokens=$GEN_TOKENS" \
    "repetitions_per_process=$REPEAT" "process_order=legacy,candidate,candidate,legacy" \
    "batch=$BATCH_SIZE" "ubatch=$UBATCH_SIZE" "threads=$THREADS" "poll=$POLL" \
    "n_gpu_layers=$N_GPU_LAYERS" "context_size=$CONTEXT_SIZE" \
    "cache_type_k=$CACHE_TYPE_K" "cache_type_v=$CACHE_TYPE_V" "flash_attention=$FLASH_ATTN" \
    "correctness_tokens=$CORRECTNESS_TOKENS" "correctness_prompt=$CORRECTNESS_PROMPT" \
    "hsa_override_gfx_version=$HSA_GFX_VERSION" \
    "ggml_hip_enable_unified_memory=$HIP_UNIFIED_MEMORY" \
    "legacy_option=OFF" "candidate_option=ON" "build_root=$BUILD_ROOT" \
    "started_utc=$STARTED_UTC" >"$OUT_DIR/run-config.txt"

if [[ "$MICROBENCH_ONLY" == 0 ]]; then
    CURRENT_GATE="gguf-tensor-admission"
    PYTHONPATH="$ROOT/gguf-py${PYTHONPATH:+:$PYTHONPATH}" \
    python3 - "$MODEL" "$OUT_DIR/system/model-tensor-census.json" "$ACTUAL_MODEL_SHA256" "$ROOT/gguf-py" <<'PY'
import hashlib
import json
import os
from collections import Counter
from pathlib import Path
import sys

model_arg, output_arg, model_sha256, parser_root_arg = sys.argv[1:5]
output = Path(output_arg)
accepted = ["Q2_0_ROCMFPX", "Q3_0_ROCMFPX", "Q6_0_ROCMFPX", "Q8_0_ROCMFPX"]
report = {
    "schema": "halofpx.gguf-target-tensor-census.v1",
    "status": "fail",
    "admission": {"rule": "at least one exact admitted ROCmFPX tensor", "accepted_tensor_types": accepted},
    "model": {"requested_path": model_arg, "sha256": model_sha256.lower()},
}
rc = 2
try:
    parser_root = Path(parser_root_arg).resolve(strict=True)
    model = Path(model_arg).resolve(strict=True)
    import gguf
    from gguf import GGMLQuantizationType, GGUFReader
    from gguf import constants as constants
    package_file = Path(gguf.__file__).resolve(strict=True)
    if parser_root != package_file and parser_root not in package_file.parents:
        raise RuntimeError(f"loaded parser outside bundled root: {package_file}")
    expected = {
        "Q2_0_ROCMFPX": (107, 32, 10), "Q3_0_ROCMFPX": (104, 32, 14),
        "Q6_0_ROCMFPX": (102, 32, 26), "Q8_0_ROCMFPX": (103, 32, 33),
    }
    admitted = set()
    parser_types = {}
    for name, (enum_id, block, size) in expected.items():
        member = GGMLQuantizationType.__members__.get(name)
        if member is None or int(member) != enum_id or constants.GGML_QUANT_SIZES.get(member) != (block, size):
            raise RuntimeError(f"bundled parser does not exactly support {name}")
        admitted.add(member)
        parser_types[name] = {"enum_id": enum_id, "block_elements": block, "type_bytes": size}
    reader = GGUFReader(model, "r")
    counts = Counter(t.tensor_type.name for t in reader.tensors)
    target_tensors = [{
        "name": t.name, "tensor_type": t.tensor_type.name,
        "dimensions": [int(v) for v in t.shape], "element_count": int(t.n_elements),
        "storage_bytes": int(t.n_bytes),
    } for t in reader.tensors if t.tensor_type in admitted]
    report.update({
        "parser": {"root": str(parser_root), "package": str(package_file),
                   "package_sha256": hashlib.sha256(package_file.read_bytes()).hexdigest(),
                   "target_types": parser_types, "python": sys.version},
        "model": {**report["model"], "resolved_path": str(model), "size_bytes": model.stat().st_size,
                  "tensor_count": len(reader.tensors)},
        "census": {"all_tensor_type_counts": dict(sorted(counts.items())),
                   "target_tensor_type_counts": {name: counts.get(name, 0) for name in accepted},
                   "target_tensor_count": len(target_tensors), "target_tensors": target_tensors},
    })
    if target_tensors:
        report["status"] = "pass"
        report["admitted"] = True
        rc = 0
    else:
        report["error"] = {"code": "no-admitted-target-tensors",
                           "message": "model contains no exact admitted ROCmFPX tensor"}
        rc = 3
except Exception as exc:
    report.setdefault("error", {"code": "gguf-census-failed", "type": type(exc).__name__, "message": str(exc)})
temporary = output.with_name(output.name + ".tmp")
temporary.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
os.replace(temporary, output)
raise SystemExit(rc)
PY
else
    printf '%s\n' \
        "MICROBENCH_ONLY=1: no GGUF was admitted and no model-level claim may be made." \
        >"$OUT_DIR/system/model-census-not-applicable.txt"
fi

verify_compile_control() {
    local mode="$1" expected="$2" build_dir="$3"
    python3 - "$build_dir/compile_commands.json" "$OUT_DIR/build/$mode-hip-compile-control.json" "$expected" <<'PY'
import json
from pathlib import Path
import sys

source, output, expected = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3] == "ON"
entries = json.loads(source.read_text(encoding="utf-8"))
quantize = [entry for entry in entries if Path(entry["file"]).name == "quantize.cu"]
rows = []
for entry in quantize:
    command = entry.get("command", " ".join(entry.get("arguments", [])))
    rows.append({"file": entry["file"], "directory": entry["directory"],
                 "definition_present": "GGML_HIP_ROCMFPX_MMVQ_SUM_FREE" in command})
passed = bool(rows) and all(row["definition_present"] == expected for row in rows)
output.write_text(json.dumps({"schema": "halofpx.hip-compile-control.v1", "expected": expected,
                              "passed": passed, "quantize_compile_entries": rows},
                             indent=2, sort_keys=True) + "\n", encoding="utf-8")
raise SystemExit(0 if passed else 1)
PY
}

record_build_provenance() {
    local mode="$1" build_dir="$2"
    local provenance="$OUT_DIR/build/$mode-provenance"
    mkdir -p "$provenance"
    find "$build_dir" -type f \( -name 'libggml*.so*' -o -name 'libllama*.so*' \) -print0 \
        | sort -z | xargs -0 -r sha256sum >"$provenance/dso-files.sha256"
    local binary
    for binary in test-backend-ops llama-bench llama-completion; do
        [[ -x "$build_dir/bin/$binary" ]] || continue
        readlink -f "$build_dir/bin/$binary" >>"$provenance/binary-realpaths.txt"
        ldd "$build_dir/bin/$binary" >"$provenance/$binary.ldd.txt" 2>&1
    done
}

build_mode() {
    local mode="$1" enabled="$2" build_dir="$BUILD_ROOT/$mode"
    local targets=(test-backend-ops
        test-halofpx-rocmfpx-mmvq-sum-free-off
        test-halofpx-rocmfpx-mmvq-sum-free-on)
    if [[ "$MICROBENCH_ONLY" == 0 ]]; then
        targets+=(llama-bench llama-completion)
    fi
    CURRENT_GATE="build-$mode"
    echo "building $mode (GGML_HIP_ROCMFPX_MMVQ_SUM_FREE=$enabled)"
    BUILD_DIR="$build_dir" JOBS="$JOBS" CMAKE_EXPORT_COMPILE_COMMANDS=ON \
        GGML_HIP_ROCMFPX_MMVQ_SUM_FREE="$enabled" \
        "$SCRIPT_DIR/build-strix-rocmfp4-mtp.sh" "${targets[@]}" \
        >"$OUT_DIR/build/$mode-build.log" 2>&1
    grep -F "GGML_HIP_ROCMFPX_MMVQ_SUM_FREE:BOOL=$enabled" "$build_dir/CMakeCache.txt" \
        >"$OUT_DIR/build/$mode-option.txt" || fail "$mode CMake option was not applied"
    cp "$build_dir/CMakeCache.txt" "$OUT_DIR/build/$mode-CMakeCache.txt"
    cp "$build_dir/compile_commands.json" "$OUT_DIR/build/$mode-compile_commands.json"
    verify_compile_control "$mode" "$enabled" "$build_dir"
    find "$build_dir/bin" -maxdepth 1 -type f -executable -print0 | sort -z | xargs -0 -r sha256sum \
        >"$OUT_DIR/build/$mode-binaries.sha256"
    record_build_provenance "$mode" "$build_dir"
}

build_mode legacy OFF
build_mode candidate ON
LEGACY_BUILD="$BUILD_ROOT/legacy"
CANDIDATE_BUILD="$BUILD_ROOT/candidate"

CURRENT_GATE="host-contracts"
ctest --test-dir "$LEGACY_BUILD" --output-on-failure \
    -R '^test-halofpx-rocmfpx-mmvq-sum-free-(off|on)$' \
    >"$OUT_DIR/correctness/host-contracts.log" 2>&1

DENSE_RE='^type_a=(q2_0_rocmfpx|q3_0_rocmfpx|q6_0_rocmfpx|q8_0_rocmfpx),type_b=f32,m=16,n=(1|2|8),k=256,bs=\[1,1\],nr=\[1,1\],per=\[0,1,2,3\],k_v=0,o=1$'
BOUNDARY_RE='^type_a=(q2_0_rocmfpx|q3_0_rocmfpx|q6_0_rocmfpx|q8_0_rocmfpx),type_b=f32,m=16,n=9,k=256,bs=\[1,1\],nr=\[1,1\],per=\[0,1,2,3\],k_v=0,o=1$'
ROUTED_RE='^type_a=(q2_0_rocmfpx|q3_0_rocmfpx|q6_0_rocmfpx|q8_0_rocmfpx),type_b=f32,n_mats=4,n_used=2,b=0,m=512,n=(1|32),k=256$'
NEGATIVE_RE='^type_a=(q4_0_rocmfp4|q4_0_rocmfp4_fast|q4_1),type_b=f32,m=16,n=(1|2|8),k=256,bs=\[1,1\],nr=\[1,1\],per=\[0,1,2,3\],k_v=0,o=1$'
PERF_RE='^type_a=(q2_0_rocmfpx|q3_0_rocmfpx|q6_0_rocmfpx|q8_0_rocmfpx),type_b=f32,m=4096,n=1,k=14336,bs=\[1,1\],nr=\[1,1\],per=\[0,1,2,3\],k_v=0,o=1$'

run_backend_test() {
    local mode="$1" build_dir="$2" label="$3" op="$4" regex="$5" expected="$6"
    local output="$OUT_DIR/correctness/$mode-$label.log"
    CURRENT_GATE="correctness-$mode-$label"
    env HSA_OVERRIDE_GFX_VERSION="$HSA_GFX_VERSION" GGML_HIP_ENABLE_UNIFIED_MEMORY="$HIP_UNIFIED_MEMORY" \
        "$build_dir/bin/test-backend-ops" test -b "$DEVICE" -o "$op" -p "$regex" >"$output" 2>&1
    grep -Fq "  $expected/$expected tests passed" "$output" ||
        fail "$mode $label did not run the exact non-vacuous $expected-case roster"
}

for spec in "legacy:$LEGACY_BUILD" "candidate:$CANDIDATE_BUILD"; do
    mode="${spec%%:*}"
    build_dir="${spec#*:}"
    run_backend_test "$mode" "$build_dir" dense-mmvq MUL_MAT "$DENSE_RE" 16
    run_backend_test "$mode" "$build_dir" n9-mmq-boundary MUL_MAT "$BOUNDARY_RE" 4
    run_backend_test "$mode" "$build_dir" routed-mmvq-mmq MUL_MAT_ID "$ROUTED_RE" 8
done
run_backend_test candidate "$CANDIDATE_BUILD" excluded-legacy-consumers MUL_MAT "$NEGATIVE_RE" 13

run_microbench() {
    local mode="$1" ordinal="$2" build_dir="$3"
    local output="$OUT_DIR/microbench/$ordinal-$mode.sql"
    CURRENT_GATE="microbench-$ordinal-$mode"
    env HSA_OVERRIDE_GFX_VERSION="$HSA_GFX_VERSION" GGML_HIP_ENABLE_UNIFIED_MEMORY="$HIP_UNIFIED_MEMORY" \
        "$build_dir/bin/test-backend-ops" perf -b "$DEVICE" -o MUL_MAT -p "$PERF_RE" --output sql \
        >"$output" 2>"$OUT_DIR/microbench/$ordinal-$mode.stderr.log"
    local type
    for type in q2_0_rocmfpx q3_0_rocmfpx q6_0_rocmfpx q8_0_rocmfpx; do
        grep -Fq "$type" "$output" || fail "$ordinal-$mode microbench omitted $type"
    done
}

# Separate processes in counterbalanced order; all raw n=1 results are kept.
run_microbench legacy    1 "$LEGACY_BUILD"
run_microbench candidate 2 "$CANDIDATE_BUILD"
run_microbench candidate 3 "$CANDIDATE_BUILD"
run_microbench legacy    4 "$LEGACY_BUILD"

if [[ "$MICROBENCH_ONLY" == 0 ]]; then
    run_completion() {
        local mode="$1" build_dir="$2"
        CURRENT_GATE="completion-$mode"
        env HSA_OVERRIDE_GFX_VERSION="$HSA_GFX_VERSION" GGML_HIP_ENABLE_UNIFIED_MEMORY="$HIP_UNIFIED_MEMORY" \
            "$build_dir/bin/llama-completion" -m "$MODEL" -dev "$DEVICE" -ngl "$N_GPU_LAYERS" \
            -fa "$FLASH_ATTN" -ctk "$CACHE_TYPE_K" -ctv "$CACHE_TYPE_V" -c "$CONTEXT_SIZE" \
            -b "$BATCH_SIZE" -ub "$UBATCH_SIZE" -t "$THREADS" --poll "$POLL" --temp 0 --seed 1234 \
            --no-display-prompt --simple-io --no-warmup --no-perf --log-disable -no-cnv \
            -n "$CORRECTNESS_TOKENS" -p "$CORRECTNESS_PROMPT" \
            >"$OUT_DIR/correctness/$mode-completion.txt" \
            2>"$OUT_DIR/correctness/$mode-completion.stderr.log"
    }
    run_completion legacy "$LEGACY_BUILD"
    run_completion candidate "$CANDIDATE_BUILD"
    CURRENT_GATE="completion-parity"
    cmp "$OUT_DIR/correctness/legacy-completion.txt" "$OUT_DIR/correctness/candidate-completion.txt" \
        >"$OUT_DIR/correctness/completion-parity.log"
    sha256sum "$OUT_DIR/correctness/legacy-completion.txt" "$OUT_DIR/correctness/candidate-completion.txt" \
        >"$OUT_DIR/correctness/completion-output.sha256"

    run_model_bench() {
        local mode="$1" ordinal="$2" build_dir="$3"
        CURRENT_GATE="model-bench-$ordinal-$mode"
        env HSA_OVERRIDE_GFX_VERSION="$HSA_GFX_VERSION" GGML_HIP_ENABLE_UNIFIED_MEMORY="$HIP_UNIFIED_MEMORY" \
            "$build_dir/bin/llama-bench" -m "$MODEL" -dev "$DEVICE" -ngl "$N_GPU_LAYERS" \
            -fa "$FLASH_ATTN" -ctk "$CACHE_TYPE_K" -ctv "$CACHE_TYPE_V" \
            -b "$BATCH_SIZE" -ub "$UBATCH_SIZE" -t "$THREADS" --poll "$POLL" \
            -p "$PROMPT_TOKENS" -n "$GEN_TOKENS" -r "$REPEAT" -o json \
            >"$OUT_DIR/model-bench/$ordinal-$mode.json" \
            2>"$OUT_DIR/model-bench/$ordinal-$mode.stderr.log"
    }
    run_model_bench legacy    1 "$LEGACY_BUILD"
    run_model_bench candidate 2 "$CANDIDATE_BUILD"
    run_model_bench candidate 3 "$CANDIDATE_BUILD"
    run_model_bench legacy    4 "$LEGACY_BUILD"
fi

CURRENT_GATE="complete"
echo "qualification completed without promoting a performance claim"
echo "mode: $QUALIFICATION_MODE"
echo "raw evidence: $OUT_DIR"
