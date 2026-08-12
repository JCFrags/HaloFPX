---
title: Dual-USB4 transport intake review
date: "2026-07-17"
status: proposed
type: intake-review
scope: strix-halo-dual-usb4-llm-wiki and dual-usb4-strix-halo-wiki
decision: selective-promotion-after-revision
risk: high
---

# Dual-USB4 transport intake review

## Executive verdict

**Decision: selectively promote after revision; do not merge either package wholesale.**

- `[VERIFIED]` Both preserved intake packages are useful research inputs. Their archive identities and extraction counts are recorded in the import receipt. The larger `strix-halo-dual-usb4-llm-wiki` package passed its packaged validator and 13 tests during this review; the extracted tree was restored from the preserved ZIP afterward because its validator rewrites validation artifacts. The `dual-usb4-strix-halo-wiki` package passed link and structured-data parsing checks; its full self-test is Linux-specific and its own report records environmental skips.
- `[VERIFIED]` USB4NET (`thunderbolt-net`) and MPTCP are stable Linux facilities. USB4STREAM is not in stable Linux 7.1.3: it first appears in the Linux 7.2 development line at commit `6db21d817b43f8ce5654ccc7aff80d40e4dba4ac`, and its interface is documented under `Documentation/ABI/testing`.
- `[MEASURED]` The canonical Wiki records historical USB4NET results near 20.8 Gb/s aggregate for several four-stream cells on Linux `7.1.3-1-cachyos`. Those observations are configuration- and date-scoped. They do not establish a universal link ceiling, two-link scaling, tail latency, controller independence, or GPU-to-GPU inference benefit.
- `[INFERENCE]` The packages add worthwhile topology checks, symbolic tensor-parallel cost equations, measurement worksheets, a proof ladder, and a useful warning that two llama.cpp `--rpc` endpoints are two remote devices rather than two paths to one remote device.
- `[RECOMMENDATION]` Reject the imported RPC runbook and systemd unit in their present form. They expose the RPC server on `0.0.0.0:50052` and then prescribe cable-pull tests without satisfying the canonical RPC provenance and vulnerability gate.
- `[RECOMMENDATION]` Keep USB4NET/TCP/MPTCP as the current project baseline. Treat USB4STREAM as a reversible, isolated experiment on an exact 7.2 candidate build until correctness, recovery, security, and matched performance evidence exists.

No canonical Wiki page was edited by this review.

## Authorities and version boundary

This review applies the repository instructions, the Agent Harness evidence flow (`sources -> claims -> Wiki -> implementation decisions`), the category manifests and linked decisions, and canonical Sections 20, 49-55, and 73-81. Imported research is evidence intake, not authority.

As checked on 2026-07-17:

| Subject | Primary evidence | Disposition |
|---|---|---|
| Linux releases | `[VERIFIED]` Kernel.org lists stable `7.1.3`, mainline `7.2-rc3`, and longterm `6.18.38`. | `7.1.3` is the current upstream stable release, but the project's `7.1.3-1-cachyos` record remains a historical environment pin, not a timeless deployment recommendation. |
| USB4NET | `[VERIFIED]` The upstream `thunderbolt-net` driver entered Linux at `e69b6c02b4c3b8d03be7136f90dd9551ad5a5a5e`; the current driver remains a virtual Ethernet interface over a Thunderbolt/USB4 connection. | Stable baseline transport. One netdev per port does not prove independent controllers, independent bandwidth domains, or additive goodput. |
| MPTCP | `[VERIFIED]` Linux supports MPTCP sockets from 5.6 onward. RFC 8684 defines a connection composed of one or more subflows with connection-level sequence and acknowledgement state. | Stable candidate for combining the two IP paths, but active multipath must be proven from socket and subflow state. Fallback to ordinary TCP is possible. |
| USB4STREAM | `[VERIFIED]` Introduced at Linux commit `6db21d817b43f8ce5654ccc7aff80d40e4dba4ac`; present in `7.2-rc3`, absent from `7.1.3`; Kconfig is `CONFIG_USB4_STREAM`, module `thunderbolt_stream`, with ConfigFS and `/dev/tbstreamX`. | Experimental candidate only. The testing ABI is not a production-stability promise and supplies neither application framing nor authenticated reliable delivery. |
| AMD platform capability | `[VERIFIED]` AMD lists two native USB4 40-Gb/s ports for Ryzen AI Max+ 395. | Product capability does not prove a specific OEM's connector wiring, controller independence, retimer path, tunnel allocation, or payload rate. |
| llama.cpp RPC security | `[VERIFIED]` GHSA-j8rj-fmpv-wcxw describes unauthenticated RPC remote code execution affecting builds through `b7991`; the source fix is `ba38f3becce7d1283585c73d796eb47d72bbbd30`. | RPC stays disabled until the deployed executable/library provenance, exposure boundary, and least-privilege controls are verified—not merely the source checkout. |

