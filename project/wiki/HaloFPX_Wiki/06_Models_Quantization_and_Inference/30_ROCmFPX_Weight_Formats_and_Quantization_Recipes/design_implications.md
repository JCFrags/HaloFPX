---
section_id: "30"
title: "ROCmFPX recipe design implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX"]
  software_versions: ["a5605a7"]
  hardware_revisions: []
related_sections: ["29", "31", "33", "57"]
---

# Design implications

- **[RECOMMENDATION]** Recipe ID form: `<format>.<implementation-family>.<topology>.<tier>.vN`; record the converter/fork commit and emitted per-tensor type manifest.
- **[RECOMMENDATION]** Begin target-model qualification with `STRIX_LEAN` and a higher-quality Q6/Q8 reference, not Q3 or pure FAST. Promote only after Section 31 comparisons.
- **[RECOMMENDATION]** Keep embedding, output, attention K/V projections, FFN down, routers, shared experts, recurrent/SSM projections, multimodal projector, and MTP heads as explicit policy categories. Unknown categories default upward in precision or block conversion.
- **[INFERENCE]** MoE artifact BPW depends heavily on expert tensor routing, so “same preset” across dense and MoE is not the same policy contract.
- **[RECOMMENDATION]** Persistent-cache fingerprints include weight tensor hashes and the resolved recipe manifest. A cache produced by one quant artifact must not be restored against another.
- **[RECOMMENDATION]** Keep TurboQuant and all ROCmFP weight types opt-in for KV until per-architecture long-context quality and backend support pass. Upstream llama.cpp does not contain the Turbo types at its pinned commit.

Corrupt or unknown custom type IDs must fail load or cause cache miss; never reinterpret them as an upstream type.

