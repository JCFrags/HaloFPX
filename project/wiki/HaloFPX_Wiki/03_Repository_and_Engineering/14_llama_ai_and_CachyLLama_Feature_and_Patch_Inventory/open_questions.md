---
section_id: "14"
title: "Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
  software_versions: []
  hardware_revisions:
    - "planned dual matched AMD Strix Halo nodes"
related_sections: ["11", "13", "15", "16"]
---

# Open Questions

| ID | Question | Why it blocks a decision | Resolution evidence |
|---|---|---|---|
| S14-Q01 | What exact ROCmFPX/llama.cpp commit receives the selected patches? | serialized state and server internals are commit-sensitive | section 11 frozen baseline and section 15 patch plan |
| S14-Q02 | Which exact fields define checkpoint compatibility? | current hash omits several semantic identities | compatibility-manifest ADR plus mismatch tests |
| S14-Q03 | Are checkpoint payloads stable across Vulkan/ROCm, compiler, device layout, and K/V quantization? | determines reuse and migration scope | differential restore matrix |
| S14-Q04 | What are rank ownership and logical-commit semantics for every distributed mode? | partial distributed restore can corrupt execution | topology-specific experiment and ADR |
| S14-Q05 | Can a system prompt contain tenant/user secrets? | current cache is cross-conversation and model-global | threat model and scope-key design |
| S14-Q06 | Is fuzzy continuation needed, and what proof threshold is safe for recurrent state? | first-4,096-token similarity is not full-state equivalence | adversarial divergence/logit tests |
| S14-Q07 | What are the intended anonymous policy and reliable 429 path? | docs/accounting disagree; fast path reads a moved-from vector; release clears identity before decrement | fixed lifecycle plus response/race/cancel tests |
| S14-Q08 | How is `llama_user_id` bound to authenticated identity and residual slot state? | client labels are spoofable, release erases ownership, and prompt-similarity selection lacks a user filter | gateway/API contract, lifecycle fix, cross-user negative tests |
| S14-Q09 | Does cache v3 reject every corruption/torn-write class without large allocation or crash? | no checksum/atomic publication is visible | fault-injection suite |
| S14-Q10 | What recurrent architectures and context operations are actually supported? | source comments include unresolved recurrent rollback concern | per-architecture state-equivalence results |
| S14-Q11 | Is MTP/draft/spec state worth its ABI and storage cost? | extra blobs expand compatibility and failure surface | ablation of TTFT/throughput/size on both nodes |
| S14-Q12 | What is expert-tracking overhead and accuracy under batching/multi-rank execution? | instrumentation could distort the workload it measures | counters vs known routing plus overhead study |
| S14-Q13 | Which runner profiles remain valid on the two matched Halo systems? | several profiles deliberately disable SSD cache and are filename-driven | `--print-profile` audit plus matched experiments |
| S14-Q14 | Are repository benchmark warm runs always SSD-restored rather than RAM/page-cache or ordinary prompt-cache hits? | source labels alone do not establish cache path | fresh-boot/drop-cache protocol and log assertions |
| S14-Q15 | What SSD endurance and capacity budget is acceptable? | large context checkpoints can create substantial write amplification | bytes/turn, DWPD estimate, eviction study |
| S14-Q16 | Which CachyLLama backend/kernel changes overlap or conflict with ROCmFPX? | fork delta includes CUDA/Metal/Vulkan and Strix tuning beyond cache features | semantic diff and section 13/15 integration review |

## Current contradictions to preserve

**[OPEN]** CachyLLama README lists `--cache-ssd-max-cold` default `0` (unlimited), while the low-level config default is 32 and `llama-ai` sets 32. Resolve defaults at the actual launch layer rather than copying documentation. [S14-001][S14-002]

**[OPEN]** CachyLLama claims the system cache works for hybrid models, while correctness still depends on template boundary detection, partial-state semantics, and recurrent coverage. This remains a claim until the matrix in [procedures](procedures_and_checks.md#d-deterministic-checkpoint-equivalence-matrix) passes.

**[OPEN]** `llama-ai` documents ROCm instability on RDNA3 and uses Vulkan as default, while HaloFPX is explicitly ROCmFPX-based. Determine whether only cache/server ideas are portable or whether backend-dependent state behavior also changes. [S14-001]

## Follow-up triggers

Re-run this inventory when any of the following changes: selected base commit, CachyLLama gitlink, cache format version, llama.cpp state API, recurrent/MTP implementation, user identity contract, distributed topology, or model/profile set.
