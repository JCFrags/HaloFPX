# P15 ROCmFPX MMVQ sum-free activation candidate

Status: **implemented behind a default-off HIP build option; target
qualification and performance result remain open**

This slice implements the first model-general generation optimization from the
2026-08-12 performance work plan. It removes one Q8_1 activation-sum warp
reduction from the HIP MMVQ path only when the weight type is exactly
`Q2_0_ROCMFPX`, `Q3_0_ROCMFPX`, `Q6_0_ROCMFPX`, or `Q8_0_ROCMFPX`.
It does not change a GGUF type, weight bytes, weight quantization recipe, or
on-disk format. Only the unused high lane of transient Q8_1 activation blocks
changes.

## Static correctness boundary

The four admitted MMVQ consumers use the Q8_1 low scale lane and all 32 signed
activation bytes. They do not use the high sum lane. The implementation
therefore preserves the 36-byte `block_q8_1` ABI, its scale, and every `qs`
byte, while writing positive half zero to the unused high lane. The existing
sum-producing kernel remains selected for ROCmFP4, ROCmFP4 FAST, every stock
quant, CUDA builds, and all HIP builds with the option disabled.

The exact whitelist and build dispatch are in
`ggml/src/ggml-cuda/rocmfpx-mmvq-sum-free.h`. The shared activation quantizer
is in `ggml/src/ggml-cuda/quantize.cu`. This is an MMVQ/small-batch generation
candidate; the separate prompt MMQ activation converter already uses a
sum-free D4 layout for applicable ROCmFP paths.

The option is deliberately default-off pending physical-target qualification:

```text
GGML_HIP_ROCMFPX_MMVQ_SUM_FREE=OFF  # legacy control and repository default
GGML_HIP_ROCMFPX_MMVQ_SUM_FREE=ON   # candidate
```

That choice follows the project rule that new behavior stays off until its
correctness and rollback gates pass. A later evidence-backed decision may
change the Strix build default; this patch does not make that promotion.

## Host and CI qualification

`test-halofpx-rocmfpx-mmvq-sum-free-off` and
`test-halofpx-rocmfpx-mmvq-sum-free-on` compile the dispatch in both modes.
They exhaustively check the `ggml_type` whitelist, exclude ROCmFP4 by bounded
candidate scope and all stock types (including affine formats that consume the
sum), bind the Q8_1 ABI to 36 bytes, and check that the unused lane is
deterministic positive zero. These
host tests prove dispatch and layout policy; they do not compile the HIP
kernel or establish target speed.

## CachyOS Strix Halo A/B recipe

Run the retained recipe separately on nimo-1 and nimo-2 with an ordinary
Q2/Q3/Q6/Q8 ROCmFPX model that fits one machine. MiniMax remains a later large
stress fixture, not the optimization target. The recipe refuses a non-CachyOS
host, a dirty source tree, a model hash mismatch, a nonempty evidence
directory, or an active llama process. Before admitting a full-model run, it
uses the repository-pinned GGUF reader to retain a tensor-type census and fails
closed unless at least one tensor has an exact admitted ROCmFPX type.

```bash
git checkout <exact-candidate-commit>

MODEL=/absolute/path/model-Q6_0_ROCMFPX.gguf \
MODEL_SHA256=<exact-64-digit-sha256> \
OUT_DIR=/var/tmp/halofpx-p15-$(hostname)-<exact-candidate-commit> \
scripts/qualify-rocmfpx-mmvq-sum-free.sh
```

If no ordinary single-node model is available, run the explicitly narrower
target screen instead:

```bash
MICROBENCH_ONLY=1 \
OUT_DIR=/var/tmp/halofpx-p15-microbench-$(hostname)-<exact-candidate-commit> \
scripts/qualify-rocmfpx-mmvq-sum-free.sh
```

That mode does not accept a model or make a model-level claim. It still records
the CachyOS/ROCm/hardware tuple, builds both modes, proves the HIP compile
definition is absent/present in the actual `quantize.cu` commands, runs host
contracts, dense correctness at n=1/2/8, the n=9 MMQ boundary, routed
`MUL_MAT_ID` at n=1/32, excluded-format controls, and counterbalanced n=1
microbench processes.

The script builds the legacy and candidate binaries from the same commit,
verifies the CMake control in each cache and actual HIP compile commands,
records binary and resolved shared-library provenance plus the exact
CachyOS/ROCm/compiler/hardware tuple, runs the focused host contracts, compares
all four formats against the CPU backend operation reference, requires
byte-exact greedy completion output in full-model mode, records a decode-shape
microbenchmark, and runs the full model in counterbalanced process order:

```text
legacy -> candidate -> candidate -> legacy
```

Each `llama-bench` process retains its normal warmup and three raw repetitions
by default. The raw manifest records prompt tokens, generation tokens, batch,
microbatch, threads, polling, device, model digest, commit, build control, run
order, and SHA-256 for every retained artifact. It labels the result
`raw-not-promoted`; the script does not decide that a gain exists.

## Promotion and kill gates

Promotion requires both target nodes to pass:

1. both HIP builds and both host dispatch modes;
2. CPU-reference backend-operation correctness for all four admitted formats;
3. byte-exact fixed-seed greedy completion parity;
4. no material prompt-processing regression in matched full-model results;
5. a repeatable generation or MMVQ latency improvement across the
   counterbalanced samples; and
6. retained raw records from the exact commit, model hash, CachyOS/ROCm tuple,
   and binary hashes.

Any correctness mismatch kills the candidate. A noise-scale or one-machine-only
timing result leaves it default-off and records a no-go or follow-up screen; it
must not be promoted as a measured gain. This plan follows Wiki Sections 37,
73, and 78 and the current `project/PERFORMANCE_WORKPLAN.md` separation of
prompt and generation claims.
