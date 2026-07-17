---
type: intake-review
status: proposed
created: 2026-07-17
reviewer_scope: "dual-node validation and 200-230 GB model intake packages"
targets:
  - "sources/imports/2026-07-17-further-research-wikis/extracted/dual-node-strix-halo-validation-wiki-2026-07-17"
  - "sources/imports/2026-07-17-further-research-wikis/extracted/llm-wiki-200-230gb"
canonical_wiki_changed: false
decision: revise-before-promotion
---

# Dual-node large-model intake review

## Review decision

**Overall verdict: REVISE BEFORE PROMOTION.** Both packages are useful research candidates, but neither is an approved implementation plan, capacity proof, model qualification, benchmark result, or release policy.

- **Dual-node validation package:** **PROMOTE SELECTIVELY.** Its evidence boundary, experiment structure, matched-baseline discipline, metric definitions, fault taxonomy, and explicit `INSUFFICIENT_EVIDENCE` state are strong. Its numeric gates are unapproved policy, its evaluator trusts self-asserted summary fields, and it does not close model-specific state ownership or single-node fallback.
- **200–230 GB model package:** **REVISE.** Its unit conversion, deterministic planning arithmetic, provenance workflow, and no-performance-extrapolation warnings are useful. Its `2x128` profile overstates the observed target capacity, several apparent fit margins disappear on the real machines, exact model/backend support is not qualified at project pins, and equal arithmetic splits are not realized placement evidence.

No target-size tokens/second, latency, scaling, power, or thermal result was found presented as a machine measurement. The main claim risks are planning `FIT` labels and heuristic shortlist scores being mistaken for facts, and candidate numeric release thresholds being mistaken for approved HaloFPX policy.

## Scope and authority

This review used, in descending authority:

1. [`PROJECT_GOAL.md`](../../PROJECT_GOAL.md), which makes 200–230 GB work Phase 2 after the integration fork passes Phase 1 stability gates.
2. Project and Agent Harness evidence rules: [`AGENTS.md`](../../AGENTS.md), [`references/agent-harness.md`](../../references/agent-harness.md), and the canonical Agent Harness `AGENTS.md`, `guide/architecture.md`, and `reviews/AGENTS.md`.
3. Canonical HaloFPX model, distributed-runtime, and verification Sections 19, 29–30, 32, 34, 38–48, and 73–81.
4. Preserved live target evidence under [`sources/measurements/2026-07-17-strix-halo-live-inventory/`](../../sources/measurements/2026-07-17-strix-halo-live-inventory/).
5. The two extracted intake candidates and their preserved ZIPs. The import receipt records 124/124 and 118/118 extracted entries and SHA-256 values `9cef69fb935c5f2284d4fe4343bc6b26b0dfe7669b1c6c563aa7ebf44984a1cf` and `720a50cc451c8e6b85fdca6331c87b01667d53c8a687e1ff9df96b8e0b6279f1` respectively ([`import-receipt.md`](../../sources/imports/2026-07-17-further-research-wikis/import-receipt.md):17-20).

The extracted packages remain source-layer candidates. No imported script was executed during this review, and no canonical Wiki page was edited.

## Findings requiring revision

| Priority | Finding | Verdict |
|---|---|---|
| P0 | The validation gate evaluator accepts favorable summary declarations without proving their raw evidence | Block evaluator promotion as release authority |
| P1 | The `2x128` capacity profile does not match the approximately 124 GiB target envelope | Recalculate from captured per-node limits and measured allocator state |
| P1 | Model architecture support is source-presence evidence at different pins, not exact artifact/backend qualification | Keep every candidate `OPEN` until exact-pin preflight passes |
| P1 | Equal split arithmetic and `tensor-split 1,1` do not prove tensor/layer/KV ownership | Require a realized placement and ownership manifest plus measurements |
| P1 | Fault testing is useful, but output commit, replay, fencing, rejoin, and fallback are unresolved | Promote fail-closed rules; keep continuation/fallback `OPEN` |
| P1 | Numeric regression, reliability, recovery, soak, headroom, and scaling gates are not approved | Retain only as candidate examples pending pilot evidence and owner decision |
| P1 | nimo-1 cannot currently stage a 200–230 GB artifact or a similarly sized transfer cache | Resolve storage topology before target artifact acquisition/load tests |
| P2 | Candidate scores imply precision unsupported by published component subscores or selected-quant quality data | Do not promote ranking or scores |
| P2 | Imported source claims use pins different from the canonical and deployed baselines | Rebase every promoted claim to exact project/deployment commits |

