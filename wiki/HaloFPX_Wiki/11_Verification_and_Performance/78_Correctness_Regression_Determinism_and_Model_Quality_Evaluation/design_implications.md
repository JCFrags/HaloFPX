---
section_id: "78"
title: "Correctness, Regression, Determinism, and Model Quality Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: ["Reproducible Builds definition accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["31", "54", "73", "74", "76", "77", "80", "81"]
---

# Design implications

## Comparator hierarchy

| Surface | Required comparison | Status |
|---|---|---|
| GGUF metadata, tokenizer vectors, token IDs, request/response schema, checksums, state identity | Exact equality | [RECOMMENDATION] |
| Backend operations | Comparator and tolerance from the pinned operation test; additions require a reviewed numerical justification | [RECOMMENDATION] |
| Same build/backend/topology, greedy replay | Exact output tokens; retain logits to determine whether byte-exact floating-point replay is supportable | [RECOMMENDATION] |
| Cross-backend or cross-rank | Top-token agreement plus logit error/divergence against a pinned reference; calibrate limits from repeated reference runs | [RECOMMENDATION] |
| Quantized model | Same tokenizer and corpus; report perplexity delta, KL divergence, top-token agreement, and task regressions | [RECOMMENDATION] |
| Structured output | Parse success and exact schema/grammar invariants, not superficial text similarity | [RECOMMENDATION] |
| State restore/recurrent continuation | Exact identity checks followed by continuation comparison; corrupt or incompatible state must be rejected | [RECOMMENDATION] |

## Determinism record

Every replay bundle should contain model/GGUF and tokenizer hashes, prompt bytes, template, seed, sampler chain in order, sampling parameters, runtime commit, compiler and flags, loaded libraries, backend/device map, rank ownership, transport configuration, cache generation, restored-state fingerprint, batch/ubatch/slot settings, request sequence, and relevant environment variables. Without that bundle, “same seed” is not a reproducible experiment.

## Quality policy

Section 31's relative perplexity tiers are proposals, not approved release gates: at most 0.1% for conversion validation, 1% for quality-focused artifacts, 3% for balanced artifacts, and 5% only for explicitly experimental artifacts. Section 78 must calibrate these against task behavior and model-family sensitivity before Section 81 adopts them.

MTP/speculative decoding needs two separate checks: accepted draft tokens must equal what the target model would have produced under the pinned decoding contract, and rejected drafts must leave target state equivalent to ordinary target decoding. Recurrent models additionally need rollback and save/restore tests at fragmented sequence boundaries.

## Failure semantics

A numerical, identity, checksum, schema, epoch, or continuation mismatch is a hard test failure. It may be classified as an expected cross-backend numerical difference only after the raw logits and complete replay identity demonstrate that it lies inside an approved, predeclared comparator.
