---
section_id: "76"
title: "Distributed Benchmark Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
  software_versions: []
  hardware_revisions: ["planned matched dual Strix Halo"]
related_sections: ["38", "40", "41", "42", "43", "44", "73", "75"]
---

# Facts and Constraints

## Implementation baseline

**[VERIFIED]** The intended ROCmFPX source observation point for this plan is commit `a5605a72768c6562241b248e268e33dc92787394`; proposed modes still require an exact implementation inventory at that revision. [S76-007]

**[VERIFIED]** At llama.cpp commit `788e07d`, the RPC backend can expose remote devices, distribute tensors/KV across local and remote devices, and accept explicit `--tensor-split`. The documentation warns that RPC is proof-of-concept, fragile, and insecure. [S76-001]

**[VERIFIED]** The pinned speculative documentation distinguishes a draft model, MTP heads, and draftless n-gram mechanisms and exposes draft length/probability/device controls. [S76-002]

**[ASSUMPTION]** HaloFPX will implement or port replication, remote draft, two-rank tensor parallel, contiguous pipeline, and MoE-aware hybrid plans. Their presence in the project brief is not implementation evidence.

## Mode cost models

| Mode | Primary benefit hypothesis | Dominant costs to isolate |
|---|---|---|
| replication | parallel independent requests, failure isolation | duplicate model memory and cache; no single-request compute split |
| native MTP | verify several self-drafted tokens per target step | MTP head compute/memory, acceptance, verification batch, rejected work |
| remote draft | overlap/relocate draft work | draft compute, request/state transfer, network wait, acceptance, cancellation |
| tensor parallel | split layer operators across ranks | collectives per layer/token, imbalance, synchronization, smaller local kernels |
| pipeline parallel | split contiguous layer stages | activation transfer, stage imbalance, fill/drain bubbles, microbatch queueing |
| MoE hybrid | place/replicate experts by routing demand | routing transfer, expert imbalance, cold experts, duplication, all-to-all-like traffic |

**[VERIFIED]** Speculative decoding can preserve the target distribution when its acceptance/correction algorithm is implemented as specified; speed depends on draft cost, verification, and accepted tokens. The paper's reported acceleration is scoped to its systems and models. [S76-003]

**[VERIFIED]** Megatron-LM places tensor-parallel communication operations around partitioned transformer computations. Its results concern different training hardware/software and do not predict HaloFPX inference speed. [S76-004]

**[VERIFIED]** GPipe identifies pipeline bubble overhead and uses microbatches to amortize fill/drain idle time. Its training formulation is useful vocabulary, not a direct inference performance claim. [S76-005]

**[VERIFIED]** DeepSpeed-MoE treats sparse expert inference as a distinct placement and communication problem. Its published results cannot be transferred to two-node Strix Halo. [S76-006]

## Required observations

For every run retain request-level TTFT, prompt/decode time, inter-token latencies, completion latency, accepted/drafted/verified tokens, quality/correctness outcome, per-rank compute and idle intervals, bytes/messages per link, collective/activation latency, queue wait, memory, power, thermals, errors, retries, and fallback events.

**[RECOMMENDATION]** Define scaling efficiency for a metric explicitly. For aggregate goodput, `E = throughput_mode / (2 * throughput_one_matched_node)`. For latency speedup, `S = latency_one_node / latency_mode`. Report both; a mode may improve one and regress the other.

**[OPEN]** A distributed output must pass the Section 78 equivalence/quality policy before its performance is eligible for comparison.

No HaloFPX result is measured here.
