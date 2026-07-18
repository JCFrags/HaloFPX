---
section_id: "82"
title: "Roadmap Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "CachyLLama@6be745998f568e379ea197fcf827baec73ff9940", "llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"]
  software_versions: ["roadmap baseline 2026-07-17"]
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["10", "15", "16", "38", "39", "40", "41", "42", "43", "44", "45", "48", "49", "50", "51", "52", "53", "55", "56", "57", "58", "59", "60", "63", "67", "70", "71", "72", "73", "74", "76", "77", "78", "79", "80", "81"]
---

# Roadmap design implications

## Epic register and exit criteria

| Epic | Work packages | Dependencies | Measurable exit |
|---|---|---|---|
| E82-00 Evidence authority | baseline manifest; source bundles; issue/ADR/experiment IDs; evidence schema | none | all four commits resolvable offline; bundles verify; dirty/local deltas recorded; source conflict retained |
| E82-01 Integration skeleton | llama.cpp-ancestry repo; six patch lanes; locked build presets; CI smoke; provenance/SBOM | E82-00 | clean and offline builds from manifest on both nodes; artifact hashes/logs retained; rollback tag checks out/builds |
| E82-10 Platform identity | BOM/firmware/cables/ports/software profile | E82-00 | HG82-A/B complete; every mismatch classified as blocking, admitted, or fixed |
| E82-11 Platform envelope | memory, storage, thermal/power, backend availability | E82-10 | HG82-C/E/F baselines reproducible; no unexplained node asymmetry in admitted cells |
| E82-20 Model/backend oracle | admit one hashed model/quant/context; CPU reference; HIP/Vulkan parity; quality corpus | E82-01,E82-11 | exact model manifest; backend-op and functional gates pass; fallback inventory explicit; candidate quality tolerances ratified |
| E82-21 Single-node product | tested API subset; lifecycle/admission; health/metrics/logs; local auth; restart | E82-20 | clean boot to truthful readiness; request/cancel/restart/error conformance; no secrets in evidence; MUP-1 runbook |
| E82-22 Quantized-KV backend candidate | L14Q P3 provenance; separate HIP decode and Vulkan coopmat1 prompt-processing manual ports; Q8_0/Q4_0 correctness, memory and matched performance | E82-20 and preserved G3 feature-off baseline | each backend lane independently admitted or deferred; no accepted baseline slowdown; prior path remains available |
| E82-30 Replicated service | two independent model servers; router; session affinity; failover policy | E82-21 | matched-node results; new-request failover; in-flight behavior explicit; cache/session misrouting rejected; MUP-2 |
| E82-40 Fabric baseline | review Sections 50-53 candidate contracts; path mapping; real-message traces; single/dual-link and fault curves | E82-10,E82-20 | HG82-D and Sections 50-55/75 evidence; carrier/multipath ADR or explicit defer |
| E82-41 Fabric API prototype | framing, credits, integrity, deadlines, async completions, epochs, observability | E82-40 | conformance and fault matrix passes over baseline carrier; one-link mode or fail-closed behavior documented |
| E82-50 Rank/runtime core | coordinator/worker lifecycle, plan handshake, command ordering, persistent graph prototype | E82-41,E82-20 | two-rank oracle, epoch/mismatch/cancel/timeout tests, allocation/graph-reuse evidence |
| E82-51 Replication comparator | representative offered-load baseline | E82-30,E82-40 | stable comparator dataset under Section 73 controls |
| E82-52 Coupled prototypes | bounded cache-disabled spikes: remote draft, TP, pipeline, MoE hybrid where feasible | E82-41,E82-50 | each spike has correctness, capacity/cost result, failure behavior, and delete/defer/admit decision |
| E82-53 Cache-off mode selection | matched Section 76 cache-disabled matrix and break-even analysis | E82-51,E82-52 | one admitted plan beats replication for ratified objective/capacity, or no-go ADR retains replication |
| E82-54 Planner | static explainable per-model plan table; guarded canary/rollback | E82-53 | held-out plan validation; no unmeasured topology auto-selection |
| E82-60 HaloKV foundation | approve and implement Sections 57-60 candidate contracts; state inventory; identity; immutable objects; atomic commit; isolation | E82-20 | incompatible/corrupt/truncated/mixed-rank state always misses/recomputes; cross-tenant access denied |
| E82-61 Rank-local restore | rank ownership; coordinated readiness; async I/O/tiering; GC/tools/endurance | E82-50,E82-60 | restore/recompute equivalence; crash/disk-full/rank-loss matrix; measured value/endurance; MUP-4 |
| E82-70 Deployment/security | resolve Sections 70-72 contingent choices; packaging; service identity; permissions/secrets; backup/migration | E82-21 plus admitted features | clean-host/cold-boot install, least-privilege audit, upgrade and rollback rehearsal |
| E82-71 Integrated verification | baseline/distributed/stress/fault matrices; cache matrix only when persistent cache is admitted; 24-hour initial soak | E82-53,E82-70; E82-61 only for a cache-integrated candidate | all mandatory Section 09 gates linked; zero safety blockers; performance misses dispositioned |
| E82-72 Release | signed immutable candidate; notes; manifests; SBOM/provenance; known limitations; support runbook | E82-71 | independent evidence review accepts candidate; rollback remains usable; release tag is immutable |

