# EXP-002 — USB4 Link Characterization

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Measure the actual host-to-host payload capacity, latency, asymmetry, CPU cost, and stability of the exact USB4 cable/controller/network configuration. |
| Release profiles | All |
| Required evidence | M1 link reference before M2 dual inference |
| Estimated measured duration | 3–5 hours plus 2-hour stability window |
| Risk class | Low |

## Decision question

What application-level network envelope is available to the runtime, and does the link remain negotiated and error-free under uni- and bidirectional load?

## Hypotheses

- **H0:** Goodput, loss, retransmission, asymmetry, CPU cost, or link renegotiation makes the path unstable or uncharacterized.
- **H1:** A repeatable USB4NET reference envelope is established and can normalize inference utilization.

## Preconditions and provenance

- EXP-001 passes; exact cable, ports, retimers, interface driver, MTU, offloads, IRQ affinity, and power profile are frozen.
- No management traffic shares the measured interface. Record interface and Thunderbolt/USB4 sysfs before and after each block.
- Install compatible `iperf3`; use the same version on both nodes.

## Factors, controls, and run order

- Direction: A→B, B→A, and simultaneous bidirectional.
- Parallel streams: 1, 4, and 8; TCP window left default and one declared tuned control if production uses it.
- Payload duration: 60 s after 10 s warm-up; five repetitions per cell, randomized.
- MTU/offload state: production value only, plus a separately labeled diagnostic comparison when needed.

## Procedure

1. Capture negotiated RX/TX speed and lane count, domain/device IDs, interface counters, routes, MTU, offloads, and CPU topology.
2. Run idle RTT sampling for five minutes, then the randomized unidirectional and bidirectional `iperf3` matrix.
3. Collect per-second goodput, retransmits, RTT/cwnd where available, CPU utilization, IRQ distribution, thermal/power, and link events.
4. Run a two-hour 70–80% reference-goodput stability load in each direction or a bidirectional equivalent.
5. Recapture sysfs/interface/journal state and calculate direction-specific reference goodput.

## Required measurements

- `USB4-NEGOTIATED`, `USB4-REF-GOODPUT`, `USB4-REF-IFRATE`, RTT distribution, jitter, retransmit rate, drops/errors, CPU-seconds/GiB, and renegotiation count.
- Direction asymmetry and run-to-run MAD/median; bidirectional fairness.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- No link/domain reconnect, speed/lane renegotiation, interface error/drop burst, or kernel warning during normal tests.
- Reference-goodput MAD/median ≤5%; unexplained A→B versus B→A median asymmetry >10% is a release investigation blocker.
- Retransmit rate remains ≤0.1% of sent segments in the clean lab reference and no sustained queue/drop condition is present.
- Candidate USB4 reference goodput is not >5% below its eligible baseline with confirmed confidence.
- The inference program uses a direction-specific same-layer reference: application payload over `USB4-REF-GOODPUT` or interface bytes over `USB4-REF-IFRATE`; it never mixes layers or uses nominal signaling rate.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- Traffic is observed on the measured interface from another process.
- Link sysfs state is unavailable before or after the block.
- CPU power profile or MTU/offloads change between cells.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

An `iperf3` envelope does not prove runtime scaling; it only bounds transport capacity and overhead.

## Research basis

[[SRC-010]](../references/Sources.md#src-010) [[SRC-011]](../references/Sources.md#src-011) [[SRC-012]](../references/Sources.md#src-012)
