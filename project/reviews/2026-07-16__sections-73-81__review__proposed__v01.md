---
type: improvement-proposal
status: proposed
target: wiki/HaloFPX_Wiki/11_Verification_and_Performance/73-81
created: 2026-07-17
risk: high
approval_required: human
decision: revise
---

# Adversarial review: Sections 73-81

## Decision

**Overall: REVISE.** After safe remediation, Sections 73-75 and 77 are acceptable as evidence-scoped methodology/experiment artifacts. Sections 76 and 78-81 still require the open experiments or P2 policy corrections below before category-level acceptance. No section contains promoted HaloFPX performance results.

## Remediation update — 2026-07-17

All safe P1 remediation authorized after this review was applied:

| Original P1 | Applied change | Current state |
|---|---|---|
| Sections 78-81 artifact schema | Added required front matter to every Markdown page; normalized every `section.yaml`; enumerated machine experiments; removed `.gitkeep` | Closed |
| Sections 78-81 dead/incomplete source records | Replaced `kyuz0` URLs with commit-pinned `charlie12345`/`fewtarius` sources; added stable source IDs, supported claims, limitations, and fact citations | Closed; deterministic source-closure validation passed |
| Section 73 implied executable schema | Implemented Draft 2020-12 schema 1.0.0 for seven record families, deterministic semantic validator, valid/invalid fixtures, and preserved execution receipt | Closed for structural conformance; metric thresholds and machine controls remain open |
| Section 74 telemetry lifecycle | Starts telemetry after readiness and before traffic; records server/collector status; removes exit masking; retains interruption cleanup | Closed, pending live smoke execution |
| Category README scaffold | Replaced with evidence-scoped navigation, authority map, links, and cross-category dependencies without promoting measurements | Closed |

The overall decision remains **REVISE** because unexecuted machine experiments and the P2 findings below still block category acceptance. Sections 73 and 74 move to **ACCEPT** as evidence-scoped planning artifacts after deterministic validation.

Post-remediation validation passed for all Sections 73-81: exactly seven required files per section, parseable Markdown front matter and `section.yaml`, required metadata fields, `applies_to` shape, source counts, source-reference closure, local links, category links, absence of stale `.gitkeep`/`kyuz0` references, and `git diff --check`. Canonical repository heads were also revalidated with `git ls-remote`.

Current blockers are Section 76's scaling-denominator correction; reproducible soak/fault protocols for Sections 79-80; Section 81's directionally explicit performance-gate rule; and all required target-machine experiments and policy approvals. Section 73 schema validity is structural only and does not approve warmup, sample counts, confidence methods, or metric thresholds.

This review did not modify wiki pages. It compared the completed artifacts with the section prompts, `research/prompts/OUTPUT_STANDARD.md`, repository governance, Agent Harness promotion rules, the draft requirements in Section 09, and the cross-section dependencies declared by Sections 73-81.

Severity meanings:

- **P1 / high:** blocks reliable indexing, evidence promotion, or safe execution.
- **P2 / medium:** can produce a wrong comparison, incomplete experiment, or unreproducible conclusion.
- **P3 / low:** clarity or consistency defect that should be fixed with the next revision.

## Findings

### P1 — Sections 78-81 do not satisfy the required artifact schema

**Remediation status: APPLIED 2026-07-17.** Retained below as the evidence and rationale for the applied correction.

Affected paths:

- `wiki/HaloFPX_Wiki/11_Verification_and_Performance/78_Correctness_Regression_Determinism_and_Model_Quality_Evaluation/`
- `wiki/HaloFPX_Wiki/11_Verification_and_Performance/79_Stress_Soak_Long_Context_Multi_Session_Power_and_Thermal_Testing/`
- `wiki/HaloFPX_Wiki/11_Verification_and_Performance/80_Fault_Injection_Cable_Pulls_Restarts_OOM_Disk_Full_and_Corruption/`
- `wiki/HaloFPX_Wiki/11_Verification_and_Performance/81_CI_Matrix_Release_Gates_Reproducibility_and_Performance_Regression_Policy/`

Evidence:

- Five required Markdown pages in each section lack all required front matter; only `README.md` has front matter.
- Each `README.md` represents `applies_to` as a list rather than the required `repositories`, `software_versions`, and `hardware_revisions` mapping.
- Each `section.yaml` omits category, source count, open-question count, required machine experiments, related sections, and applicability. It substitutes nonstandard `claim_summary`, `deliverables`, and `validation_required` fields.
- All four completed directories retain `.gitkeep`.