## Dependency and decision policy

**[RECOMMENDATION]** Dependencies are evidence contracts, not just “code complete.” For example, E82-52 consumes a versioned fabric service envelope and a single-node oracle, not merely a transport library and a binary.

**[RECOMMENDATION]** Use explicit decision states: `explore`, `prototype`, `admit`, `defer`, `reject`, `retire`. A prototype cannot enter a default plan until its correctness, value, failure, operations, and rollback evidence is accepted.

**[RECOMMENDATION]** Time-box speculative branches by evidence question, not calendar promise: one branch, one hypothesis, one comparator, one terminal decision. Preserve raw results and useful commits before deleting a rejected spike.

**[RECOMMENDATION]** Qualify replication and coupled execution first with persistent cache disabled. This cache-off path consumes the single-node oracle, fabric, buffer, and distributed-correctness evidence but does not consume HaloKV. If persistent cache is later admitted, rerun the affected distributed, service, fault, soak, upgrade, and rollback cells with HaloKV enabled; incompatible or corrupt cache acceptance remains a hard failure. A release that does not admit persistent cache does not depend on E82-60/E82-61.

## Coupled-mode prototype order

1. **[RECOMMENDATION]** Replication comparator first.
2. **[RECOMMENDATION]** Remote draft spike if a compatible draft/target pair and acceptance-rate measurement exist; target rank retains canonical sequence state.
3. **[RECOMMENDATION]** TP spike for a model/capacity cell that exposes a justified collective boundary.
4. **[RECOMMENDATION]** Pipeline spike if contiguous placement fits and activation transport is favorable.
5. **[RECOMMENDATION]** MoE hybrid only after routing traces and expert-kernel baselines exist.

**[INFERENCE]** This order minimizes simultaneous unknowns. It is not a claim that remote draft or TP will outperform replication. E82-53 may admit none.

## Rollback points

| Point | Preserve | Trigger | Rollback action |
|---|---|---|---|
| R82-0 source | frozen bundles, patch manifest, prior tag | provenance/build failure | restore last verified anchor and rebuild offline |
| R82-1 backend | CPU/reference and prior backend profile | quality/op regression or driver instability | disable candidate backend/profile |
| R82-2 fabric | upstream baseline carrier and single-link plan | corruption, reorder, jitter, reset, or patch regression | deactivate specialized carrier/multipath |
| R82-3 mode | replication plan and single-node manifests | coupled correctness/value/failure miss | drain sessions; route new work to replication/single node |
| R82-4 cache | cache-off recomputation path and old reader where safe | incompatible schema, corruption, isolation, endurance issue | disable reads/writes; quarantine by generation; recompute, never reinterpret |
| R82-5 release | prior package/config/schema/runbook and backup | install/upgrade/security/soak failure | stop candidate, restore compatible prior package/config; migrate only through tested path |

## Minimum useful product boundaries

**MUP-1** supports exactly one qualified model/backend/context and a documented API subset. It excludes coupled execution and persistent cache.

**MUP-2** adds two-node availability/capacity through independent replicas. It does not claim one request uses two GPUs.

**MUP-3** adds one coupled plan only if evidence beats the comparator or enables an otherwise infeasible admitted model. A no-go decision is an acceptable completion of P5.

**MUP-4** adds safe persistent restore. “Cache off” remains a first-class recovery mode.

## Cross-project evidence work packages

| Unresolved evidence/decision | Required before | Output |
|---|---|---|
| Sections 50-53 candidate carrier/protocol contracts and unrun experiments | E82-41 | accepted carrier, multipath, framing/credit/integrity/security ADR plus conformance evidence |
| Sections 57-60 candidate fingerprint/restore/storage/sharing contracts and unrun experiments | E82-60 | accepted schema, state ownership, object/index layout, suffix equivalence and isolation evidence |
| Sections 70-72 contingent deployment/security/compatibility choices and unrun rehearsals | E82-70 | supported deployment tuple, threat/identity decision, upgrade/rollback/backup runbooks and receipts |
| Sections 74,76,77,79-81 zero-measurement verification plans | corresponding gates | paired single-node oracle, mode/cache value, stress/fault results, calibrated CI/release policy |
| Sections 83-86 governance decisions | P0 onward | owned risk register, executable experiment cards, freshness watch, issue/milestone/ADR/review workflow |

**[RECOMMENDATION]** Resolve these through their authoritative pages, ADRs, experiments, and reviews before dependent promotion. Exploratory code may proceed, but it must remain a candidate and must not define protocol or persistent-format authority by accident.
