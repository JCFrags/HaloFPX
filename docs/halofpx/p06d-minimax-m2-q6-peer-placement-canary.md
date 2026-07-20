# P06d MiniMax-M2 Q6 peer-placement canary

Status: **exact-artifact one-layer peer-data placement qualified; shadow compute remains closed**

P06d adds the first runtime seam for the pinned 160 GB primary artifact. The
environment variable `HALOFPX_MINIMAX_M2_EXPERT_SHADOW_LAYER` is absent by
default. When explicitly set, it admits exactly one local-ROCm-owned layer of
the exact 62-layer, 3072-hidden, 1536-expert-FFN, 192-expert, top-8 MiniMax-M2
tuple and duplicates its gate, down, and up `Q6_0_ROCMFPX` expert tensors into
one RPC peer buffer. The normal graph remains authoritative and never reads
the duplicates.

Admission requires strict decimal parsing, layer split, exactly one ROCm and
one RPC backend registry, a locally owned designated layer, exact model
geometry, and exact Q6 expert types. Tensor buffer overrides are rejected.
The placement helper filters buffer types by exact requested-device ownership;
CPU fallbacks cannot satisfy it. Implementation-only duplicate tensors are
excluded from public name lookup, preserving authoritative adapter lookup.

## Exact two-node qualification

Nimo-2 coordinated layer 32 on local `ROCm0`; nimo-1 served `RPC0` on port
50053. Both isolated binaries were built in Release mode with GCC 16.1.1,
HIP, Vulkan, and RPC enabled and the WebUI disabled. The model was the pinned
revision `dba517197f2854f3d362529e13abddcdcad6c10b`, file
`saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`, size
159,873,097,824 bytes, SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.

The corrected canary loaded in 2m39.65s and returned HTTP 200 for the retained
1,129-prompt-token, 128-generation-token deterministic request. Its output
content SHA-256 was
`a9c38c7f948adcfa8cfab5468ab84cc089b01a34c3f270f1c487a9a5fa74b555`,
identical to the retained P04 control and candidate output. The single run
reported 199.79 prompt tokens/s and 16.61 generation tokens/s. These are a
compatibility canary, not a matched performance result or speed claim.

The malformed value `abc` failed before tensor data loading with process exit
status 1 and the strict-decimal diagnostic. The final source review accepted
the requested-device filter, override rejection, registry identity checks,
authoritative tensor lookup, lifetime, default-off behavior, and unchanged
graph. The first pre-review run is retained but not promoted: review found
that its buffer selection contract did not by itself exclude overrides.
The inherited feature-off and locked L02 contract controls passed 2/2 on the
final source. The exact Linux HIP/Vulkan/RPC build and request are the primary
target-environment qualification.

## Evidence and boundary

The corrected nimo-1 evidence manifest SHA-256 is
`63b0345ae95c14b4c7d2759d3bb20d0e17b69adb18e2948de0e8eb645e0b2444`;
its 1,849-byte bundle SHA-256 is
`fc64f0aa5501c43d4b000f0d6382ccf592e379b730506b2b4de70febf5db03e7`.
The corrected nimo-2 manifest SHA-256 is
`c40f4661bdd74ad1469aec022c1ed12a0989162a45e3b454844c6a133256730b`;
its 6,932-byte bundle SHA-256 is
`2c42a6115797899fa7a8fbb643e45cc548ed11b133c255fb0596c7b9e4c98780`.
All manifest entries verify.

P06d proves Q6 peer-data placement and unchanged authoritative inference for
one exact layer. It does not execute the peer copies, partition Q6 experts,
overlap local and remote work, improve performance, enable persistence, or
change KV, attention, TurboQuant, ROCmFPX, HIP, Vulkan, RPC, MTP, or
speculative behavior. P06b/P06c remain synthetic Q8 backend and
partition-mechanics evidence only.

The disposable coordinator was stopped before its worker. The known-good
nimo-2 worker and nimo-1 server were restarted worker-first with zero restarts;
HTTP health returned 200 before closeout, and the placement environment is
absent from both service definitions. No donor, GPL llama-ai, or
CachyLLama code, dependency, model mutation, remote, WebUI, persistent write,
deployment replacement, or reference-clone change entered this milestone.
