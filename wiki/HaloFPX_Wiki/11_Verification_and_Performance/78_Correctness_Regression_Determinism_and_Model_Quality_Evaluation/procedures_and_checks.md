---
section_id: "78"
title: "Correctness, Regression, Determinism, and Model Quality Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: ["Reproducible Builds definition accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["31", "54", "73", "74", "76", "77", "80", "81"]
---

# Procedures and checks

All commands are proposed. Run them from clean checkouts at the commits in the front matter and record the complete build and machine manifests. Do not overwrite prior evidence.

## Matrix

| Layer | Minimum fixtures | Required outputs |
|---|---|---|
| Unit/golden | tokenizer, template, grammar, schema, serialization, checksum | JUnit/log, exact fixture diff |
| Backend | every enabled op/type/shape; boundary and odd sizes | comparator name, threshold, observed error |
| Model smoke | load, fixed prompt, prefill/decode, embeddings if supported | tokens, timing excluded from correctness gate, logit digest |
| State | full and sequence save/restore, fragmented KV, recurrent rollback | state identity, exact continuation or approved logit comparison |
| Distributed | one node, two ranks, each rank placement, single-node fallback | ownership manifest, cross-rank logits, explicit failure behavior |
| Quantization | unquantized reference and each release quant | PPL, delta PPL, KLD, top-token agreement, task results |
| Behavior | code, JSON, tool calls, grammar, long-context retrieval | machine-validated assertions and raw responses |
| Speculation | MTP/EAGLE acceptance and rejection paths | accepted-token identity and post-rejection state equivalence |

## Execution outline

1. Hash the checkout, submodules, model, tokenizer, datasets, prompts, and expected fixtures.
2. Build the CPU reference plus every release backend with tests enabled. Preserve CMake cache, compiler version, linked-library inventory, and build log.
3. Run `ctest --test-dir build --output-on-failure`; classify every test as required, optional, or unsupported before execution. A required skip is a failed gate.
4. Run `build/bin/test-backend-ops` for CPU reference, HIP/ROCmFPX, Vulkan, and distributed placements. Preserve per-operation comparator data.
5. Build `llama-server` and run `tools/server/tests/tests.sh`; enable documented slow tests in nightly/RC scope. Preserve pytest/JUnit output and server logs.
6. Run fixed greedy and seeded-sampling replays at least repeatedly enough to characterize within-configuration variance. Randomize candidate/reference order to reduce thermal/order bias.
7. Use `llama-perplexity` on a pinned, licensed corpus and identical tokenizer. For deep triage, generate base logits once and use KL-divergence mode for candidates; budget disk space before writing logits.
8. Run ROCmFPX JSON/grammar, tool, long-context, MTP/EAGLE/speculation, runtime-regression, and state-restore checks after auditing local paths and required model identities.
9. Corrupt one byte in a copy of each serialized state/cache fixture and verify rejection or recomputation. Never mutate the only evidence copy.
10. Emit a machine-readable result containing expected and observed values, comparator, threshold origin, pass/fail/skip reason, and artifact hashes.

## Acceptance rule

[RECOMMENDATION] Exact surfaces must match exactly. Numerical surfaces pass only their predeclared comparator. Behavioral suites must satisfy machine-checked assertions. Any unclassified mismatch, required skip, crash, timeout, silent fallback, or invalid-state acceptance blocks promotion. No universal cross-backend logit threshold is approved yet.