Impact: deterministic cataloging cannot establish applicability, source/question counts, or required experiment gates. Agent Harness promotion cannot distinguish a complete reviewed section from a partial draft by the repository contract.

Required revision: normalize every Markdown front matter block, rebuild each `section.yaml` to the required summary contract, enumerate concrete experiment IDs, and remove the obsolete placeholder only when the folder is complete.

### P1 — Material source claims in Sections 78-81 point to nonexistent repositories

**Remediation status: APPLIED 2026-07-17.** Canonical repository heads were rechecked and source ledgers/citations were replaced.

Affected paths:

- `78_.../sources.md`
- `79_.../sources.md`
- `80_.../sources.md`
- `81_.../sources.md`

Evidence collected 2026-07-17 with `git ls-remote <url> HEAD`:

- `https://github.com/kyuz0/ROCmFPX.git` failed; `https://github.com/charlie12345/ROCmFPX.git` resolved to `a5605a72768c6562241b248e268e33dc92787394`.
- `https://github.com/kyuz0/CachyLLama.git` failed; `https://github.com/fewtarius/CachyLLama.git` resolved to `6be745998f568e379ea197fcf827baec73ff9940`.
- `https://github.com/kyuz0/llama-ai.git` failed; `https://github.com/fewtarius/llama-ai.git` resolved to `1017f3dfdce3ca2b06aa9007b23295db3bb35722`.

The same source pages use unlabeled two-column tables without stable source IDs, per-record publisher/repository and revision, claims supported, or limitations/conflicts. Several official documentation links are moving `latest` pages rather than frozen revisions. Root-repository links are used to support specific claims about scripts and skip behavior without identifying the inspected paths.

Impact: `[VERIFIED]` claims cannot be followed to the asserted source, and future reviewers cannot distinguish exact source evidence from an inferred repository inventory.

Required revision: replace dead URLs with the canonical commit-pinned repositories, add stable IDs, cite exact files/lines or commit paths for repository-behavior claims, record access/revision and limitations for every source, and re-check every `[VERIFIED]` claim against the corrected ledger.

### P1 — Section 73 names schema v0.1 but does not deliver the required raw-data schema

**Remediation status: CLOSED 2026-07-17.** Schema 1.0.0, a deterministic validator, one valid fixture, one invalid fixture, and a command/result receipt were added. The valid fixture passed with zero errors; the invalid fixture was rejected with nine structural/semantic errors. This closes structural conformance only.

Affected paths:

- `73_Benchmark_Methodology_Terminology_Experimental_Controls_and_Data_Schema/facts_and_constraints.md`
- `73_Benchmark_Methodology_Terminology_Experimental_Controls_and_Data_Schema/procedures_and_checks.md`
- `73_Benchmark_Methodology_Terminology_Experimental_Controls_and_Data_Schema/section.yaml`

The section defines record families and shows one illustrative `request` JSON object. It does not provide a JSON Schema for `run_manifest`, `request`, `token_event`, `collective_event`, `cache_event`, `telemetry_sample`, and `run_summary`; required/optional fields; types, enums, units, null/missing rules; referential integrity; monotonic-clock domains; schema compatibility; or validation fixtures. Downstream sections nevertheless depend on “the Section 73 schema.”

Impact: two agents can produce mutually incompatible raw evidence while both claiming conformance. Derived statistics cannot be deterministically reproduced or rejected at ingestion.

Required revision: add a versioned Draft 2020-12 schema plus valid/invalid fixtures and a deterministic validator receipt. Specify cross-record identity and timing invariants and a compatibility/migration policy. Keep it proposed until round-trip validation passes.

### P1 — Section 74's documented execution order prevents telemetry capture

**Remediation status: APPLIED 2026-07-17.** The collector now runs before measured traffic and both exit statuses are retained. Live smoke remains required.

Affected path: `74_Single_Node_HIP_and_Vulkan_Baseline_Matrix/procedures_and_checks.md`, especially the end of step 5 and step 6.

Step 5 kills and waits for `SERVER_PID`; step 6 then loops only while that PID is alive. Sequential execution therefore yields no workload telemetry. `wait "$SERVER_PID" || true` also discards an abnormal server exit, contradicting the requirement to retain failures. The procedure has no trap to preserve the server status and stop the collector on interruption.

