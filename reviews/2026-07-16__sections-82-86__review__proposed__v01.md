---
type: improvement-proposal
status: proposed
target: "wiki/HaloFPX_Wiki/12_Project_Execution_and_Governance sections 82-86 and README.md"
created: 2026-07-17
last_updated: 2026-07-17
risk: high
approval_required: human
remediation_status: applied-pending-human-approval
---

# Adversarial review: Sections 82-86

Decision: **revise before promotion**. The pages are useful draft governance material and pass the structural validator, but the major findings below prevent treating the category as an executable, internally consistent control plane.

## Scope and method

Reviewed the category README and every required file in Sections 82-86 against the five standalone prompts, `OUTPUT_STANDARD.md`, project `AGENTS.md`, the Agent Harness evidence/review rules, current primary-source authority, completed earlier Wiki sections, executable-governance expectations, and the no-fabricated-measurement rule. The review pass was read-only for Wiki material.

Severity means:

- **P1 - major:** blocks promotion or reliable automation because authority, dependency order, provenance, or execution semantics are inconsistent.
- **P2 - medium:** does not invalidate the draft, but causes stale retrieval, dangling machine references, or ambiguous implementation.

## P1 - major findings

### 1. Stable identifiers conflict with the Wiki's naming authority

Evidence:

- Section 03 recommends `HLX-OQ-NNNN`, `HLX-EXP-YYYYMMDD-NNN`, and `HLX-RUN-*`: `wiki/HaloFPX_Wiki/01_Wiki_Governance/03_Glossary_Naming_and_Stable_Identifiers/design_implications.md:17-26`.
- Section 82 uses `OQ82-*` and `M82-*`: `82_Implementation_Roadmap_Epics_Dependencies_and_Exit_Criteria/open_questions.md:19-34` and `section.yaml:10-21`.
- Section 83 uses `OQ83-*`, `R83-*`, and `M83-*`: `83_Risk_Register_Failure_Modes_Mitigations_and_Contingencies/open_questions.md:17-31`, `facts_and_constraints.md:40-57`, and `section.yaml:9-18`.
- Section 85 uses `OQ85-*` and `EX85-*`: `85_Internet_Research_Backlog_Upstream_Watch_and_Knowledge_Freshness/open_questions.md:17-34` and `section.yaml:10-20`.
- Section 86 uses `OQ-86-*` and `EXP-86-*`: `86_Issues_Labels_Milestones_ADRs_Code_Review_and_Contribution_Process/open_questions.md:17-31` and `section.yaml:10-13`, despite instructing contributors to link immutable Section-03 IDs in `design_implications.md:15-17`.

Impact: records cannot be joined deterministically across roadmap, risk, experiment, watch, issue, and ADR systems; later migration can break external links.

Required revision: approve a namespace/alias ADR before operational use. Either migrate to canonical `HLX-*` records or explicitly register every section-local identifier as an immutable alias with collision and migration tests.

### 2. Section 84 describes card families but does not supply executable standardized cards

Evidence:

- `84_On_Machine_Research_Plan_Experiment_Cards_and_Lab_Notebook/README.md:53` calls ten definitions "executable card families."
- `procedures_and_checks.md:15` says paths and interfaces remain placeholders.
- `procedures_and_checks.md:17-21` defines the card contract only as prose; there is no `card.yaml` template, schema, instance set, validator, or deterministic unresolved-field handling.
- The families at `procedures_and_checks.md:57-119` mostly name broad tool classes rather than fully resolved argv and environment snapshots.

Impact: the physical research sequence cannot yet be scheduled or machine-validated without an operator inventing fields, commands, paths, controls, and acceptance behavior.

Required revision: add a canonical card schema/template and instantiate all ten definitions. Explicit unknowns should be `null`, not omitted. Add deterministic validation and map each Section 82/83/85/86 experiment request to its owning Section 84 card.

### 3. Sections 82 and 84 disagree on whether HaloKV blocks distributed-mode evidence

Evidence:

