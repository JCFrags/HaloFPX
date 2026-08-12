---
section_id: "84"
title: "On-Machine Research Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["Custom_Inference_Project", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: ["MLPerf Inference rules@c547732b539cb3a14cc5680597714c8c1df4cad0", "linuxptp@e312b959a7dd3e8db0c6c8e282917c89909f8f76"]
  hardware_revisions: ["HaloFPX dual-Strix-Halo target; exact inventory pending"]
related_sections: ["03", "04", "05", "18", "20", "22", "73", "75", "80", "81"]
---

# On-machine research facts and constraints

## Evidence precedence

- **[VERIFIED]** Project governance routes preserved evidence into wiki claims and only then into decisions or procedures; memory and prior runs are scoped experience, not automatic truth [S84-01][S84-02].
- **[VERIFIED]** HaloFPX stable identifiers distinguish experiment definitions from individual runs. A card uses `HLX-EXP-...`; each execution uses a separate `HLX-RUN-...` [S84-03].
- **[VERIFIED]** The project benchmark contract requires immutable identities, raw per-request/time-series records, separate warmup, declared populations, and derivation receipts. It reports no target-machine measurements [S84-04].
- **[VERIFIED]** MLPerf Inference rules require replicability, consistent systems/frameworks, fixed random seeds, and shared benchmark implementations. HaloFPX borrows these controls but does not claim MLPerf compliance [S84-05].

## Measurement and uncertainty

- **[VERIFIED]** JCGM 100:2008 treats a measurement result as incomplete without a statement of its uncertainty. It distinguishes repeated-observation statistical evaluation from evaluation based on other information [S84-06].
- **[RECOMMENDATION]** Every result used by an ADR must report the measured quantity, unit, observation boundary, sample population, uncertainty method, exclusions, and environmental applicability. A percentile without its population and sample count is not decision evidence.
- **[RECOMMENDATION]** Independent runs, not requests sharing one scheduler/thermal state, are the default resampling unit for run-to-run claims. Preserve every request-level observation as well.
- **[INFERENCE]** A matched pair of nominally identical hosts is not a control until firmware, device identity, cooling, power policy, software, model bytes, and observed baselines are captured. Host asymmetry is evidence, not noise to erase.

## Time model

- **[VERIFIED]** RFC 3339 defines an Internet timestamp representation; it does not provide a monotonic duration clock or a bound on synchronization error [S84-07].
- **[VERIFIED]** Linux exposes monotonic clocks that are not subject to discontinuous wall-clock jumps. UTC is appropriate for provenance; monotonic timestamps are appropriate for local durations [S84-08].
- **[VERIFIED]** RFC 8633 recommends monitoring synchronization and warns that time-source configuration and security affect correctness [S84-09].
- **[VERIFIED]** `ptp4l` synchronizes PTP clocks and `phc2sys` commonly synchronizes a system clock to a PTP hardware clock. Availability of a usable hardware timestamp path on either USB4 network interface is not established [S84-10].
- **[RECOMMENDATION]** Record UTC plus local monotonic timestamps on both hosts. For cross-host event ordering, capture offset and uncertainty before and after each block. Do not report one-way latency when the uncertainty interval is material relative to the effect being claimed; use same-clock RTT or an instrumented echo instead.

## Raw evidence and reproducibility

**[RECOMMENDATION]** A run bundle is append-only after finalization and contains:

| Family | Required content |
|---|---|
| identity | card version/hash, run ID, host/rank/link IDs, source/build/model/tokenizer/config/workload hashes |
| execution | exact argv arrays, allowlisted environment, UTC/monotonic boundaries, exit codes, stdout/stderr |
| observations | raw request/token/collective/cache/fabric/telemetry/fault records, including failures and missing samples |
| timing | clock source, synchronization state, offset/uncertainty probes before and after |
| integrity | file sizes and SHA-256 manifest finalized before analysis |
| derivation | analysis commit, command/config, input hashes, output hashes, exclusions, statistical method |
| review | conclusion, confidence, limitations, question closures, ADR/issue links, reviewer and review time |

**[VERIFIED]** W3C PROV defines entities, activities, and agents plus derivation relationships suitable for recording which analysis produced a result from which raw artifact [S84-11]. **[RECOMMENDATION]** HaloFPX may implement the same relations without claiming full PROV conformance.

## Safety boundaries

- **[RECOMMENDATION]** Read-only inventory and user-space benchmarks default to no root. A card must state why elevation is necessary for counters, governors, IRQ placement, cgroups, PTP, device setup, or fault injection.
- **[RECOMMENDATION]** Physical cable pulls, device reset, OOM, ENOSPC, corruption, kernel/module changes, clock stepping, and power-policy changes require an isolated target, recoverable evidence, exact target resolution, rollback, and the Section 80 preflight.
- **[RECOMMENDATION]** Never mutate the only copy of a model, cache, raw run, workspace, boot volume, home directory, or source checkout. Corruption tests operate on disposable hashed copies; cache corruption must yield rejection, miss, or recomputation.

## Measurements absent

**[OPEN]** The two hosts' clock sources, synchronization error, sensor behavior, BOM parity, dual-port independence, backend parity, steady-state envelope, and safe fault mechanisms remain unmeasured.

