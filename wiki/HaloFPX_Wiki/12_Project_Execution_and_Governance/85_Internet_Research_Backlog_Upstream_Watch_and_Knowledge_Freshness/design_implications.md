---
section_id: "85"
title: "Freshness and Upstream-Watch Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["HaloFPX wiki, future implementation repository, donor and upstream repositories"]
  software_versions: ["observed 2026-07-17 snapshot"]
  hardware_revisions: ["exact two-node revisions pending"]
related_sections: ["02", "04", "11", "15", "23", "29", "57", "72", "84", "86"]
---

# Design implications

## Separate watch state from product state

**[RECOMMENDATION]** Maintain three identities for every dependency:

1. `observed_head` - volatile discovery signal with checked-at time;
2. `candidate_revision` - immutable source under review;
3. `qualified_baseline` - immutable tuple with machine evidence and rollback record.

**[INFERENCE]** This prevents a fast-moving llama.cpp head, kernel RC, Mesa staging branch, or preview ROCm lane from silently redefining the product. Frozen Section 11 pins remain historical facts even after a feed advances.

## Compatibility is a tuple

**[RECOMMENDATION]** A qualification key must include at least:

```text
hardware/BIOS/firmware + distro/kernel/config/amdgpu + ROCm/HIP/HSA/RCCL
+ Mesa/RADV + compiler/build flags + HaloFPX commit/patch hash
+ model/tokenizer/template/quant hashes + transport/cache/protocol schemas
```

**[INFERENCE]** Independent “supported” labels cannot prove the combined Linux 7.2 USB4STREAM plus gfx1151 compute lane. Section 23's compatibility matrix, Section 50's carrier decision, and Section 72's rollback policy must consume one tuple ID rather than free-form version names.

## Evidence promotion and decision propagation

Use Agent Harness governance: source evidence is preserved before synthesis; published material becomes stale on a source/use signal and returns to candidate review [S85-17].

| Stage | Required record | Forbidden shortcut |
|---|---|---|
| Detect | feed ID, old/new locator, observed UTC, fetch status | treating “no notification” as no change |
| Preserve | immutable URL/SHA/tag, metadata and hash where possible | citing only a moving `latest` page |
| Diff | changed paths/symbols/docs plus semantic summary | promoting commit messages as runtime truth |
| Impact | affected claim IDs, section IDs, tuple IDs, ADRs, experiments | global search-and-replace of versions |
| Review | authority, conflict, applicability, severity, reviewer disposition | artifact self-approval |
| Revalidate | source tests and machine experiment IDs/raw evidence | importing upstream benchmark claims |
| Publish | append/revise with supersession link and last-verified date | silently overwriting verified history |
| Decide | ADR/decision status and rollback impact | automatic baseline upgrade |

**[RECOMMENDATION]** A material upstream change creates an impact record with one of: `no-applicability`, `documentation-only`, `candidate`, `blocks-decision`, `invalidates-claim`, or `security-response`. Claims stay queryable with their prior applicability and supersession reason.

## Local-to-canonical identifier policy

Section 03 defines the canonical [`HLX-OQ-NNNN` and `HLX-EXP-YYYYMMDD-NNN` namespaces](../../01_Wiki_Governance/03_Glossary_Naming_and_Stable_Identifiers/design_implications.md#stable-identifier-namespaces). Section 84 owns the [canonical on-machine experiment cards](../84_On_Machine_Research_Plan_Experiment_Cards_and_Lab_Notebook/procedures_and_checks.md#standard-experiment-card-contract).

| Section 85 form | Role | Canonicalization rule | Current mapping state |
|---|---|---|---|
| `OQ85-NN` | local open-question alias | Allocate or reuse a canonical `HLX-OQ-NNNN`, then record an explicit alias entry with scope and owner. Never derive the canonical digits by concatenating `85` and `NN`. | **[OPEN]** No local-to-canonical mappings are asserted here. |
| `EX85-NN` | local machine-evidence requirement alias | Link explicitly to one or more Section 84 `HLX-EXP-YYYYMMDD-NNN` cards whose protocol covers every required observation and control. Similar titles or numbers do not establish identity. | **[OPEN]** No local-to-card mappings are asserted here. |

**[RECOMMENDATION]** The alias registry must store `local_alias`, `canonical_id`, `scope`, `owner`, `valid_from`, and any `deprecated_at`/supersession. A canonical card may satisfy multiple local requirements only under Section 84's deduplication rule; one local requirement may need multiple cards, but every edge must be written explicitly. Until that registry exists, use the Section 85 IDs only for navigation and do not cite them as canonical records or executed experiments.

## Severity and service levels

| Class | Examples | Required response |
|---|---|---|
| P0 | exploitable advisory, corruption acceptance, tag/hash compromise, unsafe firmware | triage same day; mark affected claims/decisions blocked; preserve evidence; test rollback |
| P1 | correctness, ABI/schema/type-ID, model/tokenizer, kernel/driver support, communicator failure | impact review within 2 business days; no promotion until targeted validation |
| P2 | performance, new backend op, optional feature | weekly triage; qualify with matched A/B evidence |
| P3 | discovery or editorial change | monthly/quarterly review |

## Design consequences by subsystem

- **Repository integration:** **[RECOMMENDATION]** Diff relevant paths and patch dependencies, not repository-wide commit counts; ROCmFPX lacks a shared current Git ancestry with llama.cpp.
- **Custom formats/cache:** **[RECOMMENDATION]** Any type-ID, block-layout, state, tokenizer, runtime, rank/topology, or cache-schema change invalidates compatibility until recomputed. Corrupt or mismatched state must remain a miss/recomputation.
- **Kernel/transport:** **[RECOMMENDATION]** Track source, distro backport, `CONFIG_*`, module, and runtime capability separately. Kernel source presence is not machine availability.
- **ROCm/Mesa/RCCL:** **[RECOMMENDATION]** Never mix release lanes by numeric ordering. Preserve component commits because umbrella version labels may contain independently versioned components.
- **Models:** **[RECOMMENDATION]** Pin the full artifact set and license/gating state; a config-only update can change graph, state, chat, or conversion semantics.
- **Hardware errata:** **[OPEN]** No reliable product-specific feed can be selected until the exact OEM/SKU/board/BIOS/controller/retimer/cable inventory exists.

## New cross-project gaps revealed

1. **[OPEN]** No common claim-to-source-to-decision dependency index or freshness metadata schema spans the wiki.
2. **[OPEN]** No owner, credentials/rate-limit policy, durable feed snapshot store, or monitor-failure alert is defined.
3. **[OPEN]** The exact OEM authority and firmware rollback capability are unavailable because the BOM is incomplete.
4. **[OPEN]** Distro backport/config provenance is not represented as a first-class dependency alongside upstream kernel commits.
5. **[OPEN]** No signed-artifact policy covers Git tags, release tarballs, model weights, firmware, and container/package repositories consistently.
6. **[OPEN]** Model identity does not yet include a complete tokenizer/chat-template/license/weight manifest.
7. **[OPEN]** ROCm production and preview numbering discontinuity can defeat naive version automation.
8. **[OPEN]** No security-response owner or decision freeze/unfreeze procedure is recorded.
9. **[OPEN]** Feed authority can migrate: RCCL moved from `ROCm/rccl` to a `ROCm/rocm-systems` subtree, while a naive default-branch watcher would continue polling a deprecated repository.
10. **[OPEN]** Provider convenience endpoints can disagree: llama.cpp's `/releases/latest` lagged its ordered release feed during this observation.

These are proposals/gaps, not silent changes to other sections.