## Verdict matrix

| Dimension | `strix-halo-dual-usb4-llm-wiki` | `dual-usb4-strix-halo-wiki` | Canonical disposition |
|---|---|---|---|
| USB4NET | Mostly treats the links abstractly; offers host benchmark scripts but little exact driver provenance. | Strong topology/enumeration treatment, including per-port netdev behavior, same-UUID ICM replacement risk, and negotiated-speed versus goodput distinction. Some claims rely on moving upstream pages. | Promote the topology checklist only after exact source pins and canonical field names are added. Keep measured rates environment-scoped. |
| MPTCP | No substantive MPTCP treatment. | Correctly distinguishes one application stream from its subflows, warns about fallback, and requires subflow proof. It also notes that aggregation is not the arithmetic sum of link rates. | Selectively promote the proof procedure and fallback warning. Do not imply that enabling MPTCP creates two active subflows or guarantees aggregation/failover latency. |
| USB4STREAM | Refers to rolling documentation and lacks an exact introduction commit, kernel boundary, and runtime proof. | Good candidate-versus-stable matrix and character-device description. Still not sufficient to claim reliable, zero-copy, authenticated, or interoperable transport. | Use as corroborating intake only. Canonical Section 50 remains authoritative: exact commit, 7.2 development status, absent on historical 7.1.3 nodes, reversible probe. |
| Kernel applicability | Does not adequately separate a stable deployed kernel from a development candidate. | Correct date-scoped release split, but calls 7.1.3 a “recommended stable baseline” too broadly. | Record exact upstream version, distribution package, config, module build ID/hash, boot ID, and runtime module. Never turn a dated upstream status into permanent project policy. |
| Capacity and latency | Useful symbolic cost model and benchmark plan. One assumptions page calls 80 Gb/s a “calculated lower bound input,” although nominal link arithmetic is a ceiling; only its derived transfer time is a lower bound. | Correctly warns that negotiated speed is not goodput, but proposes heuristic `>=1.7x` aggregate and `>=80%` per-link retention thresholds without enough trials. | No dual-link scaling or latency claim is promoted. Use matched single/dual cells, paired randomized order, raw samples, exact population definitions, and canonical sample requirements. |
| Tensor parallelism | Strongest new contribution: explicit TP=2 ownership, collective count/bytes formulas, divisibility checks, break-even denominator gate, and fail-closed placement. | Mainly provides the important `--rpc` endpoint/path distinction; it is not a TP implementation design. | Promote formulas and ownership vocabulary only as `[INFERENCE]`/candidate planning material. Section 76 remains correct: runtime availability is not benefit; trace real collectives, phases, bytes, overlap, imbalance, and end-to-end tokens/s. |
| Security | Generic authentication/checksum advice; no mandatory per-record keyed authentication and no concrete RPC vulnerability/provenance gate. | Nuanced MPTCP security discussion, but imported runbook/unit expose wildcard RPC. Custom tools use unauthenticated framing; optional hash/CRC or optional AEAD is below the canonical minimum. | Mandatory canonical rule: authenticated integrity on every control and DATA record; CRC is diagnostic only. RPC disabled until deployed-artifact and exposure checks pass. |
| Failure behavior | A failure page says a striped message may “fail or resume,” leaving epoch ownership ambiguous. | Requeues a missing chunk on the surviving path in the same session. MPTCP reinjection semantics are otherwise described reasonably. | For HaloFPX wire-v1, either rail failure invalidates the entire epoch. No same-epoch migration. Retry only a whole idempotent upper-layer operation in a new authenticated epoch. MPTCP's internal reinjection does not supersede that application invariant. |
| Reproducibility | Good environment worksheet, benchmark matrix design, raw-log intent, and tested symbolic utility. The CSV summary is too thin and package validation omits complete command/time/tool-hash provenance. | Strong proof ladder and topology capture. Five repetitions and summary-oriented CSV are below canonical transport requirements. Full self-test is platform-specific. | Adapt useful fields into Sections 73/80. Require preserved raw samples, exact scripts/tools and hashes, environment manifest, paired schedule, metric population, quantiles/confidence, error counts, failure policy, and matched configurations. |