Impact: an apparently complete baseline can lack synchronized resource/thermal evidence or silently ignore a crash.

Required revision: start the telemetry collector before measured traffic, retain both PIDs, install bounded cleanup traps, capture both exit statuses, stop the collector only after the workload/server result is recorded, and dry-run the lifecycle in a disposable smoke test.

### P1 — The category deliverable remains a stale scaffold

**Remediation status: APPLIED 2026-07-17.** The category page now provides evidence-scoped status, authority routing, section links, and dependencies.

Affected path: `wiki/HaloFPX_Wiki/11_Verification_and_Performance/README.md`.

The category prompt requires links to every section, authoritative-page guidance, a category summary, and cross-category dependencies. The current page still says `Research status: pending`, lists plain text rather than links, and does not identify Section 73 as methodology authority, Section 78 as correctness authority, or Section 81 as the proposed promotion gate.

Impact: retrieval starts from stale state and bypasses the intended evidence hierarchy.

Required revision: update the category page only after the section revisions are merged and validated; do not mark the category verified while required machine experiments remain absent.

### P2 — Sections 78-81 use non-contract claim labels and misuse `[MEASURED]`

**Remediation status: PARTIALLY APPLIED 2026-07-17.** Non-contract `[CONSTRAINT]` labels and false `[MEASURED] No ...` claims were removed, and source-backed facts now cite stable IDs. A complete material-conclusion labeling pass remains recommended.

Affected paths: all Markdown pages in Sections 78-81, especially each `facts_and_constraints.md`.

Material conclusions use bare `[VERIFIED]`, `[RECOMMENDATION]`, and `[OPEN]` inconsistently; many normative statements are unlabeled; and `[CONSTRAINT]` is not one of the repository's defined claim classes. Each facts page states `[MEASURED] No ... result was measured`, which is not a measured result and conflicts with `claim_summary.measured: 0`.

Impact: automated and human readers cannot apply the literal evidence semantics required by the project. “No measurement exists” may be mistaken for promoted experiment evidence.

Required revision: use only the six defined claim classes for material conclusions, classify project constraints as sourced `[VERIFIED]`, reasoned `[INFERENCE]`, or proposed `[RECOMMENDATION]`, and state absence of measurement as scope/`[OPEN]`, never `[MEASURED]`.

### P2 — Section 81's regression rule is not directionally decidable

Affected path: `81_CI_Matrix_Release_Gates_Reproducibility_and_Performance_Regression_Policy/design_implications.md`.

“Fail only when the interval crosses a preapproved practical-effect threshold” conflicts with “inconclusive when uncertainty straddles it.” It also omits metric orientation: higher is better for throughput, while lower is better for latency, power, and memory.

Impact: the same confidence interval can be classified as fail or inconclusive by different implementations.

Required revision: define a normalized effect orientation or explicit rule per metric. For example, fail only when the entire confidence interval lies beyond the regression boundary, pass only when the entire interval lies within the permitted region, otherwise mark inconclusive. Validate the gate with synthetic boundary cases before CI use.

### P2 — Section 76's scaling-efficiency denominator can misstate two-node value

Affected path: `76_Distributed_Mode_Benchmark_Matrix_and_Break_Even_Analysis/facts_and_constraints.md`.

The proposed `throughput_mode / (2 * throughput_one_matched_node)` assumes equal nodes and ignores the measured two-replica control. Section 74 explicitly requires proving node interchangeability rather than pooling first.

Impact: node asymmetry, routing overhead, or replica imbalance can be attributed incorrectly to the distributed mode.

Required revision: use the observed sum of matched independent-node capacities for an idealized denominator and the measured two-replica control for an operational comparison. Report both with node-local values and uncertainty.

### P2 — Sections 79 and 80 are plans, but not yet reproducible experiment protocols

Affected paths:

- `79_.../procedures_and_checks.md`
- `80_.../procedures_and_checks.md`
- corresponding `section.yaml` files

Section 79 gives useful initial 6-hour/24-hour phases but does not define repetition, aborted/censored-run handling, a numerical equilibrium rule, evidence-gap policy by signal, or how duration is shown to cover the relevant failure timescale. Section 80 gives a sound staged fault matrix but lacks stable case IDs, exact privilege/root declarations per injection, pre-arm/dry-run output, rollback/cleanup verification per mechanism, and a machine-readable request-outcome oracle. GPU reset and rail retrain mechanisms remain unselected.