## Capacity and storage audit

### The target is not `2 x 128 GiB` in the planning sense

The planning package declares each RPC member as `128.0 GiB` ([`data/profiles.json`](../../sources/imports/2026-07-17-further-research-wikis/extracted/llm-wiki-200-230gb/llm-wiki-200-230gb/data/profiles.json)). The live capture instead reports 130,491,708 KiB and 130,491,700 KiB of physical memory, approximately 124.45 GiB per node, and 133,143,986,176 bytes of configured GTT capacity, exactly 124 GiB ([`nimo-1.md`](../../sources/measurements/2026-07-17-strix-halo-live-inventory/nimo-1.md):7-18; [`nimo-2.md`](../../sources/measurements/2026-07-17-strix-halo-live-inventory/nimo-2.md):7-18). Canonical Section 19 explicitly says this inventory does not establish a 200–230 GB model budget.

Using the package's own totals but substituting only the observed physical ceiling removes about 3.55 GiB from every per-node margin; the configured GTT ceiling removes 4 GiB. This materially changes the conclusions:

| Candidate/context | Package coordinator total | Package margin at 128 GiB | Margin at 124.45 GiB physical | Margin at 124 GiB GTT | Review disposition |
|---|---:|---:|---:|---:|---|
| Qwen3 128K | 121.25 GiB | +6.75 | about +3.20 | +2.75 | Narrow planning lead only |
| Qwen3 native 262K | 127.50 GiB | +0.50 | about -3.05 | -3.50 | Does not fit observed envelope |
| Step 128K | 119.75 GiB | +8.25 | about +4.70 | +4.25 | Candidate for measured preflight |
| MiMo 128K | 124.00 GiB | +4.00 | about +0.45 | 0.00 | No safe allocator margin |
| MiMo native 262K | 124.75 GiB | +3.25 | about -0.30 | -0.75 | Does not fit observed envelope |

These are still arithmetic envelopes, not load proofs. They assume the package's selected artifact size, KV formulas, one sequence, `ubatch=512`, fixed OS/runtime/skew reserves, and quarter-GiB equal division. Real layers and non-layer tensors are indivisible; graph/work buffers, allocator fragmentation, runtime duplication, RPC transfer cache, page cache, concurrent slots, MTP/recurrent state, and backend-specific workspaces must be observed.

The running 121,861,632,736-byte model establishes a useful predecessor load point: nimo-1's RPC process used about 58.9 GiB RSS/76.0 GiB cgroup memory, while nimo-2 used about 64.4 GiB RSS/79.6 GiB cgroup memory. It does **not** prove a 200–230 GB artifact will load or retain safe headroom ([`comparison.md`](../../sources/measurements/2026-07-17-strix-halo-live-inventory/comparison.md):24-30).

### Storage blocks the first large-artifact trial

nimo-1 had only about 43 GiB free while holding about 112 GiB of RPC tensor cache; nimo-2 had about 318 GiB free ([`comparison.md`](../../sources/measurements/2026-07-17-strix-halo-live-inventory/comparison.md):10-21). Therefore:

- nimo-1 cannot currently hold a 200–230 decimal-GB artifact, a complete second copy, or a comparably large new RPC transfer cache;
- nimo-2 may hold one target artifact, but staging, verification, temporary conversion files, rollback copy, and cache growth need an explicit reserve plan;
- the present RPC cache cannot be counted as HaloKV and lacks the required cryptographic read verification, atomic publication, quota, and corruption-as-miss behavior ([`rpc-cache-audit.md`](../../sources/measurements/2026-07-17-strix-halo-live-inventory/rpc-cache-audit.md)).

