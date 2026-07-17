---
section_id: "41"
title: "Remote Speculation Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate"]
  software_versions: []
  hardware_revisions: []
related_sections: ["31", "48", "52", "66"]
---

# Procedures and checks

## Source research completed

Reviewed both foundational speculative-decoding papers, pinned current llama.cpp/vLLM speculation docs, and separated draft-model and MTP semantics. No remote HaloFPX benchmark exists.

## `DR-41-E1`: correctness oracle

1. Pin model/tokenizer hashes and sampler ABI; create fixed prompt corpus including penalties, stop tokens, grammar, context shift, and long context.
2. For greedy mode, compare every output token with target-only greedy across draft depths and forced rejection positions.
3. For exact stochastic mode, unit-test acceptance and correction against a CPU reference on synthetic small vocab distributions, including zeros, `p=q`, total rejection, and transformed distributions.
4. Run distributional tests across many seeded samples against target-only sampling; define statistical method and power before results.
5. Inject duplicate/reordered rounds, stale epochs, corrupt probability payload, token-hash mismatch, cancellation, and rollback failure.

## `DR-41-E2`: remote break-even

Sweep compatible draft models/quantizations, depth, prompt/output bucket, concurrency, and both physical links. Measure draft/verify/round p50/p95/p99, acceptance histogram, target calls, bytes by metadata class, cache memory, rollback, TTFT/ITL/E2E, energy, and fallback. Compare target-only, colocated draft, remote draft, and native MTP when supported. Keep exact model/runtime hashes and seeds.

## `DR-41-E3`: fault fallback

Kill/delay draft worker and drop/corrupt proposal/result messages at each round phase. Target must fence the branch and continue target-only from the last committed state without duplicate client tokens. Verify reconnect requires a fresh epoch/base hash.

## Promotion gates

- Exact claims require reference and distributional tests, not token anecdotes.
- Performance promotion requires lower end-to-end p99 for a declared workload without correctness regression.
- Approximate compact metadata is named and quality-gated separately.
- Raw acceptance and fabric data are retained.
