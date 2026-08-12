---
section_id: "37"
title: "Unranked gfx1151 optimization hypotheses"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: []
  hardware_revisions: ["gfx1151"]
related_sections: ["33", "34", "36", "42", "74"]
---

# Unranked optimization hypotheses

The identifiers below are stable hypothesis labels, not priorities. M37-01 must establish device-specific and end-to-end time share before any candidate is ranked.

| ID | Surface | Decode hypothesis | Prompt-fill hypothesis | Required guard |
|---|---|---|---|---|
| H37-A | ROCmFPX dequant + GEMV/MMID | fuse/unpack in registers; specialize `n=1..8`; reduce barriers/launches | route larger shapes to GEMM path | quant error, all formats/shapes, end-to-end tokens/s |
| H37-B | graph reuse and launch overhead | reuse captured/static graph shapes; avoid host sync/allocation | amortize graph construction over batches | dynamic slot/shape correctness, failure fallback |
| H37-C | MoE expert kernels | coalesce selected IDs/tokens; avoid empty expert work; tune indexed matvec | batch tokens by expert without reordering semantics | expert IDs/weights/output equality |
| H37-D | attention + RoPE | fuse position transform where reuse permits; tune FA small-q path | tiled FA, LDS/register balance | causal mask, long context, head sizes, numeric tolerance |
| H37-E | FFN gate/up/down | fused gate-up and activation; quantized down path | larger tiled GEMM and epilogue fusion | activation/order and quant equivalence |
| H37-F | row-parallel reductions | overlap link/reduction and local work; fuse residual where legal | larger reduce chunks | rank order, deterministic tolerance, single-node fallback |
| H37-G | RMSNorm/residual/elementwise | fuse adjacent bandwidth-bound kernels | lower launch count | aliasing, epsilon, mixed precision |
| H37-H | fused QKV/projections | reduce reads/launches when tensor layout permits | improve reuse in batched GEMM | model-specific layouts, LoRA compatibility |
| H37-I | MTP verification | specialize `n=2..8`, reuse verify buffers/graphs | not primary prompt path | accepted-token/output equality |
| H37-J | memory layout | align/coalesce quant blocks and expert rows | improve tiled loads/transposes | loader compatibility and unchanged GGUF semantics |

**[OPEN]** No hypothesis has a HaloFPX priority until M37-01 attributes time to it and an end-to-end guard confirms that the attributed cost is material.

## Architecture tuning knobs

**[RECOMMENDATION]** Treat wave/subgroup size, workgroup dimensions, rows per block, vector load width/alignment, LDS footprint, register pressure/occupancy, unroll, quant block mapping, and MTP column count as autotuned variants with explicit capability guards.

**[RECOMMENDATION]** Maintain separate HIP and Vulkan results. A source-equivalent algorithm can compile to different instruction sequences and synchronization behavior.

**[INFERENCE]** Fusion wins only if saved traffic/launches exceed added register pressure, code complexity, and reduced occupancy. Retain unfused kernels as correctness and performance fallback.
