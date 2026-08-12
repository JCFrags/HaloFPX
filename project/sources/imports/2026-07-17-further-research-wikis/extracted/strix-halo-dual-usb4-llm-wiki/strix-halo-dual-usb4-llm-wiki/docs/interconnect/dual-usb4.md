---
title: Dual USB4 topology
status: physical and transport model
---

# Dual USB4 topology

![Two-node dual-link topology](../../diagrams/svg/topology.svg)

## Physical model

The target topology has two systems, two model-execution ranks, and two independent host-to-host USB4 cables:

```text
node A / rank 0  -- USB4 path 1 --  node B / rank 1
node A / rank 0  -- USB4 path 2 --  node B / rank 1
```

AMD lists two native 40 Gb/s USB4 ports for Ryzen AI Max+ 395. [AMD specification](https://www.amd.com/en/products/processors/laptop/ryzen/ai-300-series/amd-ryzen-ai-max-plus-395.html). The arithmetic line-rate ceiling is therefore 40 Gb/s per path and 80 Gb/s across two paths. These are **nominal rates**, not effective payload measurements.

\[
40\ \mathrm{Gb/s}=5\times10^9\ \mathrm{B/s};\qquad
80\ \mathrm{Gb/s}=10\times10^9\ \mathrm{B/s}.
\]

Any payload time computed with those rates is an impossible-to-beat, protocol-free lower bound. It excludes encoding/tunneling overhead, headers, flow control, copies, serialization, software scheduling, contention, and latency.

## Host-to-host transport evidence

Linux documents two relevant USB4/Thunderbolt interdomain facilities:

- `thunderbolt-net`, which creates a virtual Ethernet interface for a host-to-host connection; the kernel documentation states that a virtual Ethernet interface is created for each Thunderbolt port;
- `thunderbolt-stream`, a direct high-speed host-to-host streaming interface.

See the [Linux kernel USB4 and Thunderbolt documentation](https://docs.kernel.org/admin-guide/thunderbolt.html). Windows documents USB4 interdomain connections that create a USB4NET network adapter between two PCs. See [Microsoft USB4 interdomain connections](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/usb4-interdomain-connections).

These facilities establish candidate transport paths. They do not establish application-level striping, collective support, zero-copy behavior, or throughput.

## Per-path variables

Characterize each path independently:

\[
(B_1,\ell_1,j_1,e_1),\qquad(B_2,\ell_2,j_2,e_2),
\]

where:

- \(B_j\): effective one-direction payload bandwidth for the relevant message-size region;
- \(\ell_j\): fitted fixed one-way message cost for that region;
- \(j_j\): latency-tail or jitter distribution, not just a mean;
- \(e_j\): retry/error behavior under sustained load.

For a serialized sequence of \(n\) messages with total payload \(V\) on path \(j\):

\[
T_j(n,V)=n\ell_j+\frac{V}{B_j}.
\]

This is a first-order affine model. Fit separate regions when small messages, bulk transfers, copies, or protocol thresholds exhibit different behavior.

## Full-duplex and directionality

Measure A→B, B→A, and simultaneous A↔B. Do not infer one direction from the other. A mode can have a small reverse control message yet still encounter contention or head-of-line blocking. Use:

\[
B_{j,A\to B},\;B_{j,B\to A},\;B_{j,duplex}
\]

when the measurements differ materially.

## Dual-link policies

The placement schema permits four policies:

| Policy | Meaning | Appropriate use |
|---|---|---|
| `single_link` | All model-path traffic uses one measured path | Small/latency-sensitive messages; fallback |
| `session_hash` | Whole sessions/flows are assigned to path 1 or 2 | Replicas, multiple pipeline sessions, independent RPC streams |
| `validated_striping` | One logical payload is partitioned and reassembled | Large boundary tensors or probability blocks after scaling/reordering tests |
| `control_plus_bulk` | Control messages use one path; bulk payload uses the other or validated stripes | Avoiding control-message head-of-line blocking |

No mode may substitute \(B_1+B_2\) for \(B\) solely because two cables are attached.

## Simultaneous-link gate

Measure each path alone, both with independent flows, and the planned striping code. Define:

\[
\eta_{link}=\frac{B_{both}}{B_1+B_2}.
\]

Choose the acceptance threshold before measuring. The gate must include p95/p99 latency, reorder-buffer occupancy, CPU/runtime cost, sustained stability, and directionality. A failure rejects aggregation, not necessarily the entire distributed mode.

## Message classes by execution mode

| Mode | Dominant A→B class | Dominant B→A class | Link sensitivity |
|---|---|---|---|
| Tensor parallel | Hidden-state collective payload | Same-sized collective payload | Very high message count, especially decode |
| Contiguous split | One hidden-state boundary | Token IDs / control | Bulk prefill; latency-sensitive decode feedback |
| Pipeline | Boundary microbatches | Token IDs / queue control | Throughput depends on service interval and queues |
| Expert service | Routed hidden states | Expert outputs | Two phases per active MoE layer |
| Remote speculation | Candidate IDs or probability data | Accepted length and token(s) | Usually RTT-sensitive; bandwidth-sensitive only for large probability protocols |
| Replicated decode | Request/session routing only | Independent outputs | No model-path synchronization |
| KV migration | Full KV state plus metadata | Commit acknowledgement | Large transactional transfer |

## Security and isolation

Host-to-host networking creates a trust boundary. Bind services only to intended interdomain interfaces, authenticate peers, constrain ports, and document whether traffic is encrypted. Encryption and authentication overhead must be included in measured \(B\) and \(\ell\). Never expose model RPC endpoints broadly as an accidental consequence of interface routing.
