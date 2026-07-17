---
section_id: "78"
title: "Correctness, Regression, Determinism, and Model Quality Sources"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: ["Reproducible Builds definition accessed 2026-07-17"]
  hardware_revisions: []
related_sections: ["31", "54", "73", "81"]
---

# Sources

Accessed 2026-07-17. Repository references are pinned snapshots; their tests were inspected, not executed on HaloFPX.

| ID | Source and revision | Claims supported | Limitations/conflicts |
|---|---|---|---|
| S78-01 | [llama.cpp tests at `788e07d`](https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689/tests) | Unit-test inventory, tokenization, grammar, state, seed/sampler-related test coverage | Test presence is not target-machine pass evidence; specific runtime determinism remains untested |
| S78-02 | [`tests/test-backend-ops.cpp` at `788e07d`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tests/test-backend-ops.cpp) | Operation/type/backend-sensitive numerical comparators | Upstream tolerances are not automatically HaloFPX end-to-end quality gates |
| S78-03 | [`tools/perplexity/README.md` at `788e07d`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/perplexity/README.md) | PPL/KLD workflow, comparability warning, logit storage scale | Dataset/model policy and target results remain open |
| S78-04 | [`tools/server/tests/README.md` at `788e07d`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/tests/README.md) | Server pytest execution and slow-test controls | Does not cover the future HaloFPX distributed/cache contract |
| S78-05 | [ROCmFPX scripts at `a5605a7`](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394/scripts) | Fork regression/script inventory; `check-rocmfpx-all.sh` optional-test behavior; model-dependent skip paths | Script existence or aggregate exit is not required-case coverage or target pass evidence |
| S78-06 | [Reproducible Builds definition, accessed 2026-07-17](https://reproducible-builds.org/docs/definition/) | Reproducible-build definition | Build reproducibility does not establish runtime correctness |
| S78-07 | [Project Section 31, verified 2026-07-16](../../06_Models_Quantization_and_Inference/31_Conversion_Imatrix_Calibration_and_Quality_Validation/README.md) | Existing proposed relative-perplexity tiers | Candidate project recommendation, not an approved gate |
| S78-08 | [Project Section 56, verified 2026-07-16](../../09_HaloKV_Persistent_Cache/56_CachyLLama_Cache_Semantics_and_Porting_Map/README.md) | Cache/state identity and fail-closed guidance | Wiki synthesis; machine validation remains required |
