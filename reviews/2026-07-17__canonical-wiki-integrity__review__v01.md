---
type: canonical-integrity-review
status: complete
target: "wiki/HaloFPX_Wiki and its current source/provenance controls"
created: 2026-07-17
last_updated: 2026-07-17
risk: high
approval_required: human
mode: "audit-only; no canonical Wiki or source artifact modified"
---

# Canonical HaloFPX Wiki integrity review

Decision: **revise governance and stale metadata before promoting the new research intake**. The canonical Wiki is structurally complete and its retained evidence hashes are intact, but it is not yet a versioned, citation-deterministic baseline. No canonical Wiki, source, manifest, or repository checkout was changed by this review; this review file is the only artifact created.

## Scope and authority

This audit read the project `AGENTS.md`, root `README.md`, `PROJECT_GOAL.md`, Wiki root/category indexes, `references/agent-harness.md`, and the canonical Agent Harness `AGENTS.md`, `guide/architecture.md`, and `reviews/AGENTS.md`. It inspected the current Git state, canonical section artifacts, manifest/schema, source and open-question counts, local Markdown links and fragments, citation identifiers, experiment cards, retained evidence hashes, and the newly created repository-reference clones.

Severity:

- **P1 - blocking:** change attribution, source identity, or rollback is not reliable enough for implementation planning or promotion.
- **P2 - material:** the Wiki remains usable as draft research, but retrieval, citation resolution, or cross-section state is stale or ambiguous.
- **P3 - minor:** presentation or automation coverage should be improved without blocking research review by itself.

## P1 findings

### P1-1 - the project has no versioned baseline, so concurrent edits cannot be attributed or rolled back

Evidence captured on 2026-07-17:

```text
## No commits yet on main
?? AGENTS.md
?? PROJECT_GOAL.md
?? README.md
?? references/
?? research/
?? reviews/
?? sources/
?? wiki/
tracked files: 0
top-level .gitignore: absent
```

`git diff --check` exits successfully, but that result is vacuous for the present workspace because every project artifact is untracked. It does not inspect the untracked Wiki, sources, imports, reviews, or nested repositories. With no commit graph, a later reviewer cannot distinguish original intake, prior promotion, current intake, or concurrent-agent edits using Git evidence.

Impact: implementation-plan review cannot cite a reproducible project state; rollback and blame are unavailable; a mistaken bulk stage could also treat nested Git repositories as embedded repositories rather than ordinary evidence.

Safe remediation:

1. Finish and independently review the current import/promotion wave.
2. Decide and document how `sources/repositories/*/.git` is represented in the parent repository before staging anything: excluded working clones plus a provenance manifest, pinned bundles, or deliberate submodules. Do not accidentally record unexplained gitlinks.
3. Add a narrowly scoped ignore policy for generated caches and noncanonical clone internals as appropriate.
4. Create a reviewed baseline commit whose receipt names the 12 imported archives, canonical Wiki validation result, repository-reference manifest, and known unresolved findings.
5. Require `git status` before/after each later agent lane and prohibit claiming `git diff --check` coverage over untracked files.

Predates this intake: **yes** for the absence of any commit/tracked baseline; the earlier validated workspace was already recorded as untracked. The newly cloned nested repositories are concurrent with this intake and increase the staging risk.

### P1-2 - two reference clone `HEAD`s differ from the Wiki research pins and have no local provenance boundary

Current clean clone state:

| Reference clone | Current `HEAD` | Wiki/project research pin | Pin object present locally |
|---|---|---|---|
| `sources/repositories/charlie12345__rocmfpx` | `61f2f2d7bc4955e9bca821095ef69125837133b5` | `a5605a72768c6562241b248e268e33dc92787394` | yes |
| `sources/repositories/fewtarius__llama-ai` | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | same | yes |
| `sources/repositories/fewtarius__cachyllama` | `6be745998f568e379ea197fcf827baec73ff9940` | same | yes |
| `sources/repositories/ggml-org__llama.cpp` | `6bdd77f13cf11b264b4231d320afc404f48d576e` | `788e07dc91d266ad3162a1ce9037665656269689` | yes |