Capacity promotion therefore requires both a memory budget and a storage/staging/rollback budget.

## Model and quantization support audit

The model package pins llama.cpp `6bdd77f13cf11b264b4231d320afc404f48d576e` and ROCmFPX `61f2f2d7bc4955e9bca821095ef69125837133b5` ([`pages/Runtime-Support.md`](../../sources/imports/2026-07-17-further-research-wikis/extracted/llm-wiki-200-230gb/llm-wiki-200-230gb/pages/Runtime-Support.md):3). Canonical research uses llama.cpp `788e07dc91d266ad3162a1ce9037665656269689` and ROCmFPX `a5605a72768c6562241b248e268e33dc92787394`; the live predecessor runs `rocmfp4-llama@4860505ee322091f0f61eba77d6ad49be88cf4ea`.

An architecture implementation existing at an intake pin is useful source evidence, but it does not establish that:

- the exact community/publisher artifact converts and loads;
- its GGUF metadata and tensor names match the implementation;
- all selected quant types have CPU, HIP/gfx1151, Vulkan, and RPC operations;
- MLA, sliding-window, recurrent/SSM, MoE, MTP/speculative, multimodal, tokenizer, and template state are preserved correctly;
- selected-quant quality remains acceptable;
- the canonical integration fork contains the same implementation after rebasing.

This matches canonical Section 29: architecture recognition is not a compatibility promise. No intake candidate should be promoted beyond `research lead` until exact artifact hashes, converter/runtime pins, metadata inventory, backend-op trace, load trace, tokenizer/template tests, quality comparison, and distributed-state tests pass.

The shortlist scores (`92`, `90`, `86`, and so on) are deployment heuristics, not measurements ([`pages/Ranked-Shortlist.md`](../../sources/imports/2026-07-17-further-research-wikis/extracted/llm-wiki-200-230gb/llm-wiki-200-230gb/pages/Ranked-Shortlist.md):3-31). They do not publish per-component scored evidence, and several selected quants lack direct evaluation. Do not promote the scores, rank order, "least surprising," "best," or similar comparative wording as project facts.

## Placement, ownership, failure, and fallback

The live predecessor topology is explicit: nimo-2 owns the model and LAN API, nimo-1 owns the private RPC device, the command orders `RPC0,ROCm0`, and layer split is `1,1`. This proves a current placement command and healthy load, not balanced realized bytes, rank-synchronous tensor parallelism, or a future HaloFPX ownership protocol.

The validation package describes coordinator and worker roles but its SUT template records free-form commands and split fields rather than an enforceable ownership contract (`config/sut.example.yaml:67-94`). The large-model package divides weights and KV evenly in quarter-GiB planning units. Neither specifies or proves:

- layer/tensor ranges and hashes owned by each rank;
- embedding, output head, final norm, logits reduction, sampler/RNG/grammar, tokenizer, and authoritative output owner;
- rank-local versus shared KV, recurrent, MTP, cache, and checkpoint state;
- output commit/ack boundary, retry idempotence, epoch fencing, late-completion rejection, or cancellation;
- atomic cross-rank checkpoint identity and restart/rejoin rules.

Canonical Sections 39, 43, 45, and 48 deliberately keep several of these decisions open. Promotion requires a machine-readable plan manifest plus load logs and memory deltas proving realized placement; `--tensor-split 1,1` alone is insufficient.

The validation package's fail-explicitly rule, reversible-fault-first policy, retained failure records, and post-recovery canaries are promotion candidates. Transparent continuation is not. For a 200–230 GB target that cannot fit one node, "single-node fallback" must mean either:

1. explicit bounded failure and restart after the peer returns;
2. a separately qualified smaller/reduced model and a new topology/session epoch; or
3. replay from an independently validated checkpoint only if the full fallback plan actually fits.

