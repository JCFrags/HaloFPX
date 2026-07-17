---
section_id: "31"
title: "Conversion quality design implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX"]
  software_versions: ["a5605a7"]
  hardware_revisions: []
related_sections: ["29", "30", "57", "78"]
---

# Design implications

- **[RECOMMENDATION]** Treat BF16/F16 GGUF as a validated intermediate and rollback point, not merely a temporary file.
- **[RECOMMENDATION]** Calibration data must represent actual coding, tool JSON, long documents, multilingual text, and reasoning/chat templates while remaining legally distributable. Record mixture weights and token counts.
- **[INFERENCE]** A single generic web-text imatrix can under-exercise routers, tool syntax, rare code tokens, recurrent paths, or multimodal projectors. Use stratified imatrices or merge separately recorded runs.
- **[RECOMMENDATION]** Compare three layers: publisher/source framework vs BF16 GGUF (conversion correctness), BF16 GGUF vs quant (quant loss), and backend A vs backend B (kernel/runtime correctness).
- **[RECOMMENDATION]** Use deterministic corpus slices for gates and a held-out corpus for reporting; never tune tensor protections on the final test set.
- **[RECOMMENDATION]** Persistent state identity includes source and output hashes, tokenizer/template, runtime commit, quant manifest, cache type, RoPE/scaling, and topology. Any mismatch is a cache miss.

# Proposed acceptance tiers

These are **[RECOMMENDATION]** starting thresholds, not measured facts; Section 78 should approve them after baseline variance is known.

| Gate | Conversion BF16 | Quality-first quant | Balanced quant | Experimental low-bit |
|---|---:|---:|---:|---:|
| tokenizer probe mismatch | 0 | 0 | 0 | 0 |
| missing/unexplained tensors or non-finite logits | 0 | 0 | 0 | 0 |
| relative PPL increase vs BF16 on fixed corpus | <=0.1% | <=1% | <=3% | <=5%, explicitly experimental |
| mean token-distribution KLD | establish numeric tolerance per architecture | no worse than approved reference | no worse than approved reference | report, do not silently promote |
| schema-valid tool calls on deterministic suite | 100% syntax validity | 100%; task success within 1 percentage point of BF16 | 100%; within 2 points | report only |
| deterministic state round-trip | exact continuation at same backend/settings | exact | exact | exact |

Small evaluation samples must include confidence intervals or paired outcomes; a threshold without variance is not sufficient evidence.

