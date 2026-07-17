---
section_id: "82"
title: "Roadmap Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "CachyLLama@6be745998f568e379ea197fcf827baec73ff9940", "llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"]
  software_versions: ["roadmap baseline 2026-07-17"]
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["09", "18", "20", "29", "38", "48", "49", "55", "57", "71", "73", "74", "76", "77", "79", "80", "81", "83", "84", "85", "86"]
---

# Roadmap open questions

No question below is resolved by this roadmap. Owners are roles, not named people. `HLX-OQ-*` is the canonical [Section 03 namespace](../../01_Wiki_Governance/03_Glossary_Naming_and_Stable_Identifiers/design_implications.md); each retained `OQ82-*` token is an immutable section-local alias and must not be used alone outside Section 82.

| Canonical ID | Local alias | Question | Evidence/decision needed | Provisional owner | Blocks |
|---|---|---|---|---|---|
| `HLX-OQ-8201` | `OQ82-01` | **[OPEN]** Which target model, quantization, context and workload define MUP-1? | Sections 07,29-31 plus sponsor ADR | product/model | G82-20 |
| `HLX-OQ-8202` | `OQ82-02` | **[OPEN]** Does that admitted cell fit one node with safe concurrency headroom? | M82-04 | platform/model | MUP-1 and fallback |
| `HLX-OQ-8203` | `OQ82-03` | **[OPEN]** Which Section 09 candidate SLOs and quality tolerances become binding? | sponsor acceptance/change/rejection | product/verification | release gates |
| `HLX-OQ-8204` | `OQ82-04` | **[OPEN]** What node mismatches are acceptable for matched comparisons? | M82-01 and platform ADR | platform | G82-10 |
| `HLX-OQ-8205` | `OQ82-05` | **[OPEN]** Are the two USB4 paths independent, stable and useful simultaneously? | M82-07 | fabric/platform | G82-40 |
| `HLX-OQ-8206` | `OQ82-06` | **[OPEN]** Which baseline carrier and optional kernel path meet required semantics? | Sections 50-55, M82-07/08 | fabric/kernel | E82-41 |
| `HLX-OQ-8207` | `OQ82-07` | **[OPEN]** What framing, credit, integrity, authentication and reconnect contract is authoritative? | Section 53 and protocol ADR | distributed/security | E82-50 |
| `HLX-OQ-8208` | `OQ82-08` | **[OPEN]** Does any coupled mode beat replication for a ratified objective or enable an otherwise infeasible model? | M82-09/Section 76 | runtime/verification | G82-50 |
| `HLX-OQ-8209` | `OQ82-09` | **[OPEN]** Which one coupled mode, if any, becomes the first supported plan? | G82-50 review | architecture/product | E82-54 |
| `HLX-OQ-8210` | `OQ82-10` | **[OPEN]** What is the cross-section compatibility fingerprint and migration policy? | Sections 57,67,72 and ADR | cache/runtime/operations | G82-60 |
| `HLX-OQ-8211` | `OQ82-11` | **[OPEN]** Can all admitted attention, recurrent, MTP/speculative, sampler/RNG and grammar state be restored safely? | state inventory and M82-10 | model/cache | MUP-4 |
| `HLX-OQ-8212` | `OQ82-12` | **[OPEN]** Which durability tier and SSD write budget are acceptable? | M82-11, device warranty/endurance and product policy | storage/product | cache default |
| `HLX-OQ-8213` | `OQ82-13` | **[OPEN]** What authentication, tenant, trust-boundary and secrets policy governs local/LAN use? | Section 71 threat model | security/product | G82-70 |
| `HLX-OQ-8214` | `OQ82-14` | **[OPEN]** What clean deployment, backup, protocol/cache migration and rollback support window is promised? | Sections 70,72 | operations/release | G82-70 |
| `HLX-OQ-8215` | `OQ82-15` | **[OPEN]** What sample sizes, soak duration and regression budgets support release claims? | Sections 73-81 and sponsor risk decision | verification/product | G82-70 |
| `HLX-OQ-8216` | `OQ82-16` | **[OPEN]** Who owns each patch lane, hardware lab, gate review, incident response and upstream watch? | Sections 83-86 governance assignment | project lead | execution start |

## Resolution order

1. Resolve `HLX-OQ-8201`, `HLX-OQ-8202`, `HLX-OQ-8203`, and `HLX-OQ-8204` before claiming a matched MUP-1 baseline.
2. Resolve `HLX-OQ-8205`, `HLX-OQ-8206`, and `HLX-OQ-8207` before stabilizing a fabric ABI.
3. Resolve `HLX-OQ-8208` and `HLX-OQ-8209` with matched cache-off evidence; “none” is valid.
4. Resolve `HLX-OQ-8210`, `HLX-OQ-8211`, and `HLX-OQ-8212` before enabling persistent cache by default.
5. Resolve `HLX-OQ-8213`, `HLX-OQ-8214`, `HLX-OQ-8215`, and `HLX-OQ-8216` before release promotion.

**[RECOMMENDATION]** Link each resolution to an ADR and retain rejected alternatives, evidence, applicability and rollback. Do not close a question merely because implementation chose a default.
