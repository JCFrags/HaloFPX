---
section_id: "13"
title: "ROCmFPX Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: []
  hardware_revisions: ["Two matched HaloFPX Strix Halo nodes"]
related_sections: ["15", "16", "23", "31", "36", "37", "73", "74", "78"]
---

# Procedures and checks

All commands are non-root unless marked. Replace paths explicitly; do not store models or large logs in the wiki.

<a id="s13-refresh"></a>
## Internet/source refresh

Prerequisites: Git and network access.

```bash
git ls-remote https://github.com/charlie12345/ROCmFPX.git refs/heads/main
git ls-remote https://github.com/ggml-org/llama.cpp.git refs/heads/master
git clone --filter=blob:none https://github.com/charlie12345/ROCmFPX.git ROCmFPX
git -C ROCmFPX checkout --detach a5605a72768c6562241b248e268e33dc92787394
git -C ROCmFPX status --porcelain
```

Expected: the pinned hash checks out and status is empty. Record a changed remote head as a freshness trigger; do not silently update this section.

<a id="s13-static-guard"></a>
## Static inventory guard

```bash
cd ROCmFPX
grep -n 'GGML_TYPE_.*ROCMFP\|GGML_TYPE_TURBO' ggml/include/ggml.h
grep -n 'Q._0_ROCMFP\|ROCMFPX_AGENT\|STRIX' tools/quantize/quantize.cpp
grep -R -n 'Q2_0_ROCMFPX' ggml/src/ggml-vulkan || true
grep -R -n 'Q2_0_ROCMFPX' ggml/src/ggml-cuda ggml/src/ggml-cpu
```

**[VERIFIED]** At the pinned head the Vulkan Q2 grep is empty while CPU/HIP greps are non-empty [S13-05]. A later non-empty result requires inventory review, not automatic promotion.

**[VERIFIED current-source reconciliation]** The `ggml-cuda` sources are
shared by CUDA and HIP builds; at HaloFPX `4a156395`, Q2 symbols are present in
that shared backend and remain absent from Vulkan. Static symbol presence still
requires separate runtime qualification on each admitted backend.

<a id="s13-build"></a>
## Clean build and reference gates

Prerequisites: exact toolchain recorded by section 23/26; enough disk/RAM. No root required. Build separately on both nodes from identical source.

```bash
cd ROCmFPX
env JOBS=16 scripts/build-strix-rocmfp4-mtp.sh
scripts/check-rocmfp2-reference.sh
scripts/check-rocmfpx-reference.sh
scripts/check-rocmfp4-quant-regression.sh
scripts/check-rocmfpx-model-capabilities.sh
```

Capture commit, compiler/CMake, ROCm, Mesa/Vulkan loader/driver, kernel, firmware, CPU/GPU identity, CMake cache, command, exit code, stdout/stderr, and artifact hashes. These passes are correctness gates, not performance measurements.

<a id="s13-backend-matrix"></a>
## Backend operation matrix

Run with CPU, `ROCm0`, and `Vulkan0`, one clean process at a time:

```bash
scripts/sweep-rocmfpx-backend-ops.sh
scripts/check-rocmfp4-rocm-cpy-regression.sh
scripts/check-rocmfp4-rocm-fattn-regression.sh
scripts/check-rocmfp4-rocm-runtime-regression.sh
scripts/check-rocmfp4-vulkan-cpy-regression.sh
scripts/check-rocmfp4-vulkan-runtime-regression.sh
```

Required evidence: op/type/shape/backend result, maximum error against CPU reference, actual placement, fallback reason, and crash/hang. A fallback is not a kernel pass.

<a id="s13-model-validation"></a>
## Matched model conversion and quality

Prerequisites: one legally usable BF16/F16 GGUF with SHA-256, calibration corpus, evaluation corpus, and enough storage.

1. Quantize straight Q4 dual/FAST, Q6, Q8; add Q3/Q2 only after higher-bit passes.
2. Record exact preset, imatrix hash, tensor-type histogram, output hash/size, and converter log.
3. Run deterministic CPU/HIP/Vulkan logits or token parity on short fixtures.
4. Run perplexity and task suites under section 78; agent presets require JSON, tool-call, code, and chat gates.
5. Run `llama-bench` under section 73 controls; do not compare unmatched cache, batch, context, or model revisions.

Candidate commands:

```bash
SRC=/models/source-bf16.gguf OUT=/models/model-rocmfp4.gguf \
  FORMAT=rocmfp4 PROFILE=straight scripts/quantize-rocmfpx-agent.sh

MODEL=/models/model-rocmfp4.gguf BACKEND=ROCm0 \
  scripts/check-rocmfpx-agentic-smoke.sh
```

<a id="s13-mtp-check"></a>
## MTP validation

Use a model whose parsed metadata and tensor inventory prove an embedded MTP head.

```bash
python3 scripts/rocmfpx-model-capabilities.py /models/model.gguf --json
MODEL=/models/model.gguf DEVICE=ROCm0 scripts/run-rocmfpx-mtp-server.sh
scripts/check-rocmfpx-mtp-smoke.sh
scripts/check-rocmfpx-spec-decode-all.sh
```

Required checks: non-speculative vs greedy speculative token equality; draft generated/accepted/rejected accounting; context boundary; partial-draft state; checkpoint restore; M-RoPE where applicable; separate target/draft cache types; cancellation/restart. Repeat on Vulkan. For two ranks, inject rank loss and prove the documented single-node fallback.

<a id="s13-cache-check"></a>
## TurboQuant and disk-cache validation

```bash
MODEL=/models/model.gguf DEVICE=Vulkan0 \
  scripts/run-rocmfpx-turboquant-asym-server.sh
```

Sweep `f16/f16`, `q8_0/q8_0`, `q8_0/turbo4`, and symmetric Turbo3/4 only when quality gates allow. Fill context rather than only allocating it. Record memory, prefill/decode, perplexity/task deltas, boundary settings, and MTP acceptance.

For SSD prompt cache, test wrong model/build/cache fingerprints, truncated file, byte corruption, permissions, disk-full, concurrent writers, crash during write, and restart. Expected behavior: safe error or miss/recompute; never consume invalid state. Root is required only for controlled filesystem/device fault injection, not ordinary tests.

<a id="s13-patch-reconstruction"></a>
## Patch-stack reconstruction

```bash
git -C ROCmFPX log --reverse --format='%H %aI %s' > rocmfpx-commits.txt
git -C ROCmFPX rev-list --max-parents=0 HEAD
git -C ROCmFPX merge-base HEAD upstream/master || true
```

Expected at this pin: root `ebee2649...` and no upstream merge base. In a scratch integration branch, port the clusters listed in [facts and constraints](facts_and_constraints.md#s13-patches) individually. Run format/reference gates after A/B, backend gates after C/D, model gates after E/F, and server failure tests after G.
