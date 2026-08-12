---
title: "Adversarial review of HaloFPX Wiki sections 24-48"
date: "2026-07-17"
status: "proposed"
scope: "wiki/HaloFPX_Wiki sections 24-48"
review_type: "evidence, consistency, version-baseline, and feasibility audit"
---

# Adversarial review: sections 24-48

## Verdict

All 25 audited section directories contain the seven required artifacts, every Markdown page has the required front matter, all `section.yaml` files parse, and no broken relative Markdown links were found. The strongest pages preserve the boundary between source-code facts, design recommendations, and target-machine work. No target-machine performance result in sections 24-48 is currently supported for promotion as `[MEASURED]`.

The set is not acceptable as one executable engineering baseline. The original section 26 citation to an unsupported ROCm version has been removed, but no complete executable toolchain has been selected. The sections still mix comparison snapshots that require a separate root baseline decision, and several required deliverables remain candidate sketches rather than completed research outputs. The distributed material is useful architecture research, but it is not yet feasible implementation authority because the transport, measurement, sampler-state, and recovery contracts remain unresolved.

| Category | Status | Basis |
|---|---|---|
| 05 Performance Software and Tools (24-28) | **REVISE** | The invalid section 26 ROCm baseline is corrected and missing toolchain fields are now explicit; exact executable selection and sections 27-28's mutable/unselected versions remain open. |
| 06 Models, Quantization, and Inference (29-37) | **REVISE** | Exact project repository commits are mostly consistent and section 37 is now an unranked hypothesis backlog, but section 29 does not yet satisfy the required catalog schema and section 31's thresholds remain unmeasured proposals. |
| 07 Distributed Runtime (38-48) | **REVISE** | The candidate mode contracts are thoughtful, but source baselines, sampler ownership, transport feasibility, fallback state, and break-even evidence are unresolved. These pages may guide experiments, not implementation approval. |

## Severity-ranked findings

### P0 — RESOLVED 2026-07-17: section 26's unsupported ROCm 7.14.0 baseline

