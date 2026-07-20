# L14Q-VK-01 default-off Vulkan Q8_0 KV pre-dequant candidate

Status: **QUALIFIED AS A DEFAULT-OFF EXPERIMENT; PERFORMANCE PROMOTION OPEN.**

L14Q-VK-01 is a target-native clean reimplementation of the behavior admitted
by the independently approved Vulkan runtime P3 at preparation commit
`4f0a2749c2b3c23dc3d45ea25a380ed2a274dfc2`. No donor patch, donor expression,
GPL implementation, or new dependency entered the MIT engine.

The retained P3 record is
`reviews/local-work/2026-07-19__l14q-vulkan-coopmat1-quant-kv-runtime-p3-candidate__review__v01.md`
in the preparation repository. It records behavioral evidence from donor
`Nathanw1014/llama.cpp`, Nathan Wilson's introducing commits
`4edaca09fa36acc16e7b95a6544a80ccd0dff657` and
`4355d03e86083004bd9a084eed039987806ece8a`, and the first commit's
`Assisted-by: Claude (Opus 4.8)` trailer. Those facts are provenance only; no
donor code or expression was copied.

When `GGML_VULKAN_FA_Q8_0_PREDEQUANT=ON`, standard symmetric Q8_0 K/V may be
converted into the existing reusable F16 scratch layout before the established
Vulkan coopmat1 FlashAttention prefill path. Admission requires the original
Q8 route and the effective F16 route both to select coopmat1, at least 64 query
rows, dense block-compatible K/V, a present conversion pipeline, bounded
descriptor ranges, and overflow-safe K, V, mask, and conservative split-scratch
arithmetic. TurboQuant, ROCmFPX, Q4_0, scalar FA, coopmat2, and every failed
condition retain their prior routes.

Scratch growth is transactional. Every required replacement buffer is allocated
before any authoritative pointer or size changes; Vulkan or host allocation
failure destroys only unpublished temporaries and returns to the original Q8
path. After required allocation succeeds, pending commands are synchronized,
the required replacements are published, and only then are candidate types and
tuning admitted.

The option is compile-time only and defaults `OFF`. There is no CLI, server,
RPC, persistence, WebUI, or public API surface.

## Focused Linux qualification

On nimo-1 (Linux, RADV Strix Halo), matched Release Vulkan builds completed with
the option `ON` and `OFF`, HIP/RPC disabled, and Vulkan debug/testing hooks off.
The pure selector contract passed in both builds. Five focused Vulkan cases
passed in both builds: rows 63/64/65, D128/D256, a dense permutation, and Q4_0
fallback. Their ON/OFF CSVs were byte-identical at
`1e8c4c9a5502406ee7da8b2bff5bb74d89d7a36338c03797645438bbd4fc9591`.
Two inherited non-dense Q8 controls were also byte-identical at
`e69d34254142310f99f0b217ae65afe19a6ddda75dccbd528d48ff314c55d9c5`.

The unconditional destination cast used by the shared copy shader did not alter
the existing feature-off shaders: ON/OFF `cpy_q8_0_f32.spv` and
`cpy_f32_q8_0.spv` hashes matched exactly. The new F16 shader exists only in the
ON build.

Two qualification-only builds exercised the high-risk branches. A sentinel
after successful scratch reservation exited 134, proving the eligible candidate
was reached. A forced `vk::OutOfDeviceMemoryError` inside the reservation
transaction exited 0 with the representative request completing through the
original path. Both sentinels were removed before the final builds. Independent
review returned `ACCEPT` with no remaining P1/P2 correction.

No model-level performance claim is made. A small available GGUF route probe was
excluded because this base does not recognize its `deepseek4_mtp_support`
architecture. The exact pinned 160 GB MiniMax primary workload remains the
separate matched performance authority; VK-01 remains `OFF` until its own
correctness, telemetry, and strict zero-regression evidence supports promotion.

## Deferred work and rollback

Risk-proportionate work deferred here includes the full Vulkan backend sweep,
exhaustive layout/fault permutations, multi-node qualification, and matched
primary-model performance. Those gates remain open; no absence of evidence is
treated as a speedup or safety claim.

Rollback is a rebuild with `GGML_VULKAN_FA_Q8_0_PREDEQUANT=OFF` (the default) or
one coherent revert. No running inference service or known-good deployment was
changed by this milestone.
