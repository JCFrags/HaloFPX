---
section_id: "74"
title: "Single-Node HIP and Vulkan Baseline Matrix"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
  software_versions: ["ROCm and Mesa versions must be captured per run"]
  hardware_revisions: ["nimo-1 and nimo-2; exact current revisions pending"]
related_sections: ["18", "22", "23", "29", "30", "31", "33", "36", "73", "76", "78", "79", "81"]
---

# Single-node HIP and Vulkan baseline matrix

This section defines the plan that must establish each Strix Halo node's independent performance and correctness envelope before any two-node speedup claim.

**[OPEN]** No Section 74 machine run or raw result exists in this workspace. Every result cell is NOT_RUN; this section contains no [MEASURED] performance claim.

## Baseline contract

**[RECOMMENDATION]** Run the same immutable model bytes, runtime commit, build manifest, prompt token IDs, sampling policy, and method on nimo-1 and nimo-2, separately for one HIP device and one Vulkan device. Never aggregate the nodes or call a fork-reported benchmark a HaloFPX result.

The matrix covers both machines; HIP and Vulkan; dense, MoE, hybrid/SSM, long-context, and MTP-capable candidates; reference, conventional GGUF, and ROCmFPX quants; context, batch/ubatch, K/V type, FlashAttention, MTP, and power mode. Outputs include prompt/decode throughput, TTFT, p50/p95/p99 inter-token latency, memory, quality, power, and thermals.

The executable design is in [procedures_and_checks.md](procedures_and_checks.md), the staged matrix in [design_implications.md](design_implications.md), and eligibility rules in [facts_and_constraints.md](facts_and_constraints.md).

## Research split

1. **Completed now:** pinned benchmark/server/perplexity interfaces; pinned Strix build script; matrix schema and commands; known model/quant/cache/speculation dimensions.
2. **Required on machines:** inventory, artifact hashing, backend enumeration, smoke gates, warm controlled trials, streaming latency capture, quality checks, sensor/power capture, and raw evidence preservation on both nodes.
3. **Contingent decisions:** default backend, production quant/KV/FA/MTP policy, maximum context, power mode, batching defaults, and whether nodes are interchangeable.

## Authority and dependencies

**[VERIFIED]** At pinned llama.cpp, llama-bench separates prompt processing (pp), text generation (tg), and combined (pg), emits JSONL samples, and sweeps depth, batch, K/V types, FA, and device. It explicitly excludes tokenization and sampling time [S74-01]. Server responses expose prompt/decode timing while streaming measurement is required for client-visible TTFT and inter-token latency [S74-02].

**[RECOMMENDATION]** Treat Section 73 as the proposed methodology and record-model authority, not as an implemented schema or measurement source. Section 74 outputs remain candidate evidence until the Section 73 executable schema/validator and relevant controls are validated.

## Closeout review

- No machine result was inferred; volatile state is deferred to live capture.
- Repository claims are pinned to exact commits and primary sources in [sources.md](sources.md).
- Next review trigger: first complete paired HIP/Vulkan evidence bundle, runtime/model hash change, or validation of the relevant Section 73 controls.

[S74-01]: sources.md#s74-01
[S74-02]: sources.md#s74-02
