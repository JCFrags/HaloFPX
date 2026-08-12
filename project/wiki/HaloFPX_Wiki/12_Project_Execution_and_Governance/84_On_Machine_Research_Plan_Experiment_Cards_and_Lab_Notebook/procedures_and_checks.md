---
section_id: "84"
title: "On-Machine Research Procedures, Experiment Cards, and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["Custom_Inference_Project", "HaloFPX integration repository (not yet frozen)"]
  software_versions: ["experiment card schema proposal v0.1"]
  hardware_revisions: ["two matched AMD Strix Halo systems; exact inventory pending"]
related_sections: ["03", "04", "05", "18", "20", "22", "24", "25", "29", "38", "55", "65", "73", "75", "76", "77", "78", "79", "80", "81"]
---

# On-machine research procedures, experiment cards, and checks

No command below was run. Placeholder paths and interfaces must be resolved during E01. Commands are examples to capture or validate state, not evidence of capability.

## Standard experiment-card contract

Every `card.yaml` must contain: stable experiment ID/version; status/owner/reviewer; hypothesis and falsifier; setup/SUT boundary; prerequisites and dependency gates; safety class, elevation need, stop/rollback plan; exact commands/argv; environment snapshot; independent/dependent/controlled variables; control/baseline and randomized order; raw-output paths and schemas; analysis/statistics and exclusions; acceptance/stop rules; conclusion; confidence/uncertainty; follow-up; originating section IDs; open-question, issue, requirement, ADR, and alias links. Blank fields are explicit `null`, never silently omitted. The exact required structure is [`experiment-card.schema.json`](experiment-card.schema.json); start from [`experiment-card.template.yaml`](experiment-card.template.yaml), not from prose.

Before execution: validate IDs, resolve exact targets, freeze/hash card and inputs, allocate run IDs, record clock state, and run a no-op collector check. After execution: preserve failures, finalize SHA-256, validate schemas, derive only from hashed inputs, independently reproduce the summary, record deviations, and update links. A changed card version requires new run IDs.

Validate all ten cards and the full alias map from the repository root:

```powershell
python wiki/HaloFPX_Wiki/12_Project_Execution_and_Governance/84_On_Machine_Research_Plan_Experiment_Cards_and_Lab_Notebook/validate_experiment_cards.py --format json
```

Exit zero and `"valid": true` prove structural conformance only. They do not approve a card. A card with unresolved `null` command, target, authorization, analysis, or stop fields remains `draft` and must not execute.

## Common safe preflight

Prerequisites: shell access to both hosts, sufficient non-workspace artifact storage, pinned checkouts/model bytes, stable host/rank/link IDs, and an approved card. Root is not required for these read-only examples:

```bash
date --iso-8601=ns
uname -a
git -C /path/to/checkout rev-parse HEAD
git -C /path/to/checkout status --porcelain=v1
sha256sum -- /path/to/model /path/to/card.yaml
lspci -nn
ip -details link show
ip route show table all
timedatectl show --property=NTPSynchronized --property=Timezone
```

Use `sudo` only when the card names the exact approved operation. Never run clock stepping, cache dropping, governor changes, cable pulls, device reset, cgroup OOM, filesystem fill, or corruption as common preflight.

## `HLX-EXP-20260717-841` - identity, topology, and clock admission

- **Hypothesis:** both hosts and two logical USB4 links can be identified reproducibly, and clocks expose enough state to choose valid timing methods.
- **Setup/variables/controls:** collect twice per host; compare expected volatile fields only. Inventory BOM, BIOS/firmware, kernel, ROCm/Mesa, devices, NUMA/IOMMU, NVMe, ports/cables, routes, clocksource, NTP/PTP/PHC availability. Control is repeat capture without reconfiguration.
- **Commands/prerequisites/safety:** common preflight plus `cat /sys/devices/system/clocksource/clocksource0/{current_clocksource,available_clocksource}` and, if installed, `ethtool -T <resolved-iface>`; read-only, normally no root.
- **Raw outputs/analysis:** two node environment records, topology graph, alias-to-stable-ID map, diff report, and time-capability matrix. Do not normalize away device/firmware asymmetry.
- **Conclusion/confidence/follow-up/links:** pass only if identities and differences are explainable and every later card can reference exact hosts/ranks/links. Otherwise block. Feeds Sections 18-23, 67, 73, 75 and Section 04 decisions.

## `HLX-EXP-20260717-842` - measurement-system and notebook qualification

- **Hypothesis:** collectors, clocks, sensors, artifact capture, and derivation are repeatable and do not materially perturb the tested workload.
- **Setup/variables/controls:** paired collector-off/on no-op and representative-load blocks; vary cadence and time-sync method; control same-clock RTT/echo. Measure pre/post offset and uncertainty, missing samples, cadence jitter, CPU cost, data loss, hash/restore behavior, and analysis regeneration.
- **Commands/prerequisites/safety:** `ptp4l`, `phc2sys`, or `pmc` only after E01 proves support and the card approves configuration; observation commands may require root. Do not infer bounds from `NTPSynchronized=yes`.
- **Raw outputs/analysis:** event logs from both hosts, offset/uncertainty records, collector-off/on paired metrics, corrupted-copy rejection receipt, pointer restore receipt, and independently regenerated summary.
- **Conclusion/confidence/follow-up/links:** pass per metric class; prohibit one-way timing or a sensor when its uncertainty/perturbation is unacceptable. Feeds Sections 05, 22, 27, 73, 75, 79, 81.

