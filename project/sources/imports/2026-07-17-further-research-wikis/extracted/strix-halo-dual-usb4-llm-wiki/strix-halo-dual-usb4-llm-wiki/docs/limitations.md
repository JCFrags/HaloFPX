---
title: Limitations and non-claims
status: scope boundary
---

# Limitations and non-claims

## No hardware measurements are embedded

This repository was constructed as a research and calibration model. It contains no measured USB4 bandwidth, latency, collective timing, Strix Halo compute timing, tokens/s, TTFT, ITL, acceptance rate, routing locality, or speedup from the target pair of systems.

The nominal 40 Gb/s per-port and arithmetic 80 Gb/s dual-port values are used only for protocol-free payload-time floors. They are not forecasts.

## Architecture fields are version-specific

Worked examples use the cited official configuration/repository entries and marketed parameter labels. A different checkpoint revision, architecture variant, rope/context configuration, quantization, vocabulary padding, or runtime tensor layout can alter memory and communication.

## Ideal weight sizes are lower bounds

`parameter_count × bits / 8` excludes quantization metadata, higher-precision tensors, padding, duplicate tensors, graph workspace, temporary buffers, allocator behavior, and the operating system. It is never a fit guarantee.

## First-order network equations omit measured second-order effects

The affine model \(T=\ell+V/B\) is useful only within calibrated message-size regions. Real behavior may include:

- protocol thresholds and congestion windows;
- extra copies or device synchronization;
- USB4 tunneling and flow-control interactions;
- CPU/runtime scheduling;
- asymmetric paths and shared controller resources;
- queueing, reordering, retransmission, and tails;
- full-duplex contention;
- thermal or power-state changes.

The benchmark protocol requires residual inspection and piecewise fits.

## Compute is not predicted from specifications

Graphics-core count, theoretical arithmetic rate, or memory data rate is not converted into model execution time. Kernel efficiency, quantization support, operator shapes, context, batch, cache behavior, and runtime implementation require measurement.

## TP formula scope

The TP volume model represents a Megatron-style two-rank transformer layout with two activation all-reduces per forward layer. Architectures/backends with different collective graphs, sequence/context parallelism, replicated KV groups, or a full logits gather need adjusted formulas. The p=2 collective phase count is deliberately a measured variable.

## Pipeline scope

The two-stage pipeline model assumes independent microbatches or sessions for overlap. It does not claim that consecutive tokens of one autoregressive sequence are independent. Queueing and service-time distributions can dominate the mean model.

## MoE routing is workload-dependent

The illustrative \(\rho=0.5\) rows show sensitivity only. Real remote-assignment fractions, expert imbalance, batching, and tails require router traces for the chosen prompts and model revision.

## Speculative exactness is protocol-dependent

Greedy verification can be checked by token equality. Generic exact stochastic speculative decoding may need proposal-distribution information and carefully separated RNG streams. The wiki does not assert that token IDs alone are sufficient, nor does it select one compressed probability protocol.

## KV portability is not assumed

KV layouts can depend on model revision, runtime, quantization, attention implementation, positional encoding, paging, and shard placement. Migration is a no-go without an exact export/import and transactional ownership implementation.

## Dual-link aggregation is not guaranteed

Two native ports do not prove independent end-to-end resources or application-level bonding. The topology may share internal resources, and operating-system networking may expose separate interfaces without striping one flow. `B_1+B_2` is gated by simultaneous tests.

## Security, licensing, and model terms remain external

The repository does not audit model licenses, export restrictions, privacy requirements, or security hardening for a deployment. Host-to-host endpoints and model artifacts must be governed separately.

## Inference-only scope

The formulas address forward inference. Training adds backward collectives, optimizer/gradient state, activation storage/recomputation, and different pipeline semantics.

## Decision lifetime

A GO decision applies only to the recorded systems, firmware, OS, drivers, runtime revision, model/checkpoint, quantization, cables/topology, workload distribution, and SLO. Material changes trigger remeasurement.