All four working trees were clean and their remotes pointed to the expected GitHub repositories. However, no repository-reference manifest or receipt was present beside the clones at audit time, and the parent workspace has no `.gitignore` or baseline commit. A plan author opening `HEAD` would inspect different ROCmFPX and llama.cpp content than the currently cited Wiki baseline.

Impact: source-code claims, file paths, conflict heat, test surfaces, and the fork plan can silently mix two observation epochs. This is especially dangerous for ROCmFPX, the requested canonical base.

Safe remediation:

1. Create a `sources/repositories` provenance manifest recording URL, default branch, fetch timestamp, observed remote head, research pin, object availability, license path, and intended role.
2. Provide clearly named detached worktrees or equivalent read-only views at every research pin; keep moving-head discovery separate and visibly noncanonical.
3. Make all plan/code citations name an exact commit and repository path; never cite clone `HEAD` alone.
4. Re-run source inventories and contradiction review before intentionally advancing a research pin.

Predates this intake: **no**. Clone creation times were approximately 13:49 local, after the further-research import tree was created around 13:46. This is a current concurrent-work hazard, not evidence that the earlier Wiki research was wrong.

## P2 findings

### P2-1 - three category indexes still claim research is pending

Evidence:

- `wiki/HaloFPX_Wiki/05_Performance_Software_and_Tools/README.md:5`
- `wiki/HaloFPX_Wiki/06_Models_Quantization_and_Inference/README.md:5`
- `wiki/HaloFPX_Wiki/07_Distributed_Runtime/README.md:5`

Each says `Research status: pending.` even though all 25 child sections are present, their seven required artifacts exist, and their `section.yaml` records validate. Unlike categories 01-04 and 08-12, these indexes also provide no linked authority/status summary.

Impact: top-down retrieval falsely suggests that the exact performance, model, and distributed-runtime material needed for the fork plan does not exist.

Safe remediation: regenerate these category indexes from the registry/section manifests or manually add linked section summaries, current draft boundary, unresolved machine gates, and cross-category authorities. Do not imply acceptance or machine validation.

Predates this intake: **yes**. The files were last changed at 05:34 local, before the 13:46 follow-up intake.

### P2-2 - Section 83 contradicts the current measured storage inventory

Section 83 still states at `facts_and_constraints.md:31` that exact installed NVMe models, firmware, filesystem, and SMART state "are not recorded." Section 21 now records:

- Crucial P310 `CT1000P310SSD8` and firmware `VACR001` at `README.md:17` and `facts_and_constraints.md:33-34`;
- Btrfs filesystem/capacity state at `README.md:17` and `facts_and_constraints.md:42,46-47`;
- SMART baselines at `facts_and_constraints.md:41`.

TBW warranty, device write amplification, and power-loss behavior correctly remain open. The Section 83 sentence incorrectly bundles resolved and unresolved fields.

Impact: the risk register overstates missing discovery evidence and can cause duplicate machine work or obscure the real remaining storage risks.

Safe remediation: split the risk statement into measured inventory fields and still-open vendor/endurance/durability fields, cite Section 21's live source, and update the risk trigger/mitigation without lowering the unresolved power-loss/cache-corruption gate.

Predates this intake: **yes, but arose from prior concurrent promotion**. Section 83 was last changed at 00:32, while the live inventory and Section 21 promotion were completed later on 2026-07-17; both precede the 13:46 intake.

### P2-3 - source citation ranges remain nondeterministic

The audit found **21 source-range citation occurrences across 16 files** using mixed forms such as:

- `[S28-01..S28-09]` (`05.../28.../README.md:17`);
- `[S56-01..S56-06]` (`09.../56.../README.md:17`);
- `[S86-14–S86-18]` and `[S86-25-S86-28]` (`12.../86.../README.md:23,50`).

The last form is parsed as the nonexistent identifier `S86-25-S86-28`; no formal range grammar exists in the validator. The Wiki also uses `through` and shortened end IDs. Directly enumerated source IDs resolved in the audit; the unresolved problem is the range notation itself.

Impact: deterministic dangling-citation validation cannot determine exactly which records support a claim, and range endpoints can hide missing intermediate records.