## `HLX-EXP-20260717-843` - matched single-node backend and model baseline

- **Hypothesis:** each viable model/quantization has a correct, stable single-node HIP and/or Vulkan reference with a measured resource envelope on both hosts.
- **Setup/variables/controls:** exact model/tokenizer/workload; vary host, backend, context, batch/ubatch, concurrency, KV type, flash attention, cache state, and supported optimization. Control is CPU/reference or highest-trust backend oracle as defined by Sections 29-37/78.
- **Commands/prerequisites/safety:** pinned build and model; use the owning section's exact benchmark CLI, `llama-bench`/server harness only when its metric scope matches. Root only for separately approved tuning/telemetry.
- **Raw outputs/analysis:** logits/tokens/state round trips, op/fallback traces, placement, memory, load/warmup, prompt/decode/TTFT/ITL/E2E distributions, power/thermal series, run variance, and failures.
- **Conclusion/confidence/follow-up/links:** correctness gate precedes performance. Establish host-specific baselines and viable backend/model cells; do not select distributed architecture. Feeds Sections 24-37, 73-74, 78.

## `HLX-EXP-20260717-844` - dual-link fabric and GPU-to-peer-GPU path

- **Hypothesis:** link A, link B, and both-link policies have proven binding, integrity, independence, scaling, tail behavior, and bounded degradation for actual message shapes.
- **Setup/variables/controls:** both directions; payload, queue depth, concurrency, uni/bidirectional, single/dual rail, TCP/approved probe carrier, CPU/GPU endpoints, idle/decode/NVMe contention. Control is identical codec and one rail.
- **Commands/prerequisites/safety:** path-specific bind/address/route must be explicit. Representative host baseline: `iperf3 -s --bind <link-address>` and paired client with declared options; use only a pinned, reviewed GPU probe. Kernel/module or physical fault work is deferred to E09.
- **Raw outputs/analysis:** route/socket binding proof, payload-delivery validation, RTT and admissible one-way records, bandwidth/tails, counters/interrupts/retries, CPU/power/thermal, GPU synchronization, link independence/scaling, and single-link degradation.
- **Conclusion/confidence/follow-up/links:** produce admissible carrier/policy regions; a fast corrupt or ambiguously bound run fails. Feeds Sections 49-55, 75 and transport ADRs.

## `HLX-EXP-20260717-845` - buffer visibility, coherence, copy, graph, and synchronization

- **Hypothesis:** candidate HIP/Vulkan mapped, registered, staged, and graph-reused paths preserve visibility/order and reduce end-to-end cost at observed payloads.
- **Setup/variables/controls:** CPU staging is control; vary allocation/memory type, direction, producer/consumer, synchronization primitive, copy count, alignment, size, reuse/capture, and transport carrier.
- **Commands/prerequisites/safety:** run reviewed conformance probes from Sections 24/25/54; no arbitrary device-memory reads or unsupported peer assumptions. Root only for approved driver tracing.
- **Raw outputs/analysis:** pattern/guard validation, visibility litmus outcomes, explicit synchronization boundaries, copy attribution, graph compatibility, latency/tails/CPU/power, and profiler perturbation.
- **Conclusion/confidence/follow-up/links:** reject any stale/torn read. Promote a path only for proven backend/memory/message cells. Feeds Sections 24, 25, 32, 45, 54.

## `HLX-EXP-20260717-846` - HaloKV correctness, durability, performance, and endurance

- **Hypothesis:** rank-local persisted state restores exact compatible continuations, rejects every incompatibility/corruption, survives declared crash points, isolates tenants, and has bounded write/performance costs.
- **Setup/variables/controls:** cold recomputation is control; vary model/state kind, prefix, tier, backend/rank/topology, compatibility input, crash point, corruption, quota/pressure, prefetch, migration, and workload locality.
- **Commands/prerequisites/safety:** only disposable cache roots and copies; record resolved path and capacity. Disk-full uses quota/loopback/sacrificial storage under E09, never the workspace/model/evidence filesystem.
- **Raw outputs/analysis:** suffix-only oracle, hit/miss reason, restored/evaluated tokens, state hashes, I/O/latency, write amplification, SMART/endurance deltas, crash journal, quarantine/recovery, isolation tests.
- **Conclusion/confidence/follow-up/links:** silent incompatible or corrupt acceptance is an immediate fail. Performance claims require correct restore and matched cold control. Feeds Sections 56-65, 77 and cache ADRs.

## `HLX-EXP-20260717-847` - cache-off and cache-integrated distributed-mode matched matrices

