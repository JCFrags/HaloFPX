# EXP-001 — Lab Preflight and SUT Freeze

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Prove that the named testbed is fully identified, isolated, observable, and safe to benchmark; quantify instrumentation overhead before accepting any performance sample. |
| Release profiles | All |
| Required evidence | G1 prerequisite; M1/M2 runs depend on this card |
| Estimated measured duration | 2–4 hours plus one reboot per node |
| Risk class | Low |

## Decision question

Can an independent operator reconstruct the exact hardware/software/model/topology and collect synchronized evidence without materially changing the workload?

## Hypotheses

- **H0:** Required provenance, clock, isolation, or collector-overhead controls are missing or outside bounds.
- **H1:** The SUT is frozen, fingerprints agree with the manifest, clocks are suitable, the RPC path is isolated, and collectors are within overhead limits.

## Preconditions and provenance

- Populate `config/sut.yaml` from `config/sut.example.yaml`; no required release/profile/SLO field may remain null.
- Freeze model and tokenizer artifacts by SHA-256; freeze runtime commit, build flags, kernel, driver/ROCm, BIOS/AGESA, USB4 firmware, power profile, and client placement.
- Define a dedicated test subnet. RPC must bind only to the isolated USB4 interface or loopback/tunnel boundary; do not expose upstream proof-of-concept RPC to an untrusted network.
- Capture ambient target, background-service suppression, storage free space, and rollback/console access.

## Factors, controls, and run order

- Topology: Node A single, Node B single, dual.
- Collector profile: disabled control versus normal 1 Hz profile versus short high-rate profile.
- Workload: `smoke-64x16`, deterministic sampling, model-resident state after first controlled load.
- Counterbalance collector-off/on order and repeat each comparison at least five times.

## Procedure

1. Create testbed and software fingerprints with `tools/collect_provenance.sh`; hash the output.
2. Verify UTC synchronization and measure cross-host offset; capture monotonic-to-UTC anchors at collector start/end.
3. Record USB4 topology, authorization, negotiated speed/lane count, interface addresses, MTU, routes, offloads, firewall, and listening sockets.
4. Run the deterministic smoke request on A, B, and dual. Confirm device placement and that removing the worker causes explicit—not silent—failure.
5. Run matched collector-disabled and collector-enabled blocks. Retain every request and telemetry record.
6. Validate records against schemas and finalize raw-file hashes.

## Required measurements

- Provenance completeness; raw-file hash verification; time offset and drift; listening/bind scope.
- Median throughput and p95 latency with/without collectors; collector CPU, memory, disk, and lost-sample counts.
- Canary status, output hash, runtime process tree, device assignment, and log completeness.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- Provenance completeness = 100%; all raw hashes verify; exact model/tokenizer/runtime IDs are present.
- Cross-host UTC offset ≤5 ms for decomposition traces; client-observed TTFT/ITL remain computed on one monotonic clock.
- Normal collector changes median throughput by ≤1% and p95 latency by ≤2%; otherwise reduce cadence and repeat.
- No RPC listener on an untrusted interface; firewall and route evidence match the declared security boundary.
- A, B, and dual smoke requests complete with 100% critical correctness and no crash/oops/reset.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- Auto-update, thermal/power-profile transition, or unrelated workload begins during the block.
- Fingerprint changes after the run starts.
- Collector loses >1 consecutive expected samples in a short control block.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

This card establishes lab readiness only. It does not demonstrate performance, stability, scaling, or fault recovery.

## Research basis

[[SRC-004]](../references/Sources.md#src-004) [[SRC-007]](../references/Sources.md#src-007) [[SRC-013]](../references/Sources.md#src-013)
