---
title: Network cost model
status: symbolic model
---

# Network cost model

## Two paths, not one assumed fabric

The platform exposes two physical USB4 ports per node. Model them as two paths with independently measured properties:

\[
\mathcal L=\{(B_1,\ell_1),(B_2,\ell_2)\}.
\]

**SOURCED FACT.** AMD specifies each native port at 40 Gb/s. Linux can create one virtual Ethernet interface per Thunderbolt/USB4 port. [AMD](https://www.amd.com/en/products/processors/laptop/ryzen/ai-300-series/amd-ryzen-ai-max-plus-395.html), [Linux kernel](https://docs.kernel.org/admin-guide/thunderbolt.html).

**DECISION RULE.** Do not replace the two paths with \(B_\Sigma\) until a simultaneous test demonstrates that two transfers provide materially higher aggregate payload throughput than one transfer at the message sizes of the intended mode. Shared internal resources, transport serialization, software copies, or flow hashing may prevent scaling.

## Message model

For a one-way transfer of \(V\) bytes using \(n\) sequential fixed-cost phases:

\[
T_{msg}=n\ell+\frac{V}{B}+T_{copy}+T_{queue}+T_{runtime}.
\]

The compact equations elsewhere in the wiki write \(n\ell+V/B\). The omitted terms are not assumed zero; they are either included in fitted \(\ell,B\) for the actual application path or retained as separately measured runtime terms.

### Fitting \(B\) and \(\ell\)

For a fixed transport and direction, measure one-way or ping-pong time across geometrically spaced message sizes. Fit the affine region:

\[
T(V)\approx \ell+\frac{V}{B}.
\]

Do not use a single large-transfer `iperf3` result for decode collectives. Decode modes can be dominated by small-message fixed cost. Conversely, do not use tiny-message latency to estimate 128 MiB prefill transfers.

Keep at least two calibrated regions when behavior changes with size:

\[
(B_{small},\ell_{small}),\qquad(B_{bulk},\ell_{bulk}).
\]

## Validated striping

For a message divided into \(V_1+V_2=V\):

\[
T_{stripe}=\max\left(\ell_1+\frac{V_1}{B_1},\;\ell_2+\frac{V_2}{B_2}\right)+T_{reassembly}.
\]

With negligible reassembly and equal fixed cost, the ideal split is:

\[
V_1=V\frac{B_1}{B_1+B_2},\qquad
V_2=V\frac{B_2}{B_1+B_2}.
\]

For small messages, striping can be slower because it incurs two software paths and reassembly. The placement files therefore use one of four policies:

- `single_link` — one path for the modeled flow;
- `session_hash` — independent sessions select different links; no single-message aggregation;
- `validated_striping` — large tensors are split over two sockets/streams after the gate passes;
- `control_plus_bulk` — control/token feedback uses one path while bulk activations use one or two validated paths.

## Full duplex and volume accounting

State direction explicitly. For a p=2 collective, rank A and rank B can send concurrently. This wiki reports:

- **per-rank sent bytes** — useful for each rank's egress time;
- **per-rank received bytes** — equal in symmetric p=2 all-reduce;
- **aggregate bidirectional cut bytes** — sum of bytes sent in both directions.

Do not divide aggregate bidirectional bytes by one-direction bandwidth. Critical-path collective time uses per-rank directional volume and the actual full-duplex behavior.

## P=2 all-reduce

For an all-reduce tensor of \(S\) bytes, common p=2 implementations can have the same per-rank byte total but different transport phases:

- direct exchange and local reduction: one fixed-cost phase, per-rank sent bytes \(S\);
- ring reduce-scatter + all-gather: two fixed-cost phases, total per-rank sent bytes \(S\).

Therefore:

\[
T_{AR,2}(S)=m_{AR}\ell+\frac{S}{B},
\]

where \(m_{AR}\) is traced from the actual runtime. The repository does not select one algorithm on behalf of the implementation.

## Nominal payload floors

**CALCULATED LOWER BOUND.** One 40 Gb/s port corresponds to 5 GB/s decimal; two correspond arithmetically to 10 GB/s. Thus:

\[
T_{floor,1}=\frac{V}{5\times10^9},\qquad
T_{floor,2}=\frac{V}{10\times10^9}.
\]

These floors exclude all protocol overhead, copies, latency, queueing, reassembly, and contention. A measured value cannot legitimately be expected to beat them. A mode that fails even against the floor is a firm no-go for that objective; passing the floor proves nothing by itself.

## Control-plane recommendation

Keep session IDs, cancellation, heartbeats, token IDs, and sampler status on a reliable ordered control channel. Bulk tensors should carry explicit session, layer, microbatch, token-position, dtype, shape, and sequence-number headers. Do not let a stalled bulk path block cancellation or failure detection indefinitely.