- **Hypothesis:** one or more of replication, native/local/remote speculation, two-way tensor parallelism, pipeline, or MoE hybrid has a correctness-preserving measured advantage in a defined workload region.
- **Setup/variables/controls:** first run the mandatory `cache-off` path with persistent cache disabled and its absence proven. E03 per-host baselines and two independent replicas are controls. Vary mode, model, prompt/output bin, concurrency, topology/plan, both-link policy, microbatch, speculative depth, and expert placement. A separately labeled optional `cache-integrated` path adds cache state and matched cold/hit/miss conditions.
- **Commands/prerequisites/safety:** cache-off requires E03, E04, and E05 only. Cache-integrated additionally requires E06 pass. Exact coordinator/rank argv, ownership, and cache-disable/enable evidence must be recorded. No performance-only run can bypass token/logit/state oracle.
- **Raw outputs/analysis:** store cache-off and cache-integrated records in separate raw streams. Both include client metrics plus stage/collective/activation/speculation/expert/link traces, paired uncertainty, memory, energy, failure denominator, break-even model, and held-out inputs; only the integrated stream may attribute cache traces or gains.
- **Conclusion/confidence/follow-up/links:** conclusions are region- and path-scoped. A cache-off result selects architecture candidates without requiring HaloKV. A cache-integrated conclusion is invalid unless E06 passed and matched cache controls are present. E09/E10 still gate. Feeds Sections 38-48, 55, 75-76, 78.

## `HLX-EXP-20260717-848` - service envelope, lifecycle, concurrency, and soak

- **Hypothesis:** the selected candidate and single-node fallback maintain declared API/lifecycle semantics, fairness, bounded resources, observability, and thermal stability under representative service loads.
- **Setup/variables/controls:** vary cold/warm lifecycle, concurrent sessions, short/long prompts, interactive/batch arrivals, cancellation, cache locality, reload, native/container deployment, and duration. Control is clean pinned single-node service.
- **Commands/prerequisites/safety:** use pinned API/load clients and service units; test paths/secrets redacted. Long runs require storage/thermal stop limits. Container/system changes require reversible deployment plan.
- **Raw outputs/analysis:** conformance, queues/fairness, TTFT/ITL/goodput/tails, resource slopes, cache churn, telemetry, cardinality/privacy, reload/cancel/reclaim, deployment parity and cold boot.
- **Conclusion/confidence/follow-up/links:** no readiness claim if starvation, leak, thermal drift, secret exposure, or fallback failure exists. Feeds Sections 46, 66-72, 79.

## `HLX-EXP-20260717-849` - fault, security, recovery, and rollback

- **Hypothesis:** approved protocol/process/cache/filesystem/transport/device faults fail explicitly, preserve committed state, converge ownership, and recover within recorded bounds without collateral loss.
- **Setup/variables/controls:** clean E07/E08 run before and after each isolated fault; one injection at a time before approved combinations. Exact injection ordinal and removal time are variables.
- **Commands/prerequisites/safety:** Section 80 mandatory preflight, isolated deployment, hashed copies, out-of-band access, emergency stop, rollback, and second operator for cable tests. Elevation normally required. No generic destructive command is supplied here.
- **Raw outputs/analysis:** synchronized fault ledger, request outcomes, rank epochs, kernel/service logs, before/after hashes, recovery times, resource cleanup, clean rerun, and collateral-scope audit.
- **Conclusion/confidence/follow-up/links:** any silent wrong output, bad-state acceptance, split brain, unbounded hang/retry, host-wide OOM, or loss outside disposable targets fails and triggers safety review. Feeds Sections 48, 53, 56-64, 71-72, 80, 83.

## `HLX-EXP-20260717-850` - randomized holdout and independent reproduction

- **Hypothesis:** frozen candidate conclusions reproduce from raw data and predict unseen workload/environment blocks without material regression.
- **Setup/variables/controls:** freeze card, code, analysis, thresholds, and candidate before revealing randomized holdout order. A different operator/agent regenerates summaries and repeats selected cells on both hosts.
- **Commands/prerequisites/safety:** non-destructive unless a separately approved E09 subset is included. Execute only pinned commands from prior cards; no tuning after holdout inspection.
- **Raw outputs/analysis:** holdout run bundles, reproduction receipts, raw/derived hashes, paired effects/uncertainty, prediction error, threshold decisions, and every deviation.
- **Conclusion/confidence/follow-up/links:** pass permits evidence attachment to an ADR/release gate; failure or irreproducibility blocks promotion and creates a gap. Feeds Sections 04, 73, 76, 78, 81, 82, 86.

## Notebook closeout checklist

- [ ] Both hosts use one run ID and distinct stable node/rank IDs.
- [ ] Pre/post clock offset and uncertainty are present or cross-host timing is explicitly disabled.
- [ ] Exact commands, exit codes, failures, deviations, and missing samples are retained.
- [ ] Raw artifacts are hashed before analysis; derived outputs cite input hashes.
- [ ] Card conclusion states scope, uncertainty, confidence, gate result, and falsifying evidence.
- [ ] Owning questions/decisions are linked; no wiki claim is promoted without raw and environment links.
- [ ] A reviewer checked correctness, freshness, clarity, provenance, safety, and reusable improvement.