## Material contradictions and required remediation

### P0 — unsafe RPC activation and fault test

`dual-usb4-strix-halo-wiki/docs/18-runbook.md` and `configs/systemd/ggml-rpc-server-mptcp.service` start RPC on `0.0.0.0:50052`; the runbook then performs live cable-pull tests. This bypasses the canonical Section 51 security gate.

`[RECOMMENDATION]` Do not promote or execute this runbook or unit. A replacement must fail closed unless all of the following are captured: deployed binary hash; loaded-library provenance; exact source fix/commit relationship; listener address; firewall scope; service user and capabilities; filesystem/device access; peer restriction; and a negative test showing that an unapproved peer cannot reach the service.

### P1 — same-epoch chunk migration conflicts with wire-v1

`dual-usb4-strix-halo-wiki/docs/17-custom-runtime-design.md` requeues a missing chunk on the surviving path in the same session. `strix-halo-dual-usb4-llm-wiki/docs/feasibility/failure-modes.md` leaves open whether a striped message can resume.

`[VERIFIED]` Canonical Sections 52 and 53 require a global epoch reset when either rail fails. A stale or partial epoch must never be accepted. A whole idempotent upper operation may be retried only in a new authenticated epoch.

`[RECOMMENDATION]` Rewrite the imported failure taxonomy around explicit epoch, rank ownership, abort propagation, bounded teardown, stale-record rejection, and single-node fallback. Do not promote same-session migration.

### P1 — integrity model is below the canonical minimum

Both packages discuss checksums, hashes, or optional encryption, but neither consistently requires cryptographic integrity for every control and DATA record. The packaged copy tools implement no peer authentication or encryption.

`[VERIFIED]` RFC 8684's MP_JOIN/ADD_ADDR HMAC binds subflows and addresses to the original MPTCP connection. It does not encrypt or authenticate inference payloads, provide application identity, or eliminate TCP fallback. Linux's optional DSS checksum is not a substitute. Thunderbolt security levels concern PCIe tunneling/DMA authorization, not ThunderboltIP payload security.

`[RECOMMENDATION]` Retain the canonical per-record HMAC requirement. Treat CRC only as an early corruption diagnostic. Specify anti-replay epoch/sequence handling and key derivation/rotation before transport implementation.

### P1 — measurement thresholds and populations are insufficient

The second package's `>=1.7x` aggregate and `>=80%` per-link retention heuristics are not derived from canonical evidence and conflict with Section 20's suggested `>=90%` retention question and Section 55's ownership of final thresholds. Its benchmark matrix contains only five repetitions. The first package's one-row measurement template omits essential population, confidence, and failure-policy fields.

`[RECOMMENDATION]` Do not promote the thresholds. For each matched member, use an even fixed `N >= 20` paired schedule; use at least 100,000 observations for each latency population; publish all raw values, warm-up/exclusion policy, failed trials, quantiles, confidence procedure, and stopping rule. Keep exploratory results explicitly exploratory.

### P2 — nominal-rate and latency interpretation

The first package correctly derives a best-case transfer-time lower bound from a nominal-rate ceiling in one page, but another labels the 80-Gb/s two-link arithmetic itself a “lower bound.” This reverses the bound.

`[RECOMMENDATION]` Use four separate terms: advertised port rate, negotiated tunnel/link rate, transport payload goodput, and application-effective transfer rate. Report RTT directly. Use `RTT/2` only as a labeled symmetry assumption; synchronized one-way latency needs clock-error bounds. Never infer dual-link throughput from port labels.

### P2 — moving sources and example topology

Several imported Linux and llama.cpp references point to moving `master` pages; one source snapshot records a blob hash without the enclosing commit. The second package uses `10.44.1.0/30` for rail two, while the canonical historical capture used `10.44.0.4/30`.

`[RECOMMENDATION]` Reconcile each promoted source to an exact commit/tag and retain retrieval metadata. Mark address plans as examples unless they match the current measured deployment. Never overwrite a measured topology with a research-package example.

## Selective promotion candidates

