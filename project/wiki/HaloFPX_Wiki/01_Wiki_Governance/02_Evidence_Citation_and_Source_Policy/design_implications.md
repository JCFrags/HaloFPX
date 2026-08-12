---
section_id: "02"
title: "Evidence Policy Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "ROCmFPX", "CachyLlama", "llama-ai", "llama.cpp"]
  software_versions: []
  hardware_revisions: []
related_sections: ["01", "04", "05"]
---

# Design implications

## Source identity

**[RECOMMENDATION]** Source IDs are stable within the wiki (`S<section>-NN`). A record contains publisher/repository, title, URL or canonical path, full commit/tag/document revision, publication date when available, access date, license, supported claims, and limitations. Mutable URLs must be accompanied by an immutable revision or a preserved snapshot checksum.

## Upstream snapshots observed 2026-07-16

| Repository | Observed HEAD | Policy consequence |
|---|---|---|
| `charlie12345/ROCmFPX` | `a5605a72768c6562241b248e268e33dc92787394` | Cite code/README at this hash, never `main` alone |
| `fewtarius/CachyLlama` | `6be745998f568e379ea197fcf827baec73ff9940` | Treat persistent-cache statements as repository claims until tested |
| `fewtarius/llama-ai` | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | Scope agent-serving claims to exact code |
| `ggml-org/llama.cpp` | `788e07dc91d266ad3162a1ce9037665656269689` | Track divergence and upstream semantics by commit |

These are **[VERIFIED]** remote HEAD observations made with `git ls-remote` on the access date, not approved project pins and not evidence that the code was built [S02-07, S02-08, S02-09, S02-10].

## Source lifecycle

```text
candidate -> reviewed -> active -> stale -> deprecated
                    \-> rejected
```

- **[RECOMMENDATION]** `active` means usable within recorded applicability, not permanently true.
- **[RECOMMENDATION]** Mark a source `stale` when its review trigger fires; do not erase supported historical facts.
- **[RECOMMENDATION]** `deprecated` requires a reason and replacement where known.
- **[RECOMMENDATION]** Contradictory active sources create a ledger item in section [04](../04_Assumption_Open_Question_and_Decision_Ledgers/README.md).

## Confidence guidance

| Confidence | Meaning |
|---|---|
| high | Direct, applicable primary evidence; no known conflict |
| medium | Good evidence with an applicability or completeness limitation |
| low | Indirect, preliminary, stale, or conflicting evidence |

**[INFERENCE]** Keeping confidence separate from claim type prevents a common failure: treating a precisely cited upstream promise as a measured local property.
