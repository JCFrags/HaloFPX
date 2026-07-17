---
section_id: "48"
title: "Distributed Correctness Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "ROCm/rccl@57e58688f44c77076ad536ef1f6b68741fc6e694"]
  software_versions: []
  hardware_revisions: ["two matched AMD Strix Halo systems (exact BOM open)"]
related_sections: ["42", "45", "53", "57", "61", "63"]
---

# Facts and constraints

| Claim | Consequence |
|---|---|
| **[VERIFIED]** Fixed sampling seed alone does not guarantee identical output when batch shape or prompt-cache execution changes. | Determinism fixtures must freeze backend, plan, batch/ubatch, cache path, concurrency, and build [S48-LLAMA-SERVER]. |
| **[VERIFIED]** llama.cpp can return raw token IDs and exposes seed, slot, cache, split, and streaming controls. | Tests can compare token/logit/state behavior at stable interfaces [S48-LLAMA-SERVER]. |
| **[VERIFIED]** RCCL has asynchronous communicator error inspection (`ncclCommGetAsyncError`). | The runtime must poll/propagate errors and define timeout/abort; success cannot be inferred from a missing synchronous exception [S48-RCCL-API]. |
| **[VERIFIED]** RCCL source is versionable and includes collective/group implementation plus fault-injection build support. | Test evidence must name the exact source/build, and fault injection can supplement physical cable tests [S48-RCCL-SRC, S48-RCCL-BUILD]. |
| **[VERIFIED]** PyTorch's deterministic-algorithm mode is scoped to same inputs/software/hardware and warns it alone is insufficient for reproducibility. | A useful precedent for fail-on-nondeterministic-op, not evidence for HaloFPX [S48-TORCH-DET]. |

## Correctness domains

**[RECOMMENDATION]** Distinguish:

- protocol correctness: no missing/duplicate/out-of-order step, token, collective, or cancellation;
- state correctness: KV/recurrent/MTP/sampler state matches model, plan, rank ownership, token prefix, and epoch;
- numerical conformance: logits/activations within predeclared tolerances for a fixed fixture;
- deterministic test mode: repeated identical fixture yields the declared level (bitwise tokens/logits or tolerance-bounded logits);
- user-visible correctness: exactly one committed token sequence with explicit terminal status.

**[INFERENCE]** Bitwise equality across HIP/Vulkan or distributed/single-node paths is unlikely to be a safe universal requirement due to floating-point reduction order. Token equality at greedy decoding plus bounded logit error and model-quality gates is a candidate, not yet approved.

**[ASSUMPTION]** The transport can report per-link failure and integrity status. Section 53 must define framing, sequence numbers, checksums/authentication, and credits.