| Candidate | Destination | Required before promotion |
|---|---|---|
| Same-UUID ICM replacement and coexistence stop gate | Sections 20 and 49 | Verify against an exact upstream source commit; express as a detection/stop condition; test on the target nodes without changing networking. |
| Per-port netdev discovery and topology capture fields | Sections 20, 49, 73, 80 | Map to controller/domain/BDF/retimer/cable/connector/IRQ/NUMA fields and preserve raw `ip`, `ethtool`, sysfs, Thunderbolt, and kernel-log output. |
| MPTCP proof ladder: meta-socket, endpoint config, two subflows, no unintended fallback | Sections 49, 53, 55 | Pin kernel/iproute2 versions and commands; capture `ss -M`, `ip mptcp`, counters, peer state, and fault transitions. Separate normal reinjection from application epoch semantics. |
| USB4STREAM stable/candidate compatibility matrix | Section 50 | Rebuild from exact Linux tags/commit; identify testing ABI; state no stable-production or interoperability guarantee. |
| Warning: two llama.cpp `--rpc` endpoints are two devices, not redundant routes to one device | Sections 51 and 76 | Verify against the exact deployed llama.cpp commit and CLI/runtime behavior. Keep it separate from TP semantics. |
| TP=2 ownership record, communication equations, divisibility checks, and positive-denominator break-even gate | Section 76 | Translate into canonical plan-manifest vocabulary; label assumptions; validate formulas/tests; then trace an actual target runtime before any performance recommendation. |
| Four policy names: `single_link`, `session_hash`, `validated_striping`, `control_plus_bulk` | Sections 52-55 | Define rank/epoch ownership, security, fallback, and failure behavior for each. Rename or reject any policy that implies unsafe same-epoch recovery. |
| Benchmark worksheet and proof-ladder fields | Sections 73, 75, 80, 81 | Add exact sample populations, raw-artifact paths/hashes, tool/source hashes, paired randomization, confidence method, errors, clock model, failure policy, and canonical minimum sample sizes. |
| Symbolic cost-model utility and tests | Experiments/tooling, then Section 76 | Preserve attribution/license; pin Python dependencies; add provenance output and boundary tests; compare predictions with raw traces before Wiki promotion. |

## Deferred or rejected material

- `[REJECTED]` Wildcard RPC runbook and systemd service in their current form.
- `[REJECTED]` Same-epoch chunk requeue or “resume” after a rail fault for HaloFPX wire-v1.
- `[REJECTED]` Hash-only, CRC-only, or optional-authentication transport framing.
- `[DEFERRED]` USB4STREAM as a baseline, production ABI, zero-copy path, or reliable transport. Upstream `read`/`write` paths copy through kernel pages, and the testing ABI/source does not define application-visible retransmission, authenticated framing, or exactly-once semantics.
- `[DEFERRED]` Imported `1.7x`, `80%`, or any other performance gate until a canonical experiment owns and justifies it.
- `[DEFERRED]` Any dual-link capacity, latency, or independence claim derived from two enumerated netdevs, two advertised 40-Gb/s ports, or negotiated link values.
- `[DEFERRED]` Any TP benefit claim derived only from the symbolic model or runtime feature availability.
- `[DEFERRED]` The package's alternative second-rail subnet as deployment truth.
- `[DEFERRED]` Package-generated summaries as performance evidence when raw observations and environment lineage are absent.

## Required experiment program

All experiments should be instantiated as canonical experiment cards before execution. Preserve commands, raw output, hashes, exact timing, boot IDs, environment, and deviations. No experiment below authorizes enabling RPC outside its security gate.

### E1 — physical and logical topology recapture

- Map connector -> cable identity/certification -> retimer -> USB4 domain/controller/BDF -> xdomain -> netdev -> IRQ/CPU/NUMA.
- Capture negotiated width/speed and tunnel events without treating them as payload goodput.
- Check whether both interfaces can coexist; stop on same-UUID replacement or topology ambiguity.
- Deliverable: immutable raw capture plus a human-readable topology map. Two netdevs remain insufficient evidence of independence.

### E2 — isolated and simultaneous USB4NET capacity

- Compare rail A only, rail B only, simultaneous independent flows, MPTCP one-subflow, and MPTCP two-subflow cells.
- Use an even fixed `N >= 20`, paired/randomized or interleaved order, identical CPU placement, MTU, stream count, direction, message size, duration, and thermal state.
- Record per-rail and aggregate payload goodput, CPU utilization, IRQ distribution, retransmits, tunnel/netdev errors, and thermal/power state.
- Report retention and scaling as distributions with confidence, not a single maximum.

