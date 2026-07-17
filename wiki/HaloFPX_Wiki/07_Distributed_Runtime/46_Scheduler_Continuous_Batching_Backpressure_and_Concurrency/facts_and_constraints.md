---
section_id: "46"
title: "Scheduler Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "vllm-project/vllm@9354f222042986addf20709e5274fc26e0d09745"]
  software_versions: []
  hardware_revisions: ["two matched AMD Strix Halo systems (exact BOM open)"]
related_sections: ["39", "43", "45", "48", "58", "68"]
---

# Facts and constraints

| Claim | Evidence and implication |
|---|---|
| **[VERIFIED]** llama.cpp `server_slot` represents one sequence; `server_context` owns the inference context and active slots. | [S46-LLAMA-DEV] at pinned commit. HaloFPX must not equate an HTTP connection with distributed sequence ownership. |
| **[VERIFIED]** llama.cpp fills one shared batch from active slots with prior generated tokens or prompt tokens, then calls `llama_decode`; differing LoRA configurations are incompatible for batching. | [S46-LLAMA-DEV]. Batch eligibility needs an explicit compatibility key. |
| **[VERIFIED]** llama.cpp exposes continuous batching, slot count, explicit slot selection, streaming, cancellation paths, and Prometheus metrics. | [S46-LLAMA-SERVER]. These surfaces do not define multi-host atomic admission. |
| **[VERIFIED]** llama.cpp warns that prompt-cache reuse can change results because logits may not be bit-identical across batch sizes. | [S46-LLAMA-SERVER]. Correctness testing must vary co-tenancy and batch shape. |
| **[VERIFIED]** vLLM V1 prioritizes decode, then uses remaining token budget for chunked prefills; on KV exhaustion it documents preemption plus recomputation. | [S46-VLLM-OPT]. This is a design precedent, not a HaloFPX result. |
| **[VERIFIED]** llama.cpp documents layer split as pipeline-oriented and tensor split as collective-heavy and experimental outside the stated NVIDIA expectation. | [S46-LLAMA-MGPU]. Scheduling policy must be plan-mode-specific. |

## Mixed-workload constraints

**[INFERENCE]** Prefill is bursty and compute-heavy while decode is latency-sensitive and repeated; allowing unbounded long prefills can inflate every active stream's inter-token latency. A bounded prefill quantum plus decode deadline guard is therefore required.

**[INFERENCE]** In tensor/pipeline modes, the slowest required rank determines step completion. Local admission based only on coordinator memory can deadlock or fail after partial reservation.

**[ASSUMPTION]** The intended product serves multiple users and agent sessions concurrently. Exact tenants, quotas, and priority classes remain unspecified.

## Scheduling metrics

Record by model fingerprint, plan ID, backend, rank set, user class, prompt bucket, and cache-hit class:

- arrivals, admitted, rejected, cancelled, timed out, replayed;
- queue depth in requests and tokens; queue wait p50/p95/p99;
- TTFT, inter-token latency, end-to-end latency, output tokens/s, useful tokens/s;
- active slots, scheduled tokens/step, batch fill, prefill/decode tokens, idle/bubble time per rank;
- KV reserved/used, cache hit/miss/restore bytes, evictions and recomputations;
- transport credit stalls, collective wait, rank skew, stream-buffer bytes and blocked-client time;
- per-user service share, maximum starvation interval, priority inversions, and overload duration.

No **[MEASURED]** claims exist for the target machines in this section.
