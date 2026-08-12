---
type: remediation-closeout
status: complete
target: "safe P2 findings from reviews/2026-07-17__canonical-wiki-integrity__review__v01.md"
created: 2026-07-17
last_updated: 2026-07-17
risk: medium
approval_required: human
canonical_promotion: not-authorized
---

# Canonical Wiki integrity remediation closeout

Decision: **the authorized semantic P2 remediation is complete and validator-clean**. This pass repaired stale navigation, one cross-section storage contradiction, nondeterministic source-range citations, and nonportable Markdown anchors. It did not approve a release baseline, change a source pin, change any `section.yaml` status/applicability record, create a Git commit, or resolve the audit's P1 provenance findings.

## Authority and scope

Before editing, this pass re-read the project/Wiki/Agent Harness authorities, the relevant generated manifest entries, Section 04's decision-ledger boundary, Section 21's live storage evidence and manifest, and Section 83's risk authority and manifest. Source records were preserved; the remediation changed only synthesis/index text and portable navigation markup.

## Applied remediation

### Category 05-07 indexes

- Replaced the three stale `Research status: pending.` statements with scoped source-backed-draft summaries.
- Added direct links to all 25 child sections.
- Kept every child section explicitly bounded by `needs-machine-validation`; no backend, model, distributed mode, tuning profile, or performance result was promoted.

### Section 83 storage-risk reconciliation

- Replaced the obsolete claim that model, firmware, filesystem, and SMART state were unrecorded.
- Recorded the Section 21 live inventory as environment-scoped `[MEASURED]` evidence: Crucial P310 `CT1000P310SSD8`, firmware `VACR001`, Btrfs state, capacity/headroom, and SMART baselines.
- Preserved the real `[OPEN]` boundary: vendor TBW/DWPD/warranty, PCIe/queue qualification, workload write amplification, volatile-write-cache/PLP behavior, and application-level power-loss recovery.
- Updated source record `S83-04` to describe Section 21's 2026-07-17 state and its limitations. `S83-11` continues to own the cache-write/endurance gap.

### Source citations

- Replaced all **21 nondeterministic source-range citations across 16 files** with explicit, existing source IDs.
- Covered mixed range forms: `..`, shortened end IDs, `through`, Unicode dash ranges, and duplicated prefixes such as `S86-25-S86-28`.
- Left experiment/open-question ranges unchanged because they are navigation to registered non-source records, not source citations and were outside this repair directive.

Post-remediation source-range count: **0**.

### Portable anchors and fragments

- A precise pre-remediation recount found 36 `{#custom-id}` heading attributes in Sections 13 and 57; the original audit review was corrected from 38 to 36.
- Replaced all 36 unsupported attributes with explicit `<a id="..."></a>` anchors immediately before their headings, preserving the intended stable fragment IDs.
- All six previously unresolved fragment links now resolve under the GFM/CommonMark-oriented checker.

Post-remediation custom-heading-attribute count: **0**.

## Validation evidence

| Check | Result |
|---|---|
| Deterministic manifest check | pass; `manifest.yaml` still matches registry and section manifests |
| Manifest JSON Schema | pass |
| Wiki structural/schema validation | 86/86 complete and schema-valid; 0 missing/incomplete/invalid |
| Validator unit tests | 3/3 pass |
| Experiment-card validation | 10 valid; 0 errors |
| Markdown local-file links | 624 checked; 0 missing |
| Markdown fragment links | 65 checked; 0 unresolved |
| Declared source IDs | 728 |
| Bracketed direct source-reference occurrences | 1,277 checked; 0 unresolved |
| Nondeterministic source-range citations | 0 remaining |
| Unsupported `{#...}` heading attributes | 0 remaining |
| Retained evidence hashes | 15/15 match; no preserved evidence changed |
| Stale category pending markers | 0 remaining in categories 05-07 |
| Obsolete Section 83 storage-absence sentence | 0 remaining |

No `section.yaml` file changed, so manifest regeneration was unnecessary; the required deterministic `--check` passed. The root repository still has no commits and all project content remains untracked, so `git diff --check` cannot prove whitespace integrity for these files and is not represented as meaningful coverage.

## Residual findings and handoff

The original audit's P1 findings remain open and intentionally untouched:

1. Establish a reviewed parent-repository baseline and safe nested-repository representation before staging or implementation.
2. Add a provenance manifest and exact pinned source views for the repository clones; do not plan from moving clone `HEAD`s.

The validator-coverage proposal also remains useful: promote the ad hoc citation, fragment, evidence-hash, category-index, and repository-reference checks into deterministic maintained tooling. This remediation proves the current pages pass those checks; it does not yet make the checks part of normal CI.

Final disposition: **safe P2 semantic defects remediated; canonical Wiki remains draft/needs-machine-validation and P1 provenance gates still block implementation-baseline promotion**.