Impact: operators can execute materially different tests under the same name; destructive cases cannot yet be safely automated or compared.

Required revision: assign experiment/case IDs, prerequisites, exact scoped targets, expected invariants, stop conditions, cleanup receipts, and retained evidence. Calibrate soak duration from pilot failure/resource-drift observations. Do not execute physical or kernel/block faults before a disposable-environment safety review.

### P3 — Terminology and authority links drift across the category

Affected paths:

- Section 74 uses `ILT`; Sections 09 and 73 define `ITL`.
- Sections 78-81 do not explicitly identify which focused page is authoritative for methodology, correctness, stress, fault, and release policy.
- Section 79 refers to “Section 73 baselines,” although Section 73 defines methodology; measured baselines belong to Sections 74, 76, and 77.

Impact: retrieval and generated reports can fork terminology or cite a methodology page as measurement evidence.

Required revision: normalize to `ITL`, add explicit authority links, and distinguish methodology from measured baseline artifacts.

## Missing tests and research before acceptance

1. Run a deterministic section-contract validator across 73-81: required files, front matter shape, `section.yaml`, source/question counts, claim labels, and relative links.
2. Extend Section 73 fixture coverage beyond the required valid/invalid request pair when each remaining record producer is implemented; keep structural validation separate from statistical approval.
3. Execute a shell/process-lifecycle smoke for Section 74 proving synchronized workload and telemetry capture, exit-status preservation, and cleanup after interruption.
4. Audit every repository `[VERIFIED]` claim in 78-81 against exact canonical commit paths; retain a link/commit validation receipt.
5. Pilot the Section 73 statistical rules on synthetic and target-machine variance: p95/p99 sample adequacy, block/run resampling, false-positive and false-negative rates, and missing/failure handling.
6. Compare Section 76's idealized sum-of-nodes denominator, measured two-replica control, and each distributed candidate on deliberately asymmetric node loads.
7. Calibrate Section 79 equilibrium and duration rules from repeated short/medium pilots before using 6-hour/24-hour runs as release evidence.
8. Build a disposable Section 80 harness with case IDs, `--dry-run`, exact target resolution, privilege declaration, stop conditions, cleanup receipts, and post-fault Section 78 correctness replay.
9. Unit-test Section 81 gate decisions at pass/fail/inconclusive boundaries for higher-is-better and lower-is-better metrics, required skips, stale baselines, and runner failure.
10. Validate the revised category README links and authority map after section corrections.

## Section disposition

| Section | Status | Reason |
|---|---|---|
| 73 | **ACCEPT** | Schema 1.0.0, deterministic validation, valid/invalid fixtures, and execution evidence are delivered; machine controls and thresholds correctly remain open. |
| 74 | **ACCEPT** | Strong staged matrix; the documented process lifecycle was corrected to collect telemetry and preserve exit status. Live machine validation remains required. |
| 75 | **ACCEPT** | Complete evidence-scoped fabric plan with correct GPU-consume boundary, clock limits, contention, and fault handoff. Machine validation remains required. |
| 76 | **REVISE** | Good factorized mode plan; correct the scaling denominator before break-even analysis. |
| 77 | **ACCEPT** | Complete, preservation-first cache benchmark plan with explicit donor limitations and fail-closed corruption behavior. Machine validation remains required. |
| 78 | **REVISE** | Metadata and provenance are repaired; comparator calibration, datasets/models, material-claim review, and target execution remain open. |
| 79 | **REVISE** | Metadata and provenance are repaired; reproducible soak duration/equilibrium and censored-run policy remain incomplete. |
| 80 | **REVISE** | Metadata and provenance are repaired; executable fault-case controls, exact mechanisms, and disposable-environment proof remain incomplete. |
| 81 | **REVISE** | Metadata and provenance are repaired; regression decision semantics, runner policy, and release-gate execution remain open. |

## Acceptance gate for the revision

Accept the category only when all P1 findings are closed, every section passes deterministic contract validation, all `[VERIFIED]` claims resolve to primary evidence, the Section 73 schema validates retained fixtures, and Sections 74/81 pass their lifecycle and decision-rule tests. P2 findings may remain only as explicitly owned, dated, non-release-blocking open questions with no affected gate enabled.

Decision: **revise**.