- `82_Implementation_Roadmap_Epics_Dependencies_and_Exit_Criteria/README.md:38-40` places coupled-mode selection in P5 before HaloKV in P6.
- `84_On_Machine_Research_Plan_Experiment_Cards_and_Lab_Notebook/design_implications.md:26-30` and `section.yaml:24-29` make distributed experiment E07 depend on HaloKV experiment E06.
- `82.../design_implications.md:34-37` also makes integrated verification unconditionally depend on HaloKV despite earlier minimum useful products and cache-disabled fallbacks.

Impact: transport and distributed-mode break-even evidence is unnecessarily blocked on persistent-cache correctness, contradicting the roadmap and delaying a valid cache-off replication/coupled-mode decision.

Required revision: split cache-off distributed qualification from cache-integrated qualification. Make release dependencies conditional on admitted features, while keeping corrupt or incompatible cache acceptance a hard failure whenever caching is enabled.

### 4. Section 85's liburing 2.15 record points to 2.14 authority

Evidence:

- `85_Internet_Research_Backlog_Upstream_Watch_and_Knowledge_Freshness/sources.md:104-109` declares liburing 2.15 and peeled commit `d41bf922...`, but links the 2.14 release and tree `e3d35ea...`.
- `procedures_and_checks.md:50-56` still queries `refs/tags/liburing-2.14*`.
- Primary Git authority resolves liburing 2.15 to annotated tag object `84bb497...` and peeled commit `d41bf922...`; 2.14 peels to `e3d35ea...`.

Impact: the watch ledger can report a current version while fetching and verifying different source bytes.

Required revision: replace the stale release/tree URLs and watch command with 2.15 identifiers, preserving both annotated-tag and peeled commit IDs.

### 5. Section 86 is stale against the completed category

Evidence:

- `86_Issues_Labels_Milestones_ADRs_Code_Review_and_Contribution_Process/facts_and_constraints.md:60` says Sections 82-85 are not yet integrated.
- All four sections are now present and structurally complete, and Section 86 cites them elsewhere.
- `open_questions.md:31` still leaves authoritative input selection unresolved without recording a completed reconciliation.

Impact: the proposed issue/PR/ADR process does not yet identify which roadmap, risk, experiment, and freshness records are its controlling inputs.

Required revision: perform and record the cross-section contract reconciliation. Replace the obsolete availability gap with the real remaining acceptance or ownership decision.

### 6. Section 82 misuses the `[MEASURED]` label

Evidence: `82_Implementation_Roadmap_Epics_Dependencies_and_Exit_Criteria/facts_and_constraints.md:64` labels "No HaloFPX measurement was generated" as `[MEASURED]`.

Impact: `[MEASURED]` is reserved for reproducible results linked to raw data and environment metadata. Using it for absence weakens automated and human evidence promotion rules.

Required revision: relabel the statement `[VERIFIED]` with an artifact/manifest audit, or `[OPEN]` if completeness cannot be established.

## P2 - medium findings

### 7. The category README remains stale and is not a useful authority index

Evidence: `wiki/HaloFPX_Wiki/12_Project_Execution_and_Governance/README.md:5` says `Research status: pending`; lines 7-11 are plain text without links, section statuses, authority boundaries, or cross-category dependencies.

Required revision: update the status and add linked summaries. State clearly that structural completion does not imply machine validation or accepted project policy.

### 8. Section 85 uses fragment syntax unsupported by GitHub/CommonMark

Evidence:

- `85.../README.md:36` links `#s85-promotion` and `#s85-propagation`.
- Targets use headings such as `## Safe watch cycle {#s85-promotion}` at `procedures_and_checks.md:15` and a matching form in `design_implications.md:37`.

Impact: ordinary GFM/CommonMark renderers treat `{#...}` as visible heading text rather than a custom anchor, so the intended fragments do not resolve.

Required revision: use renderer-generated heading slugs or explicit portable HTML anchors.

### 9. Citation ranges are not defined source IDs

Evidence: `82.../README.md:21` uses `[S82-01..S82-06]`; `85.../README.md:23-27` uses forms such as `[S85-02-S85-05]` and `[S85-06-S85-13, S85-16]`.

