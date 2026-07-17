---
title: "Backpressure and flow control"
tags: ["backpressure", "flow-control", "quotas"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["RPC-06", "KV-02", "KV-08"]
related: ["Protocol-Overview", "Epochs-Retries-Cancellation", "Degraded-Mode-Behavior"]
---

# Backpressure and flow control

## Why transport flow control is insufficient

Streaming RPC flow control prevents an immediate receiver overrun, but HaloKV must also bound local disk writes, checksum/decompression CPU, object-store requests, pinned host memory, GPU staging, per-tenant cache occupancy, and checkpoint backlog. Application credits model these resources explicitly.

## Credit model

The receiver grants a transfer a monotonically numbered budget:

```text
CreditGrant(stream_id, grant_seq, max_additional_bytes,
            max_additional_pages, expires_at, receiver_watermark)
```

The sender may have at most the cumulative unreturned grant in flight. Credits are consumed by declared uncompressed bytes, not compressed wire bytes, preventing compression-ratio abuse. Every frame also has an absolute maximum size.

Credits do not survive epoch changes unless the reconnect plan reissues them. Duplicate grants are deduplicated by `grant_seq`; credit return is cumulative to avoid double accounting.

## Bounded resources

Maintain independent limits for:

- active checkpoint operations per session and per rank;
- queued checkpoint operations per tenant;
- in-flight wire bytes and uncompressed bytes;
- page count and manifest entry count;
- local temporary/orphan bytes;
- concurrent hash, compression, upload, and GPU-copy workers;
- inventory/Bloom-filter size and recovery-plan entries;
- retry rate and reconnect rate.

Admission reserves worst-case metadata and temporary space before snapshotting. A rank must not consume GPU memory for a transfer it cannot durably or promptly complete.

## Priority and deadlock avoidance

Reserve a small control-plane channel and worker pool for heartbeats, cancellation, credit grants, errors, and status queries. Data-plane saturation must not prevent the messages required to release data-plane resources.

Use a fixed acquisition order, for example:

```text
tenant quota -> operation slot -> local temp bytes -> transfer credits -> GPU staging
```

Never hold GPU staging while blocking indefinitely for disk or remote credits. Time-bounded reservations are released on cancellation, epoch loss, or connection loss.

## High- and low-water behavior

| Condition | Behavior |
|---|---|
| Below low watermark | grant normal credits; allow checkpoint admission |
| Between low/high | reduce grants; prefer incremental checkpoints and active recovery |
| Above high | stop new checkpoint admission; continue control, cancellation, and bounded draining |
| Hard limit / disk full | return `RESOURCE_EXHAUSTED`; abort uncommitted operation; preserve committed references |

Server responses include a coarse retry category or minimum delay, not sensitive global capacity details.

## Slow peer handling

A slow rank cannot make queues unbounded. The coordinator has one active checkpoint per session by default and a finite prepare deadline. If the slow rank misses it, the operation aborts; the fast rank’s prepared objects remain harmless orphans. Repeated slowness may move the session to `PERSISTENCE_LAGGED`, reduce checkpoint frequency, or fail admission according to durability policy.

## Backpressure and inference

Checkpoint backpressure should not silently block token production forever. Policy chooses among:

- **strict durability:** stop admitting or pause at a safe boundary when checkpoint lag exceeds limit;
- **latency-first:** continue inference, mark new state unprotected, and expose the larger replay window;
- **shed:** reject new sessions or cache persistence while preserving existing committed state.

The selected mode is observable and part of the response/service contract.