### E3 — latency and tail behavior

- Sweep transport, message size, concurrency, direction, and single/dual-path modes.
- Preserve at least 100,000 observations per comparison member; report p50/p90/p95/p99/p99.9/max and loss/error populations.
- Label `RTT/2` as a symmetry estimate only. For one-way measurement, record clock synchronization method and uncertainty.
- Include reordering and retransmission effects; do not assume MPTCP or USB4STREAM lowers latency.

### E4 — MPTCP construction, fallback, and recovery proof

- Prove the application opened an MPTCP socket, the intended path-manager endpoints exist, and exactly the expected subflows are active.
- Detect initial TCP fallback and loss of a secondary subflow. Record scheduler/path-manager/sysctl values and recovery timing.
- Inject one-rail faults safely and distinguish kernel-level subflow reinjection from HaloFPX application epoch abort/retry.
- Check reconnect behavior; do not assume a removed subflow returns within the same meta-connection.

### E5 — end-to-end GPU-produced to peer-GPU-consumed transfer

- Measure from a producing GPU buffer through the complete transport/runtime path to a peer GPU buffer whose contents are validated.
- Compare USB4NET/TCP, USB4NET/MPTCP, and any later USB4STREAM candidate using the same tensor shapes, dtype, synchronization, and correctness oracle.
- Decompose copies, serialization, CPU scheduling, wire time, peer placement, and overlap. Host `iperf3` is a lower-layer diagnostic, not the decisive application result.

### E6 — isolated USB4STREAM candidate probe

- Boot an exact, reversible Linux `7.2-rc3` (or later explicitly selected candidate) environment; capture kernel/config/module/source hashes and keep the stable baseline bootable.
- Verify ConfigFS, stream device attachment, bidirectional correctness, copy/CPU behavior, poll/EOF/error semantics, unplug behavior, and coexistence with USB4NET.
- Compare against matched USB4NET cells. Do not expose inference or RPC traffic until application framing, authenticated integrity, acknowledgements, and epoch reset are implemented.

### E7 — wire-v1 epoch and corruption fault campaign

- Inject rail unplug, partial record, duplicate, reorder, stale epoch, corrupt payload, corrupt tag, delayed close, peer restart, and both-rail loss.
- Required result: invalid state causes rejection, miss, abort, or recomputation—never acceptance. Either-rail loss invalidates the epoch; single-node fallback is explicit and bounded.
- Preserve the trace needed to prove rank ownership, abort propagation, no stale completion, and whole-operation retry in a new authenticated epoch.

### E8 — RPC security negative gate

- Before any RPC load, verify deployed binary and loaded-library hashes against an approved fixed lineage.
- Confirm listener binding/firewall/peer scope, least-privilege service identity, and device/filesystem permissions.
- Prove the port is closed or unreachable when provenance or policy is missing. Exercise fault tests only after that negative gate passes.

### E9 — TP=2 model-to-runtime validation

- Use the first package's symbolic equations to pre-register predicted collective count, bytes per rank, and break-even assumptions.
- Trace the selected runtime to measure actual collective phases, tensor sizes, overlap, imbalance, synchronization, and effective communication time.
- Compare TP=2 with the single-node baseline on identical request populations, accuracy/correctness, context, batch, and generation settings. Reject TP if the positive-denominator condition or measured benefit fails.

## Further research backlog

1. `[OPEN]` Reconcile USB4STREAM's exact ABI and driver changes from introduction commit through the eventual stable release; identify any backport and its ABI/support consequences.
2. `[OPEN]` Pin and review `thunderbolt-net` framing, NAPI, ring/queue, MTU, error, and teardown behavior at the selected deployment kernel; test their relationship to tail latency.
3. `[OPEN]` Determine the target Linux MPTCP path manager, scheduler, `close_timeout`, `stale_loss_cnt`, and fallback settings, and how they interact with application epoch timeout/retry.
4. `[OPEN]` Establish whether the target hardware's two ports share controller, fabric, PCIe, memory, CPU, IRQ, or tunnel-allocation bottlenecks under simultaneous load.
5. `[OPEN]` Track upstream USB4STREAM application-visible error reporting, registered-buffer/mmap/splice possibilities, reliability framing, and non-Linux interoperability. No present source supports zero-copy or standardized interoperability.
6. `[OPEN]` Review the exact llama.cpp RPC commit selected for deployment, including transport abstraction, endpoint semantics, loaded dependency lineage, and all security changes after the cited fix.
7. `[OPEN]` Specify and, if worthwhile, model-check global epoch reset, anti-replay, rank ownership, abort, reconnection, and whole-operation retry invariants.
8. `[OPEN]` Benchmark authenticated framing choices under matched correctness guarantees; include CPU cost, key lifecycle, record size, batching, and tail behavior.
9. `[OPEN]` Identify the actual TP implementation/collective path for the chosen runtime and model; verify KV/head divisibility, activation ownership, tensor layouts, and fallback rather than extrapolating from a generic Megatron-style model.
10. `[OPEN]` Define a canonical decision threshold for simultaneous-link retention and application benefit only after exploratory distributions are available; Section 55 should own that decision.