Impact: these strings do not identify source records and defeat deterministic dangling-citation validation.

Required revision: enumerate source IDs or define and validate one formal range syntax.

### 10. ROCm lane terminology is not reconciled

Evidence:

- `83_Risk_Register_Failure_Modes_Mitigations_and_Contingencies/README.md:13-14` and `section.yaml:27-28` call ROCm 7.2.1 the Ryzen-supported control and ROCm 7.14.0 a broader qualification candidate.
- `85_Internet_Research_Backlog_Upstream_Watch_and_Knowledge_Freshness/README.md:13,25` and `section.yaml:37` call ROCm 7.2.3 the research/qualification baseline.

Impact: "supported control," "research baseline," "qualification baseline," installed tuple, and candidate lane can be mistaken for interchangeable states.

Required revision: add a lane-aware baseline table covering vendor-supported Ryzen tuple, frozen research baseline, installed tuple, and unqualified candidates.

## Positive checks

- `validate_wiki.py` reports 86 complete, 0 incomplete, and 0 missing.
- Each reviewed section contains exactly the seven required files.
- Declared source and open-question counts match their visible records.
- Relative file links resolve. The Section 85 issue is fragment semantics, not a missing file.
- Historical Section 83 `[MEASURED]` USB4 statements resolve through Section 20 to preserved raw logs, environment state, and checksum receipts and remain scoped.
- No fabricated benchmark, target-machine performance result, or universalized repository claim was found.

## Applied remediation - 2026-07-17

The coordinating agent applied the authorized mechanical and major revisions after this review was recorded. The Wiki remains a draft and no human-owned project decision was inferred.

| Finding | Remediation | Status | Residual gate |
|---|---|---|---|
| P1-1 identifier conflict | Added canonical/candidate `HLX-OQ-*` crosswalks and Section 84's 32-entry experiment-alias registry | Applied | Human approval of aliases and a canonical risk namespace |
| P1-2 non-executable cards | Added schema, template, validator, and ten explicit YAML cards | Applied | Resolve and authorize explicit `null` fields before any run |
| P1-3 cache dependency inversion | Split cache-off E07 from conditionally cache-integrated E07 | Applied | Machine evidence for each admitted path |
| P1-4 liburing authority mismatch | Pinned 2.15 URLs, annotated tag, peeled commit, and watch query | Applied | Normal future feed revalidation |
| P1-5 stale Section 86 integration | Integrated Sections 82-85 as authoritative drafts | Applied | Human adoption, owners, forge, policies, and deviation authority |
| P1-6 invalid `[MEASURED]` absence | Relabeled as `[VERIFIED]` artifact/manifest audit | Applied | Run the named experiments before any performance promotion |
| P2-7 stale category index | Added linked authority/status/dependency summary | Applied | Generate category status automatically in future |
| P2-8 broken fragments | Replaced custom heading attributes with portable fragments | Applied | Retain fragment checks in link validation |
| P2-9 synthetic citation ranges | Enumerated source IDs | Applied | Add deterministic dangling-citation validation |
| P2-10 ROCm lane ambiguity | Added aligned 7.2.1/7.2.3/7.14.0/installed-tuple vocabulary | Applied | Capture the actual installed tuple on both nodes |

