---
section_id: "02"
title: "Evidence, Citation, and Source Policy"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "ROCmFPX", "CachyLlama", "llama-ai", "llama.cpp"]
  software_versions: []
  hardware_revisions: []
related_sections: ["01", "04", "05"]
---

# Evidence, Citation, and Source Policy

**[RECOMMENDATION]** Every material conclusion must carry a claim label and a nearby source or experiment reference. A page-level bibliography without claim-to-source mapping is insufficient for implementation-sensitive statements.

## Evidence order

1. Exact source code or repository content at a full commit hash.
2. Official specification, standard, or versioned vendor documentation.
3. Peer-reviewed research paper or stable preprint, with limitations stated.
4. Official issue, pull request, or maintainer discussion.
5. Reproducible benchmark artifact with matched configuration.
6. Community report or secondary explanation, labeled as such.
7. Memory or prior-run experience, scoped and awaiting promotion.

**[VERIFIED]** Commit-addressed GitHub links identify exact file versions; branch links do not [S02-01]. **[VERIFIED]** W3C PROV models entities, activities, agents, primary sources, derivation, revision, and invalidation, which map cleanly to the project's evidence lineage [S02-02].

## Policy boundary

Verified vendor or repository claims establish what a source says, not that the claim holds on HaloFPX hardware. Performance, quality, cache correctness, and distributed failure behavior require project experiments under section [05](../05_Research_Data_and_Benchmark_Artifact_Conventions/README.md).

## Research split

- Internet/source-code research completed: permanent source addressing, provenance relationships, citation metadata, and claim terminology.
- Machine work required: snapshot exact checked-out commits, source licenses, build provenance, model hashes, and matched experiment artifacts.
- Contingent decisions: source snapshot retention, review cadence, and confidence scoring automation.

See [facts](facts_and_constraints.md), [implications](design_implications.md), [checks](procedures_and_checks.md), [questions](open_questions.md), and [source records](sources.md).