## Promotion acceptance gate

An item from either intake package may be promoted only when:

1. its source is pinned to an exact primary source version/commit or explicitly labeled secondary evidence;
2. imported wording is reconciled with the canonical claim labels and current decisions;
3. environment-specific observations retain hardware, kernel, tool, configuration, raw artifact, and time scope;
4. security and failure behavior satisfy Sections 51-53 and 80, including RPC provenance and authenticated epoch semantics;
5. performance claims satisfy Sections 55, 73, 75, and 81, including matched populations, raw data, sample minimums, uncertainty, and failure accounting;
6. TP claims satisfy Section 76 and are validated against an actual runtime trace;
7. address plans, thresholds, and procedures are not silently substituted for measured canonical state;
8. the original archives, licenses, hashes, and import receipt remain preserved.

Until those gates pass, the packages remain governed intake evidence and experiment-design material—not verified Wiki truth or executable operational procedure.

## Evidence consulted

### Preserved intake

- `sources/imports/2026-07-17-further-research-wikis/import-receipt.md`
- `sources/imports/2026-07-17-further-research-wikis/extracted/strix-halo-dual-usb4-llm-wiki/strix-halo-dual-usb4-llm-wiki/`
- `sources/imports/2026-07-17-further-research-wikis/extracted/dual-usb4-strix-halo-wiki/dual-usb4-strix-halo-wiki/`

### Canonical project authority

- `wiki/HaloFPX_Wiki/04_Hardware_and_OS_Platform/20_Dual_USB4_Port_Topology_Controller_Independence_and_IOMMU_Groups/`
- `wiki/HaloFPX_Wiki/08_Fabric_and_Transport/49_*/` through `55_*/`
- `wiki/HaloFPX_Wiki/11_Verification_and_Performance/73_*/` through `81_*/`
- `references/agent-harness.md` and the routed Agent Harness architecture/review authority.

### Primary external sources

- [Linux Kernel Archives](https://www.kernel.org/)
- [Linux Thunderbolt/USB4 administration guide](https://www.kernel.org/doc/html/latest/admin-guide/thunderbolt.html)
- [Initial upstream thunderbolt-net commit](https://github.com/torvalds/linux/commit/e69b6c02b4c3b8d03be7136f90dd9551ad5a5a5e)
- [Linux MPTCP documentation](https://docs.kernel.org/networking/mptcp.html)
- [Linux MPTCP sysctls](https://docs.kernel.org/networking/mptcp-sysctl.html)
- [RFC 8684: Multipath TCP](https://www.rfc-editor.org/rfc/rfc8684.html)
- [USB4STREAM introduction commit](https://github.com/torvalds/linux/commit/6db21d817b43f8ce5654ccc7aff80d40e4dba4ac)
- [USB4STREAM testing ABI at Linux 7.2-rc3](https://github.com/torvalds/linux/blob/v7.2-rc3/Documentation/ABI/testing/configfs-thunderbolt_stream)
- [AMD Ryzen AI Max+ 395 product specification](https://www.amd.com/en/products/processors/laptop/ryzen/ai-300-series/amd-ryzen-ai-max-plus-395.html)
- [llama.cpp RPC advisory GHSA-j8rj-fmpv-wcxw](https://github.com/ggml-org/llama.cpp/security/advisories/GHSA-j8rj-fmpv-wcxw)
- [llama.cpp source fix `ba38f3becce7d1283585c73d796eb47d72bbbd30`](https://github.com/ggml-org/llama.cpp/commit/ba38f3becce7d1283585c73d796eb47d72bbbd30)