Safe remediation: enumerate every source ID in claim citations, or adopt one formally specified range grammar and extend tests/validation before using it. Add a citation validator that distinguishes source IDs from experiment/open-question IDs.

Predates this intake: **yes**. Representative affected pages and the Section 86 remediation were last changed before the 13:46 intake. This also means the earlier review's statement that no stale citation-range tokens remained was too narrow.

### P2-4 - custom Markdown heading attributes produce unresolved fragments under GFM/CommonMark

A precise pre-remediation recount found **36 headings** using `{#custom-id}` syntax in Sections 13 and 57. A GFM-style fragment audit checked 65 local fragment links and found **6 unresolved**:

- Section 57's README links to `#s57-facts`, `#s57-design`, `#s57-procedures`, `#s57-open`, and `#s57-sources`;
- Section 13 procedures links to `facts_and_constraints.md#s13-patches`.

GFM/CommonMark renders `{#...}` as heading text rather than declaring that custom anchor, so the intended fragments do not exist. Ordinary local-file resolution still passes because the target files exist.

Impact: deep navigation fails in common renderers and a file-only link checker reports a false clean result.

Safe remediation: use renderer-generated slugs or explicit portable HTML anchors, update links, and add fragment resolution to the standard link check.

Predates this intake: **yes**. Representative Section 13 and 57 pages were last changed at 12:16 and 00:08, respectively, before the 13:46 intake.

### P2-5 - the standard validators do not cover the semantic failures above

`validate_wiki.py` correctly checks required artifacts, registry identity, status/date/value shapes, and declared source/open-question counts. `generate_wiki_manifest.py --check` correctly checks its documented structural scope. Neither validates category-index freshness, citation targets/ranges, Markdown fragments, cross-section resolved/open contradictions, evidence hashes, or Git coverage.

Impact: a fully green standard run can coexist with stale authority indexes, dangling semantic citations, broken fragments, and contradictory risk state.

Safe remediation: add separate deterministic validators for citation IDs, local files plus fragments, category status/index generation, evidence hash manifests, and repository-reference manifests. Keep semantic contradiction review as an independent human/agent gate rather than pretending schema validation can prove it.

Predates this intake: **yes**. These coverage gaps existed in the earlier 86/86 validation baseline.

## Positive integrity results

| Check | Result |
|---|---|
| Deterministic manifest check | pass; `manifest.yaml` matches registry and section manifests |
| Manifest JSON Schema | pass |
| Required section artifacts | 86/86 complete; 0 missing/incomplete |
| Section metadata/count validation | 86/86 schema-valid |
| Validator unit tests | 3/3 pass |
| Declared source records | 728 total; all section-declared counts match |
| Declared open questions | 873 total; all section-declared counts match |
| Markdown local-file links | 599 checked; 0 missing target files |
| Markdown fragments | 65 checked; 6 unresolved, documented above |
| Experiment cards | 10 valid; 0 errors |
| Retained evidence hashes | 15/15 match: six 2026-07-17 live-inventory files and nine preserved 2026-07-10/12 cluster files |
| Section status boundary | all 86 remain `needs-machine-validation`; no structural result was mistaken for approval |
| Reference research-pin objects | all four exact project research pins are present in the local clones |

No fabricated benchmark or target-machine performance claim was discovered in this scoped audit. External HTTP availability and substantive truth of all 728 source records were not re-fetched; this was an integrity audit of the current canonical workspace, not a new Internet-research campaign.

## Remediation order before new promotion

1. Freeze the parent-repository and nested-clone representation, then record an explicit baseline/provenance manifest.
2. Expose exact pinned source worktrees/views and require the fork plan to cite them, not moving clone heads.
3. Repair the Section 83 storage contradiction and category 05-07 indexes.
4. Normalize source citations and repair custom fragments.
5. Add deterministic citation/fragment/category/hash/repository-manifest checks.
6. Re-run all positive checks above, review the 12 new research packages against exact pins, and only then promote claims or approve the fork plan.

Final disposition: **canonical Wiki content is structurally intact but promotion-blocked by unversioned project state and ambiguous source-checkout identity**. The remaining P2 defects are bounded and safely repairable without discarding any source material.