**Original evidence.** Section 26 identified `ROCm Core SDK 7.14.0` as its candidate and claimed that 7.14.0 listed a `gfx1151` package (`26_Compiler_CMake_Linker_and_Reproducible_Toolchain/section.yaml`, `sources.md`, and `facts_and_constraints.md`). The cited links were mutable `/latest/` pages. AMD's official [ROCm release history](https://rocm.docs.amd.com/en/develop/release/versions.html), checked 2026-07-17, listed ROCm 7.2.3 as the current release and did not list 7.14.0. AMD's immutable [ROCm 7.2.3 release notes](https://rocm.docs.amd.com/en/docs-7.2.3/about/release-notes.html) identify RCCL 2.27.7 in that release. Section 24 independently pins ROCm 7.2.3 and its component commits.

**Impact.** Compiler, package, ABI, feature, and reproducibility conclusions in section 26 cannot be used. Treating 7.14.0 as a real package baseline could produce non-reproducible installation instructions or mask an accidental documentation-version parse.

**Remediation.** All 7.14.0-dependent claims and applicability entries were removed. Section 26 now cites AMD's immutable ROCm 7.2.3 release notes and versioned Strix Halo guidance, labels 7.2.3 a research comparison rather than a deployment selection, and explicitly leaves the exact host/package/compiler tuple `[OPEN]`. CMake 4.4.0 is also labeled a documentation comparison rather than the selected executable.

**Affected paths.** `wiki/HaloFPX_Wiki/05_Performance_Software_and_Tools/26_Compiler_CMake_Linker_and_Reproducible_Toolchain/`.

### P1 — There is no reconciled executable version baseline across sections 24-48

**Evidence.** The audited pages use at least these distinct lines:

| Component | Snapshots used | Conflict |
|---|---|---|
| ROCm/HIP | ROCm 7.2.3 and HIP 7.2.53211 in section 24; HIP 7.2.53211 plus a page displayed as development 7.13.0 in section 25; ROCm 7.2.3 immutable comparison evidence in section 26; HIP 7.2.53210 in section 45 | No selected package/build/runtime fingerprint binds these claims together; develop/current snapshots remain comparisons. |
| RCCL | ROCm 7.2.3 / RCCL 2.27.7 commit `96a25b5...` in section 24; development docs 2.30.4 and commit `57e5868...` in sections 42 and 45; section 48 combines commit `57e5868...` with 2.27.7 documentation | API availability and runtime behavior are being compared across different source and documentation revisions. |
| Vulkan/Mesa | Vulkan 1.4.357 and Mesa main `20f4f9f...` in section 25 | This is a source comparison, not the distribution-patched Mesa/kernel actually running on either rank. |
| Framework analogues | PyTorch 2.13 in section 43 and PyTorch 2.9 in section 48 | Reference designs are not clearly separated from candidate dependencies in a shared ledger. |

ROCmFPX `a5605a72768c6562241b248e268e33dc92787394` and llama.cpp `788e07dc91d266ad3162a1ce9037665656269689` are mostly consistent in sections 29-47, which is a strength. That consistency does not resolve their compiler, ROCm, Mesa, kernel, RCCL, firmware, or package ABI.

**Impact.** A reader can accidentally combine an API from RCCL 2.30.4 with the ROCm 7.2.3 runtime, or compare a Mesa-main Vulkan path against a different installed HIP stack and call it matched. Distributed rank compatibility and experiment reproducibility remain undefined.

**Required revision.** Create one version-baseline decision/ledger that distinguishes: selected executable baseline, inspected upstream comparison, reference-only framework, installed machine state, and rejected candidate. Bind every experiment and plan to the complete fingerprint and fail closed on rank mismatch.

**Affected paths.** Sections 24-28, 37, 42-45, and 48; especially their `sources.md`, front matter, and `section.yaml` applicability records.

### P1 — PARTIALLY REMEDIATED: section 26 does not yet select the toolchain required by its prompt

**Evidence.** The prompt requires supported Clang/LLVM, GCC, HIP compiler, shader compiler, CMake, Ninja, Python, linker, and packaging versions, plus architecture flags and reproducible build controls. The section specifies only candidate ROCm and CMake versions. Clang, GCC, Ninja, Python, linker, shader compiler, package manager, package hashes, and accepted architecture flags remain recommendations or pending inventory. The source repository commits are recorded but not a buildable dependency lock.

**Impact.** The section cannot reproduce a binary, prove gfx1151 code-object emission, compare two ranks, or establish that a benchmarked artifact corresponds to a source/toolchain fingerprint.

**Remediation and remaining work.** Section 26 now contains a required-component matrix covering Clang/LLVM/HIP, GCC, Mesa/ACO, CMake, Ninja, Python, linker, and packages, with every unselected field explicitly `[OPEN]`. Selection still requires exact executable/package versions and hashes, supported/unsupported lanes, emitted target flags, ABI compatibility, sanitizer/LTO/PGO constraints, preset names, generated artifacts, and two-host fingerprint checks. Preserve a real clean-build/diffoscope experiment before promoting reproducibility claims.

**Affected paths.** Section 26, especially `facts_and_constraints.md`, `design_implications.md`, `procedures_and_checks.md`, and `section.yaml`.

### P1 — The section 29 catalog does not meet its required coverage or establish fit

**Evidence.** The prompt requires architecture, total and active parameters, layers, hidden size, heads, context, weight size by quant, KV bytes/token, special state, license, conversion status, backend support, and validation status across dense, MoE, GQA, MLA, sliding-window, Mamba/SSM/hybrid, MTP, coding, tool-use, and long-context targets. Section 29 contains five useful candidate rows, but has no pinned MHA or sliding-window candidate and does not give a complete field set per row. Quant sizes are nominal `parameters * bits / 8`, not actual artifact sizes; converter/backend status is summarized as open rather than an explicit matrix; license text for DeepSeek-V3 is unresolved.

**Impact.** The catalog cannot yet drive memory placement, download/license approval, conversion scheduling, backend gating, or distributed-mode feasibility. The DeepSeek-V3 fit inference is especially sensitive to the still-unrecorded exact machine memory/BOM and actual GGUF layout.

**Required revision.** Define one machine-readable row schema, fill every required field or mark it `[OPEN]` with an owner, add pinned targets for missing architecture classes, and distinguish publisher labels, derived estimates, actual file hashes/sizes, backend recognition, successful conversion, and measured execution.

**Remediation status.** The README and new OQ29-09 now state the incomplete schema, missing MHA/sliding-window targets, nominal-size limitation, unresolved license/state fields, and missing converter/backend/machine evidence as explicit `[OPEN]` work. No placeholder model facts were invented. The finding remains open until those rows are researched.

**Affected paths.** `wiki/HaloFPX_Wiki/06_Models_Quantization_and_Inference/29_Target_Model_Catalog_and_Architecture_Support_Matrix/`.

### P1 — Distributed sampler ownership is not a closed cross-section contract

**Evidence.** Section 39 recommends that tokenization and stochastic sampling belong on the coordinator. Section 43 assigns the physical sampling fast path to rank 1 beside the output head while retaining logical coordinator authority and returning an updated sampler/RNG record. Section 48 says the coordinator owns sampling/RNG state and the canonical command log. Section 43 correctly records the unresolved protocol as `DR43-O7`, but sections 39 and 48 can still be read as a different execution rule.

**Impact.** RNG consumption, grammar state, cancellation, replay, exactly-once output, and recovery cannot be implemented safely from the current pages. A coordinator/rank disagreement can silently duplicate or skip stochastic state even if logits are correct.

**Required revision.** Publish one mode-independent ownership contract distinguishing logical authority, physical execution, state-transition validation, commit point, replay token, and failure behavior. Prove exact state round trips for deterministic and stochastic samplers before allowing rank-local sampling.

**Remediation status.** Sections 39, 43, and 48 now explicitly say that coordinator authority does not select physical sampler placement, that rank-1 sampling is only a candidate, and that RNG/grammar state transfer, output commit, transport, checkpoint, replay, and fallback remain `[OPEN]`. This removes an unsafe implied decision without pretending to resolve the protocol.

**Affected paths.** Sections 39, 43, 45, and 48; related state dependencies in sections 32, 36, and 41.

### P1 — The distributed designs remain feasibility proposals because their required transport and measurement authorities are unresolved

**Evidence.** Sections 38-48 repeatedly defer actual bandwidth/latency distributions, dual-link behavior, GPU-visible buffer movement, command transport, integrity, cache restore, benchmark policy, and numerical tolerance to sections 49-58 and 73-78. No HaloFPX two-rank tensor, pipeline, expert, remote-draft, persistent-graph, or recovery measurement is linked. Current llama.cpp RPC and ROCmFPX split modes are correctly described as evidence, not proof of the proposed rank protocols.

**Impact.** The pages define credible experiments and candidate contracts, but not implementable topology selections or performance break-even points. A design that is correct in abstract may be dominated by host staging, queueing, dual-link imbalance, or failure-recovery cost.

**Required revision.** Gate implementation decisions on a measured transport envelope by message size/concurrency/direction, actual tensor/state shapes, rank memory headroom, correctness oracle, and fault matrix. Mark every topology plan explicitly `candidate` until those dependencies close.

**Remediation status.** Sections 39, 43, and 48 now surface transport integrity/completion, boundary movement, cross-rank checkpointing, state ownership, and fallback as explicit blockers. Feasibility remains open pending the referenced research and experiments.

**Affected paths.** Sections 38-48, with dependencies on sections 49-58, 73, 74, 75, 76, and 78.

### P2 — PARTIALLY RESOLVED: performance language and unmeasured priority

**Original evidence.** The literal `[MEASURED]` occurrences in sections 24, 26, 37, 38, and 42-48 were warnings or future promotion rules, not positive HaloFPX results. Section 31 correctly labels its 0.1/1/3/5 percent perplexity and tool-call thresholds `[RECOMMENDATION]`, and section 38 leaves objectives open. Section 37 formerly gave P0-P3 optimization priorities and stated that decode was dominated by particular operation classes before a target-machine profile existed. Sections 25, 43, 44, and 47 correctly require matched evidence before declaring a backend or topology winner.

**Impact.** Readers or agents may interpret a priority label or numeric gate as empirical authority. Repository measurements in ROCmFPX and literature results remain environment-specific and cannot rank HaloFPX work.

**Remediation.** Section 37's P0-P3 table is now an unranked H37-A through H37-J hypothesis backlog. The prior claim about decode dominance is an `[ASSUMPTION]`, and the page explicitly states that no item receives priority until M37-01 attributes target-machine and end-to-end time. Section 31's thresholds correctly remain provisional; the root-level generated promotion rule remains outside this remediation scope.

**Affected paths.** Sections 31, 37, 38, 42-47; especially `37_.../facts_and_constraints.md` and `design_implications.md`.

### P2 — Mutable documentation weakens otherwise commit-pinned source records

**Evidence.** Moving `latest` or `develop` documentation appears in sections 24, 25, 26, 27, 37, 42, 45, and 48. Some rows pair the moving page with exact source commits and explicitly state the limitation; others record only a displayed documentation version and access date. Section 27's profiler interface and sections 42/45's RCCL API are particularly version-sensitive.

**Impact.** A later reader cannot prove which wording or API surface supported the `[VERIFIED]` statement. An access date is useful provenance but is not an immutable revision.

**Required revision.** Prefer versioned documentation URLs, exact documentation commits, release PDFs, or locally hashed snapshots. Where only moving documentation exists, treat it as discovery evidence and pin the implementation source used for any behavioral conclusion.

**Affected paths.** `sources.md` in sections 24-27, 37, 42, 45, and 48.

### P2 — Category manifests and identifier conventions are stale or inconsistent

**Evidence.** Category READMEs 05-07 still say `Research status: pending` even though all 25 section directories are populated and individually marked `needs-machine-validation`. Source IDs use several grammars (`S24-001`, `S30-01`, `S48-LLAMA-SERVER`), while open-question IDs in sections 46-48 start with `S46-OQ`, `S47-OQ`, and `S48-OQ`, making them look like source identifiers. Open-question tables in sections 30-33 and several distributed pages rely on file context rather than literal `[OPEN]` labels.

**Impact.** Automated source/question aggregation is ambiguous, category freshness is misleading, and the root's promised stable registries cannot be generated reliably.

**Required revision.** Generate category status from validated section metadata; select disjoint grammars for source, question, experiment, and decision IDs; require explicit question status in data or labels; and publish redirects for any renamed IDs.

**Affected paths.** Category READMEs 05-07; `open_questions.md` in sections 30-33 and 38-48; source ledgers across 24-48.

## Version-baseline reconciliation

The review does not select a deployment baseline; that requires machine inventory and a project decision. It does establish the safe interpretation of current evidence:

1. `ROCmFPX@a5605a72768c6562241b248e268e33dc92787394`, `llama.cpp@788e07dc91d266ad3162a1ce9037665656269689`, and `CachyLLama@6be745998f568e379ea197fcf827baec73ff9940` are inspected source snapshots, not proof of a mutually compatible build.
2. ROCm 7.2.3 plus its component pins is the only coherent ROCm release snapshot documented across these sections. It remains a research baseline until both machines record installed packages, kernel, firmware, Mesa, compiler, and runtime behavior.
3. RCCL 2.30.4/development material is comparison evidence only. It must not supply API or behavior to a ROCm 7.2.3 plan unless the exact newer RCCL source is deliberately built, dependency-checked, and machine-validated.
4. Mesa main, Vulkan 1.4.357, vLLM, Megatron-LM, DeepEP, and PyTorch pages are reference implementations or source comparisons unless a decision explicitly adopts them.
5. The former `ROCm Core SDK 7.14.0` entry was removed as unsupported; no future version may enter applicability from a moving `latest` page without immutable release/package evidence.

## Per-section disposition

| Section | Status | Required action before promotion |
|---|---|---|
| 24 | **ACCEPT WITH CONDITIONS** | Retain 7.2.3 as research snapshot; pin mutable profiler/HSA docs and run all machine coherence/collective tests. |
| 25 | **ACCEPT WITH CONDITIONS** | Bind Mesa/Vulkan claims to installed lineage; keep Vulkan as measured candidate only. |
| 26 | **REVISE** | P0 corrected; select and validate the exact executable/package toolchain now enumerated as OPEN. |
| 27 | **REVISE** | Select exact profiler/kernel/tool versions and archive versioned interfaces before using counter claims. |
| 28 | **REVISE** | Bind tuning procedures to an exact kernel/systemd/filesystem stack and retain reversible A/B evidence. |
| 29 | **REVISE** | Complete the required catalog schema and missing architecture targets. |
| 30 | **ACCEPT WITH CONDITIONS** | Keep fork formats commit-scoped; require collision, backend, quality, and artifact tests. |
| 31 | **ACCEPT WITH CONDITIONS** | Keep numerical gates as recommendations until variance and evaluation ownership are approved. |
| 32 | **ACCEPT WITH CONDITIONS** | Revalidate lifecycle hooks after baseline selection and prove public/internal adapter boundaries. |
| 33 | **ACCEPT WITH CONDITIONS** | Measure actual layouts, fallback, quality, shifting, and persistence compatibility. |
| 34 | **ACCEPT WITH CONDITIONS** | Treat telemetry schema and placement inputs as proposed until overhead and representative routing traces exist. |
| 35 | **ACCEPT WITH CONDITIONS** | Prove recurrent-state shape, shift, serialization, and rollback per target model. |
| 36 | **ACCEPT WITH CONDITIONS** | Validate MTP/speculator state, acceptance semantics, and stochastic replay on target builds. |
| 37 | **ACCEPT WITH CONDITIONS** | Hypotheses are now unranked; rank work only after section 27/M37-01 profiling and end-to-end guards. |
| 38 | **ACCEPT AS FRAMEWORK** | Populate objectives and cost terms with p99 target evidence before choosing a mode. |
| 39 | **REVISE** | Close coordinator/rank sampler authority and leadership/fencing contracts. |
| 40 | **ACCEPT WITH CONDITIONS** | Prove affinity benefit, durable lease/commit behavior, and cache/state failover. |
| 41 | **ACCEPT WITH CONDITIONS** | Measure acceptance, round-trip latency, target ownership, and disposable draft recovery. |
| 42 | **REVISE** | Reconcile RCCL lineage and prove the intended collective path over the actual fabric. |
| 43 | **REVISE** | Close sampler-state transfer and prove pipeline capacity/performance/failure behavior. |
| 44 | **ACCEPT AS CANDIDATE DESIGN** | No current expert-parallel implementation exists; require exact model maps, traces, memory, transport, and fault evidence. |
| 45 | **REVISE** | Reconcile HIP/RCCL baseline and turn the proposed ring into a versioned, tested wire/state specification. |
| 46 | **ACCEPT WITH CONDITIONS** | Validate admission, fairness, cancellation, and bounded resource behavior under mixed load. |
| 47 | **ACCEPT AS FRAMEWORK** | Planner outputs remain invalid until the measured feature set, confidence policy, held-out tests, and rollback exist. |
| 48 | **REVISE** | Reconcile RCCL source/docs, define the oracle/tolerances, and prove checkpoint/output-ledger recovery. |

## New research prompts and acceptance questions

1. **Unified executable baseline:** What exact kernel, firmware, ROCm packages and component commits, HIP compiler, Mesa/RADV, Vulkan headers, RCCL, CMake, Ninja, Python, linker, and project commits are installed or selected on both ranks, and which combinations are rejected?
2. **Section 26 correction:** Which immutable AMD release/package sources define a real gfx1151 toolchain, and can two clean builds from distinct paths reproduce every host binary, code object, shader, generated file, and package manifest?
3. **RCCL lineage and transport:** Which RCCL commit is ABI-compatible with the selected ROCm stack, which collective/network plugin path actually operates over the dual USB4 topology, and what are its p50/p99 curves and failure semantics by message shape?
4. **Model catalog completion:** Which pinned target represents every required architecture class, what are its actual artifact hashes/sizes and state bytes, and which conversion/backend/quality stage has it demonstrably passed?
5. **Sampler authority protocol:** For coordinator-local and rank-local sampling, who owns RNG/grammar state before and after a token, what is committed atomically, and how do cancellation, replay, rank loss, and stale epochs fail closed?
6. **Performance-claim promotion:** Can a validator require every positive latency/throughput/energy/quality claim to reference a checked-in experiment manifest, raw data, exact environment, uncertainty, and matched baseline?
7. **Transport-feasibility gate:** For every candidate distributed mode, what are the real tensor/state message sizes, copy stages, GPU/CPU synchronization points, queueing distribution, memory headroom, and break-even rule under the intended workload?
8. **Source-freeze audit:** Can every mutable `latest`/`develop` source be replaced by an immutable document revision, source commit, release PDF, or content-hashed local snapshot without losing licensing/provenance?
9. **Cross-section contract test:** Can one generated schema prove that ownership, epochs, command IDs, plan hashes, cache/state identity, output commit, single-node fallback, and failure behavior agree across sections 39-48?
10. **Manifest freshness:** Can category READMEs and global ledgers be generated from validated section metadata so status, IDs, links, and unresolved dependencies cannot silently drift?

## Acceptance gate

The P0 finding is resolved. Do not accept sections 24-48 as an integrated implementation baseline until a single version ledger is approved, section 26 has a selected and validated toolchain, section 29 satisfies its required coverage, sampler ownership is a tested cross-mode contract, and the relevant transport/correctness experiments exist. Continue to use the remaining pages as scoped research and experiment design. Re-run structure, source-pin, identifier, claim-label, local-link, and version-consistency validators and retain the result as review evidence before promotion.