It must not mean silently continuing a partial distributed state. Canonical Section 48 requires restart from the original prompt when a mutually validated checkpoint cannot be proven.

## Validation package evaluator and evidence binding

The package correctly labels itself D0/design-only and says synthetic examples are not machine evidence ([`README.md`](../../sources/imports/2026-07-17-further-research-wikis/extracted/dual-node-strix-halo-validation-wiki-2026-07-17/dual-node-strix-halo-validation-wiki-2026-07-17/README.md):1-19; [`EVIDENCE-STATUS.md`](../../sources/imports/2026-07-17-further-research-wikis/extracted/dual-node-strix-halo-validation-wiki-2026-07-17/dual-node-strix-halo-validation-wiki-2026-07-17/EVIDENCE-STATUS.md):1-25). That discipline should be retained.

However, `tools/evaluate_gates.py:55-177` trusts summary booleans/numbers for completed experiments, matching, raw-hash verification, SLO outcomes, fault IDs, and performance ratios. It does not require the separate provenance audit, recompute metrics from immutable raw records, or verify a complete experiment-to-input hash chain. `schemas/summary.schema.json:74-164` similarly permits declarative completion fields and an optional/nullable packet digest.

**Disposition:** the evaluator may be studied as a UI/reporting prototype, but it must not authorize a HaloFPX release. Before promotion it must:

1. ingest and verify a provenance-audit artifact;
2. bind every experiment result and derived metric to immutable raw input hashes and exact code/config identity;
3. verify required raw-record coverage and failure denominators;
4. reject path/hash mismatches and missing records;
5. support the documented waiver state with verifiable identity, scope, expiry, and nonwaivable enforcement;
6. pass a negative fixture containing favorable forged summary fields but no evidence, returning `INSUFFICIENT_EVIDENCE`.

## Benchmark controls and acceptance gates

### Promotion candidates

The following concepts align with canonical Sections 73–80 and are suitable for selective reconciliation:

- one frozen SUT/workload/cache-state per run and independent run blocks;
- client/server/host/accelerator/link measurement layers;
- paired prompt IDs across Node A, Node B, and dual topology;
- randomized or counterbalanced order and fixed model/tokenizer/context/sampling/environment;
- failed, cancelled, rejected, and timed-out requests retained;
- cache states measured separately, including correctness cache-on versus cache-off;
- same-layer USB4 numerator and denominator definitions rather than signaling-rate claims;
- capacity-extension wording: no speedup ratio when the large workload cannot run on one node, plus a smaller matched control;
- raw-first records, immutable provenance, explicit `INSUFFICIENT_EVIDENCE`, and reversible fault injection.

### Do not promote numeric policy

The following values are candidate policy without HaloFPX pilot variance, product SLO approval, or release-authority decision:

- 3%/5% performance regression warnings/failures and 5%–15% latency cutoffs (`config/regression-thresholds.yaml:10-80`);
- 1.15x scale-out goodput, 1.10x/1.15x latency ratios, and 10% capacity headroom (`config/release-gates.yaml:131-150`);
- 99.99% request success, 72-hour soak, 60-second evidence gap, and three-of-three fault repetitions (`config/release-gates.yaml:92-130`);
- 10/30/120-second recovery targets (`config/slo-policy.yaml:17-21`);
- 95% CI-width, request-count, tail-quantile, 20% saturation-headroom, and warmup defaults in `wiki/Benchmark-Methodology.md`;
- exact quality-drift tolerances in `wiki/Correctness-Program.md`.

Canonical Sections 73 and 81 keep estimator, repetition, practical-effect, tail sample, recovery, soak, and release thresholds open. In particular, 200 successful requests is too weak to justify a stable p99 tail claim, and ordinary 200/1,000-request cells cannot establish a 99.99% reliability floor without a separately approved exposure/confidence plan.

## Promotion candidates by package

