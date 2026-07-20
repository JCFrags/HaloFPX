# L14Q-H01 default-off HIP standard quantized-KV decode port

Status: **QUALIFIED AS A DEFAULT-OFF EXPERIMENT; PERFORMANCE PROMOTION OPEN.**

H01 is a target-native clean reimplementation of the behavior admitted by the
independently approved `L14Q-H01` runtime P3 at preparation commit
`4f0a2749c2b3c23dc3d45ea25a380ed2a274dfc2`. No donor patch was applied and no
donor or GPL implementation entered the MIT engine.

When `GGML_HIP_QUANT_KV_FATTN_TILE=ON`, the HIP selector may route symmetric
Q8_0/Q8_0 or Q4_0/Q4_0 K/V through the existing shared-tile FlashAttention
decode kernel without materializing a full-cache F16 copy. Admission is limited
to D128 or D256, one query token, exact GQA8, the existing mask/zero-bias/padded
KV contract, packed quantized rows, aligned higher strides, and a supported tile
configuration. ROCmFP4, ROCmFPX, TurboQuant, mixed types, prompts, wrong ratios,
and every failed precondition preserve their existing routing.

The option remains compile-time only and defaults `OFF`. There is no CLI,
server, RPC, Vulkan, persistence, WebUI, or public API surface in H01.

## Focused target qualification

On nimo-1, clean Linux gfx1151 HIP builds passed with the option both `ON` and
`OFF`. The four admitted D128/D256 x Q8_0/Q4_0 correctness cells passed; the
accepted 20-case L14Q-T01 matrix passed 20/20 in both builds; inherited ROCm
`FLASH_ATTN_EXT` passed 2919/2919; D160 remained unsupported; representative
ROCmFPX and Turbo controls passed; and the feature-off plus L02 contracts passed
2/2. `roctracer` identified the typed tile kernel for admitted D128/Q8 and
D256/Q4 controls and the existing VEC/F16 routes for wrong-GQA and prompt-batch
controls. No permanent diagnostic was added.

The exact pinned 160 GB primary artifact was then run on nimo-1/nimo-2 with
identical build/runtime/request tuples in the order H01, control, H01, control.
Each block contained one excluded warmup and five retained requests. All 24
requests returned HTTP 200, consumed 1129 prompt tokens, generated 128 tokens,
reported zero prompt-cache reuse, and produced content hash
`a9c38c7f948adcfa8cfab5468ab84cc089b01a34c3f270f1c487a9a5fa74b555`
using `jq -r .content` including its trailing LF.

| Metric | H01 ON, mean +/- sample SD | Control, mean +/- sample SD | H01 - control | Approx. 95% CI |
|---|---:|---:|---:|---:|
| Prompt tokens/s | 207.78595 +/- 0.11058 | 207.80601 +/- 0.12183 | -0.02006 (-0.0097%) | [-0.12204, 0.08191] |
| Generation tokens/s | 16.75229 +/- 0.02844 | 16.83636 +/- 0.20606 | -0.08407 (-0.4993%) | [-0.21300, 0.04486] |

The generation control contains two high samples, so the interval crosses zero
despite a worse H01 point estimate. This is ambiguous evidence: it is neither a
speedup claim nor a confirmed regression. The strict final non-inferiority gate
is open and the feature remains `OFF`. The owner's greater-than-30 tokens/s goal
remains a stretch objective, not a pass/fail baseline.

## Open gates and rollback

The two edited instance units are generated files. Their current generator does
not emit the H01 declarations, so regeneration would remove them. Do not run the
generator or promote a release until an admitted generator update or an
equivalent deterministic preservation contract resolves this source-of-truth
hazard.

Risk-proportionate work deferred from this milestone includes the final G9/G10
trial volume, D256 model-level performance, broad TTFT/tail/utilization/memory
telemetry, and exhaustive selector/fault permutations. Those remain versioned
acceptance work; they were not allowed to block visible default-off progress in
the absence of a concrete defect.

Rollback is a rebuild with `GGML_HIP_QUANT_KV_FATTN_TILE=OFF` (already the
default) or one coherent revert of H01. The known-good production services were
not reconfigured and were restored after qualification.