1. **Identifiers:** Sections 82, 83, 85, and 86 now publish canonical/candidate `HLX-OQ-*` crosswalks and retain their former section-local IDs as aliases. Section 84 owns the canonical `HLX-EXP-*` cards and a validated 32-alias mapping for the `M82`, `M83`, `EX85`, and `EXP-86` requests. False one-to-one mappings remain explicitly open. `R83-*` stays local because Section 03 has no approved risk namespace; external use remains blocked pending the naming-authority decision.
2. **Executable cards:** Section 84 now includes `experiment-card.schema.json`, `experiment-card.template.yaml`, ten explicit YAML cards under `cards/`, `experiment-aliases.yaml`, and `validate_experiment_cards.py`. Every required field exists; unresolved owner, command, authorization, threshold, analysis, and rollback values are explicit `null` values in draft cards.
3. **Dependency order:** Sections 82 and 84 now separate mandatory cache-off distributed qualification from optional cache-integrated qualification. Cache-off E07 depends on E03-E05; HaloKV E06 is conditional only for cache-integrated claims and admitted cache release scope.
4. **liburing authority:** Section 85 now points to liburing 2.15, annotated tag object `84bb497ca2f9d24ca0b9e5646fb6a05e72c0f04e`, peeled commit `d41bf9220ec39277ff235379e9089d9e0fd6c2a5`, and the matching 2.15 watch query.
5. **Section integration:** Section 86 now consumes Sections 82-85 as authoritative draft inputs while leaving human adoption, owners, forge, rules, policies, thresholds, exceptions, and release acceptance open. The stale availability statement was removed.
6. **Claim discipline:** Section 82 now records the absence of measurements as a `[VERIFIED]` artifact/manifest audit rather than a `[MEASURED]` result.
7. **Retrieval and citations:** The category README now links all five authoritative drafts and states the validation boundary. Section 85 uses portable heading fragments, and Sections 82/85 enumerate source IDs instead of synthetic range tokens.
8. **ROCm vocabulary:** Sections 83 and 85 now distinguish the Ryzen-supported 7.2.1 control, frozen/research 7.2.3 baseline, broader unqualified Core SDK 7.14.0 candidate, and still-open installed two-node tuple.
9. **Generated authority:** `manifest.yaml` was regenerated after the section-manifest changes and its deterministic `--check` mode passes.

Post-remediation validation results:

- experiment cards: 10 valid, 0 errors; alias map: 32 entries, reverse-coherent;
- required section artifacts: 86 complete, 0 incomplete, 0 missing;
- section schemas: 86 valid, 0 invalid, including Sections 82-86;
- source/open-question counts: 82 `10/16`, 83 `22/15`, 84 `11/12`, 85 `17/18`, 86 `28/15`;
- Markdown front matter and YAML parse; relative file links resolve; repaired fragment targets resolve;
- no stale citation-range tokens, liburing 2.14 watch reference, invalid no-measurement label, stale category status, or stale Section 86 integration statement remains;
- `manifest.yaml --check` and scoped `git diff --check` pass.

Residual disposition: **major mechanical findings remediated; human approval still required**. Draft cards are intentionally non-runnable until their `null` fields are resolved and authorized. Canonical risk IDs, human roles, repository/forge, security and contribution policy, acceptance thresholds, exception authority, and release adoption remain open.

## New research and implementation gaps

1. Human approval of the canonical OQ aliases plus an approved risk/requirement/release namespace and collision/dangling-reference validator.
2. Resolve and authorize every `null` card field, then rehearse the card validator and notebook on a disposable no-op fixture before physical experiments.
3. Reconciled ROCm lane vocabulary backed by actual installed-tuple receipts from both nodes.
4. Named human governance roles, canonical implementation forge/repository, and a tested enforcement fixture.
5. Feed-record validator for release/tag URLs, annotated and peeled IDs, fragment targets, and source citations.
6. Generated category authority/status indexes so concurrently completed sections cannot leave stale navigation.

## Proposed revision order and validation

1. Resolve the identifier and dependency ADRs first.
2. Add executable Section 84 card artifacts and map all experiment aliases.
3. Repair Section 85 source authority, anchors, and citations.
4. Reconcile Section 86 and regenerate the category index.
5. Correct claim labels and ROCm lane vocabulary.
6. Rerun YAML/front-matter validation, exact-file checks, source/OQ/dangling-ID checks, link/fragment checks, `git diff --check`, and one disposable governance-fixture exercise.

Possible regression: renaming IDs or changing dependency edges can break existing links and experiment plans. Preserve aliases and migration receipts; do not silently replace published identifiers.

Validation required: independent human review, schema/validator tests, the Section 86 fixture-repository exercises, and a dry-run conversion of representative roadmap, risk, experiment, feed, issue, and ADR records.

Decision: remediation applied; pending human approval for policy adoption and promotion beyond draft/needs-machine-validation.