| Candidate artifact or idea | Decision | Conditions |
|---|---|---|
| Validation package evidence boundary and `INSUFFICIENT_EVIDENCE` state | Accept concept | Map to canonical claim labels and provenance rules |
| SUT freeze checklist | Accept with revision | Add canonical pins, actual nimo roles, realized placement, ownership, storage reserve, and security fields |
| Metric definitions and paired A/B/dual controls | Accept with reconciliation | Resolve any differences against Section 73; preserve denominator and clock scope |
| Experiment-card structure and fault taxonomy | Accept as source material | Deduplicate/map into Section 84; do not create a second canonical experiment namespace |
| Raw schemas and collectors | Defer | Code review, threat review, schema mapping, fixture tests, overhead measurement, and hash binding required |
| Gate evaluator | Reject as authority | Evidence-chain remediation and adversarial fixtures required |
| Numeric release/regression/SLO values | Reject as defaults | Human approval after pilot distributions and product requirements |
| Decimal-GB to binary-GiB conversion and `ceil(payload)+1` planning method | Accept as planning method | Label assumption; substitute actual machine capacity and artifact bytes |
| Capacity calculator and KV formulas | Accept as candidate tool | Independent formula review, actual GGUF metadata, allocator traces, multi-sequence cases, and target profiles |
| Candidate manifests/source index | Accept as research backlog | Refresh exact revisions, licenses, hashes, and project-pin applicability |
| `FIT` tables | Reject as machine facts | Regenerate for 124/124.45 GiB limits and validate by staged load |
| Shortlist scores/ranking | Reject | Replace with evidence matrix and explicit user-approved weights if ranking is still desired |
| Performance-open/no-extrapolation warnings | Accept | Preserve prominently in every derivative artifact |

## Implementation prerequisites

No 200–230 GB Phase 2 implementation should begin until all of these are satisfied:

1. **Phase 1 gate:** the canonical ROCmFPX integration fork builds reproducibly on both nodes and passes baseline, server, cache, corruption, recovery, and single-node development gates.
2. **Exact target selection:** publisher/model revision, license disposition, tokenizer/template, selected quant, shard list, exact bytes, and cryptographic hashes are approved.
3. **Pin reconciliation:** candidate architecture and quant support is re-audited at the exact integration-fork commit and both deployed binaries/libraries are attributable to it.
4. **Storage plan:** nimo-1 headroom is remediated; download, staging, verification, conversion, cache, rollback, and reserve locations are defined without deleting preserved evidence.
5. **Measured memory envelope:** per-node weights, KV/special state, graph/work buffers, process/cgroup/GTT use, `MemAvailable`, swap/PSI, fragmentation, and concurrency growth are captured.
6. **Placement/ownership plan:** every tensor/layer and distributed state owner, sampler/output authority, plan hash, epoch, failure behavior, and rollback path is machine-readable.
7. **Transport/security gate:** exact RPC source and deployed provenance, private binding/firewall, peer trust, dual-rail characterization, and protocol integrity are verified. Unmodified RPC remains lab-only.
8. **Fallback decision:** explicit failure versus reduced-model routing versus validated restart/replay is selected for every workload; no partial state is accepted.
9. **Benchmark contract:** Section 73 schema/control integration, matched baselines, failure denominators, tail/exposure sufficiency, sensor calibration, and raw-data routing are approved.
10. **Policy approval:** Section 81 thresholds, waivers, soak/recovery budgets, and release authority are decided from pilot evidence rather than imported defaults.

## Required experiments

