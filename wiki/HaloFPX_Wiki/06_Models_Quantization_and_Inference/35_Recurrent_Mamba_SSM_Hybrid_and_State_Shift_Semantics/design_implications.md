---
section_id: "35"
title: "Recurrent state design implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["57", "58", "61", "63", "77"]
---

# Design implications

## Atomic continuation

**[RECOMMENDATION]** Store one manifest that references all rank-local attention pages, recurrent tensors, position/sequence metadata, and checksums. Publish it atomically only after every component is durable. Restore succeeds only if all required components validate; otherwise recompute from a verified prefix.

**[RECOMMENDATION]** Include an explicit state-kind registry (`attention-kv`, `mamba-ssm`, `rwkv`, `gated-delta`, model-specific) and reject unknown kinds. Never deserialize by tensor size alone.

## Sequence operations

| Operation | Safe default for recurrent/hybrid state |
|---|---|
| clear entire sequence | clear all state kinds together |
| append tokens | advance from the exact preceding state |
| remove suffix | restore checkpoint at/before cut, then recompute |
| edit/remove middle | invalidate all state after first changed token |
| copy/fork sequence | deep logical snapshot or proven copy-on-write; no accidental aliasing |
| context shift | recompute unless architecture-specific equivalence is proven |
| cross-rank restore | require same owner map or an explicit verified migration |

**[INFERENCE]** Attention-only prefix matching is insufficient for hybrid models: two sessions can share token suffixes or positions but have different recurrent states.

## Distributed ownership

**[RECOMMENDATION]** Each rank writes only the state for layers it executes, with a coordinator manifest containing rank count, layer ranges, shard hashes, and completion generation. On rank loss, fall back to a complete single-node checkpoint or recompute; do not combine shards from different generations.

**[ASSUMPTION]** Layer ownership will be stable during a session. If section 47 permits live replanning, it must specify a stop-the-world state migration protocol first.