| ID | Experiment | Minimum decision evidence |
|---|---|---|
| INTAKE-LM-01 | Storage and artifact-staging preflight | Exact artifact/shard bytes and hashes; free-space reserve; temporary and rollback peak; RPC-cache quota; safe cleanup/rollback plan |
| INTAKE-LM-02 | Exact architecture/backend preflight | GGUF metadata/tensor inventory, converter/runtime pins, tokenizer/template, CPU reference load, HIP/gfx1151 and Vulkan op coverage, RPC compatibility, unsupported-op failure |
| INTAKE-LM-03 | Progressive allocator and context fit | Load at minimal context, then staged 4K/32K/64K/128K/native; record per-rank RSS/cgroup/GTT/MemAvailable/KV/graph buffers/swap/PSI; clean OOM/reject behavior |
| INTAKE-LM-04 | Realized placement and ownership | Tensor/layer map and hashes, non-layer tensors, rank memory delta, KV/state owner, sampler/output owner, actual split skew, plan identity |
| INTAKE-LM-05 | Single-node denominator and capacity boundary | A-only/B-only same-family controls; documented safe target failure; no invalid target speedup ratio; reduced-model fallback qualification if selected |
| INTAKE-LM-06 | Single-rail and dual-rail transport | Directional/bidirectional application and interface goodput, tails, CPU cost, MPTCP subflow attribution, simultaneous load, one-rail loss, recovery independence |
| INTAKE-LM-07 | Distributed correctness and state | Local/distributed logits or approved oracle, cache-on/off, long context, MoE/MLA/MTP/recurrent state as applicable, cancellation, checkpoint/replay, output commit |
| INTAKE-LM-08 | Rank/link/process fault and fencing | Idle/prefill/decode faults, late completions, epoch fencing, explicit client outcomes, restart/rejoin, no silent continuation, post-recovery canaries |
| INTAKE-LM-09 | Matched performance and thermal matrix | Frozen model/settings, randomized paired runs, predeclared warmup/stopping, adequate tail/reliability exposure, all failures retained, wall power/thermals, raw records |
| INTAKE-LM-10 | Evidence-evaluator adversarial test | Forged favorable summary without raw records, wrong hashes, missing topology, stale pins, waiver misuse, and synthetic data must all return insufficient/fail |

These identifiers are review-local. Before execution, map or amend them into the canonical Section 84 experiment-card namespace rather than creating a competing experiment authority.

## Research gaps

1. Which exact 200–230 GB artifact and selected quant best serves the user's workload after selected-quant quality evaluation?
2. Which candidate architectures and tensor types work at the canonical integration-fork pin on both HIP/gfx1151 and Vulkan, including RPC paths?
3. What are the actual graph/workspace and per-sequence state costs for the exact artifact, backend, context, batch, slots, and distributed plan?
4. Can a load plan retain an approved reserve below the 124 GiB GTT ceiling without swap-in, pressure collapse, or allocator fragmentation?
5. How will model shards, RPC transfer cache, HaloKV, temporary conversion files, and rollback copies fit on the current SSDs?
6. Which rank owns embeddings, output head, sampling/RNG/grammar, KV/recurrent/MTP state, and authoritative output for each candidate mode?
7. Does coarse layer placement, actual tensor parallelism, pipeline placement, MoE-aware placement, or a hybrid win for the selected workload after communication cost?
8. Do both USB4 rails provide additive application goodput and independent failure behavior under simultaneous GPU/runtime traffic?
9. What safe reduced-model or restart behavior replaces impossible same-model single-node continuation?
10. What tail sample size, reliability exposure, warmup, confidence method, practical-effect threshold, soak duration, and recovery budget are approved after pilot data?
11. How will evaluator results be cryptographically bound to every raw record, derived metric, experiment receipt, waiver, and release decision?
12. What source/license/provenance changes exist between intake pins, canonical research pins, the integration fork, and the deployed predecessor?

## Final disposition

The packages should remain preserved under `sources/imports/` as candidate evidence. Promote only the explicitly listed concepts after reconciliation and independent validation. Do not copy the packages wholesale into the canonical Wiki, do not use their `FIT` labels or scores as selection authority, and do not enable their numeric gates or evaluator as release authority.

The most valuable next step is not downloading the top-ranked artifact. It is an approved Phase 2 preflight that first fixes nimo-1 storage headroom, regenerates capacity tables for the observed 124 GiB/GTT envelope, selects one exact artifact and canonical runtime pin, and executes `INTAKE-LM-01` through `INTAKE-LM-04` before any performance claim.
