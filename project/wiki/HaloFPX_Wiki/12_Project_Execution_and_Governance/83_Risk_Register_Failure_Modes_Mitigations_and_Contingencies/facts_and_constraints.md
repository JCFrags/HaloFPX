---
section_id: "83"
title: "Risk Register Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories:
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
  software_versions: ["research snapshot 2026-07-17"]
  hardware_revisions: ["dual Strix Halo; exact BOM open"]
related_sections: ["11", "13", "20", "21", "22", "23", "31", "48", "63", "65", "71", "78", "79", "80", "81"]
---

# Facts, constraints, and scored register

## Scoring method

**[RECOMMENDATION]** Score each axis from 1 (least) to 5 (greatest): likelihood `L`; impact `I`; and detectability difficulty `D`, where 5 means likely to escape before user or data impact. Gross priority is `L x I x D`: critical `60-125`, high `40-59`, medium `20-39`, low `<20`. Score the failure before project-specific controls. A residual target is a gate, not a claim that planned controls already work.

**[RECOMMENDATION]** Confidence is `H` when the mechanism and local applicability are both evidenced, `M` when one is evidenced, and `L` when both depend heavily on assumptions. Control state is `none`, `planned`, `partly-evidenced`, or `verified-on-target`. No register item is currently `verified-on-target` as a complete risk control.

## Source-backed constraints

- **[VERIFIED]** AMD specifies Ryzen AI Max+ PRO 395 with two native 40-Gbps USB4 ports, 128 GB maximum LPDDR5x memory, 45-120 W configurable TDP, and 100 C Tjmax [S83-13]. OEM topology, cooling, firmware, and sustained application limits remain separate evidence needs.
- **[MEASURED]** Historical project receipts show two USB4 domains and two traffic-carrying rails; they do not establish controller/root independence or linear aggregation [S83-03].
- **[VERIFIED]** Linux documents one Thunderbolt domain per host controller as typical, shared tunneled bandwidth, explicit low/insufficient-bandwidth events, and USB4NET/USB4STREAM facilities [S83-16]. The USB-IF specification defines dynamic link sharing, not an application throughput guarantee [S83-17].
- **[VERIFIED]** AMD recognizes `gfx1151` but current compatibility and limitations documents narrow supported tuples and record Ryzen AI Max 300-series LLM crash/performance caveats [S83-06, S83-14, S83-15]. Recognition is not ROCmFPX qualification.
- **[VERIFIED]** Current code contains custom HIP and Vulkan paths, but no HaloFPX measurement establishes correctness parity or the winning backend [S83-02, S83-08].
- **[MEASURED]** The 2026-07-17 live inventory records both installed drives as Crucial P310 `CT1000P310SSD8` devices with firmware `VACR001`, Btrfs filesystem state, capacity/headroom, and baseline SMART counters; Section 21 owns the scoped evidence and limitations [S83-04]. **[OPEN]** Exact vendor TBW/DWPD and warranty revision, negotiated PCIe/queue behavior, device/filesystem write amplification under the intended cache workload, volatile-write-cache/PLP behavior, and application-level power-loss recovery remain unproven [S83-11].
- **[OPEN]** Exact sustained clocks, package/skin temperatures, fan policy, throttling thresholds, and simultaneous compute/fabric/storage behavior are not recorded [S83-05].

## Lane-aware ROCm vocabulary

These labels are intentionally non-interchangeable. The Section 23 compatibility evidence and [Section 85 freshness ledger](../85_Internet_Research_Backlog_Upstream_Watch_and_Knowledge_Freshness/) remain the detailed authorities; this table prevents the risk register from treating umbrella versions as one ordered lane.

| Lane term | Exact version/scope | Evidence state | Permitted use in Section 83 | Not established |
|---|---|---|---|---|
| Ryzen vendor-supported control | ROCm `7.2.1`; Ryzen AI Max+ 395 / `gfx1151`; Ubuntu 24.04.4 and the documented framework/kernel constraints | **[VERIFIED]** vendor support boundary [S83-06, S83-14] | Correctness and rollback control candidate for R83-003/M83-02 | ROCmFPX correctness, USB4STREAM compatibility, target installation, or performance |
| Frozen research baseline | ROCm `7.2.3` tag `14f8138863403a26e0caef6671cfab9b09aa636e`, as recorded by Section 85 | **[VERIFIED]** immutable research/upstream-watch reference in Section 85 | Source comparison, RCCL/component research, and freshness tracking | Vendor-supported Ryzen control status, installed state, or HaloFPX qualification |
| Broader Core SDK candidate | ROCm Core SDK/TheRock `7.14.0` tag `830cc1b5e90d7da1b07e39113d7a5c95f3e687a1` | **[VERIFIED]** broader official matrix/release exists; **[OPEN]** project qualification [S83-06, S83-14] | Separate candidate lane for M83-02/M83-03 | Automatic succession from `7.2.x`, package/component parity, or target correctness |
| Installed two-node tuple | Kernel/config, amdgpu/KFD ancestry, firmware hashes, ROCm/HIP/HSA/RCCL packages, Mesa/RADV, and build identity on both nodes | **[OPEN]** | Must be captured by M83-02 before any installed/qualified claim | Any exact installed version, cross-node match, rollback safety, or supported combined tuple |

**[RECOMMENDATION]** A risk event or experiment record must name the lane term and exact component tuple. Numeric comparison between `7.2.3` and `7.14.0` is not a promotion rule, and “baseline,” “supported,” “installed,” and “qualified” must not be used as synonyms.

## Living risk register: score and accountability

`R83-*` is the immutable local key for this table. **[OPEN]** Section 03 currently defines no canonical risk namespace. Until a naming ADR approves one, external records must link the Section 83 path plus `R83-*`; they must not mint or infer `HLX-RISK-*` aliases.

Every score and owner assignment below is **[RECOMMENDATION]** and provisional. Evidence references support the failure mechanism or current uncertainty, not the numeric score.

| ID | Risk / failure mode | L | I | D | Gross / tier | Confidence | Control state | Accountable owner | Evidence |
|---|---|---:|---:|---:|---:|---|---|---|---|
| R83-001 | USB4 label/negotiation is treated as usable sustained payload capacity | 4 | 4 | 3 | 48 high | H | partly-evidenced | Fabric lead | S83-03, S83-16, S83-17 |
| R83-002 | Two ports share a controller, PCIe root, clocks, IRQs, or thermal budget, defeating independence | 3 | 5 | 4 | 60 critical | M | partly-evidenced | Platform lead | S83-03, S83-13, S83-16 |
| R83-003 | Kernel/amdgpu/firmware/ROCm/Mesa maturity causes crash, wrong result, or regression on `gfx1151` | 4 | 5 | 3 | 60 critical | H | planned | Software-platform lead | S83-06, S83-14, S83-15 |
| R83-004 | HIP or Vulkan backend misses the latency/throughput target or regresses by shape | 4 | 4 | 3 | 48 high | M | planned | Backend lead | S83-02, S83-08 |
| R83-005 | Independent-root ROCmFPX and divergent CachyLLama make drift/conflict/provenance unmanageable | 5 | 4 | 3 | 60 critical | H | partly-evidenced | Integration maintainer | S83-01, S83-02 |
| R83-006 | Conversion, quantization, custom kernels, MTP, recurrent, cache, or distributed state silently changes model output | 4 | 5 | 4 | 80 critical | H | planned | Correctness lead | S83-02, S83-07, S83-09 |
| R83-007 | Unified-memory pressure, fragmentation, pinned buffers, cache, or concurrency triggers OOM/livelock | 4 | 5 | 2 | 40 high | M | planned | Runtime lead | S83-09, S83-13 |
| R83-008 | HaloKV write amplification or retention exhausts SSD endurance/capacity unexpectedly | 3 | 4 | 4 | 48 high | M | planned | Storage/cache lead | S83-04, S83-11, S83-21 |
| R83-009 | Sustained compute plus USB4 and NVMe causes throttling, instability, or unsafe surface thermals | 4 | 4 | 3 | 48 high | M | planned | Platform lead | S83-05, S83-13 |
| R83-010 | Required out-of-tree/backported kernel patches create maintenance, security, or rollback failure | 4 | 4 | 3 | 48 high | M | planned | Kernel maintainer | S83-06, S83-16 |
| R83-011 | Critical knowledge is concentrated in one person/agent run; review and recovery capacity are absent | 4 | 4 | 4 | 64 critical | M | planned | Project owner | S83-22 |
| R83-012 | Research dependencies and unknown hardware/runtime behavior make the planned schedule non-credible | 5 | 4 | 3 | 60 critical | H | planned | Program owner | S83-03 through S83-12 |
| R83-013 | Unauthenticated/insecure RPC, weak peer identity, secrets, or shared cache exposes code/data | 4 | 5 | 4 | 80 critical | H | partly-evidenced | Security owner | S83-12, S83-18, S83-19, S83-20 |
| R83-014 | Upstream model/backend/API changes invalidate patches, tests, formats, or cache ABI | 5 | 4 | 3 | 60 critical | H | planned | Upstream steward | S83-01, S83-02 |
| R83-015 | Torn, corrupt, stale, or cross-incompatible cache state is accepted as valid | 4 | 5 | 4 | 80 critical | H | planned | Storage/cache lead | S83-10, S83-12 |
| R83-016 | Cable/rank/coordinator failure hangs work, duplicates output, or cannot degrade to one node | 4 | 5 | 3 | 60 critical | H | planned | Distributed-runtime lead | S83-09 |
| R83-017 | Unpinned dependencies, model artifacts, generated assets, or licenses prevent reproducible/legal release | 3 | 5 | 4 | 60 critical | M | planned | Release owner | S83-01, S83-22 |
| R83-018 | Missing/ambiguous telemetry reports readiness while no work executed or masks partial failure | 3 | 4 | 4 | 48 high | M | planned | Operations owner | S83-09, S83-22 |

## Living risk register: control, trigger, and fallback

| ID | Mitigation / prevention | Detection or escalation trigger | Fallback / contingency | Residual target |
|---|---|---|---|---:|
| R83-001 | Matched one-link/two-link payload, latency, loss, CPU, and application tests; pin cables/ports/config | Negotiated lanes change; p95 or goodput misses transport gate | USB4NET on one qualified rail; replication instead of bandwidth-coupled mode | <=24 |
| R83-002 | Map domain/NHI/PCIe/ACPI/IRQ/retimer/cable identities; paired saturation design | Shared ancestor/resource or dual-link gain below preregistered threshold | Treat ports as one failure/capacity domain; use failover only | <=24 |
| R83-003 | Supported tuple matrix, exact packages/firmware, smoke/correctness/soak, known-good image | KFD fault, GPU reset, compile failure, wrong-result, vendor caveat matches workload | Roll back full tuple; single-node known-good lane | <=24 |
| R83-004 | Shape-complete matched HIP/Vulkan benchmark and correctness gates | Any required cell slower than baseline budget or wrong | Per-plan backend selection; CPU fallback only for explicitly viable ops | <=24 |
| R83-005 | Frozen vendor bases, provenance map, small patch series, scheduled sync rehearsal | Unknown-origin diff; sync exceeds effort budget; security fix cannot port | Hold baseline/security-only backport; drop nonessential donor feature | <=24 |
| R83-006 | Golden logits/tokens, task-quality suite, conversion hashes, cache/restore/distributed metamorphic tests | Tolerance/quality regression, unsupported op, state replay mismatch | Known-good model/quant/backend; disable feature or reject release | <=20 |
| R83-007 | Admission budget with headroom; memory telemetry; bounded queues/cache; OOM fault tests | Headroom below gate, allocation failure, swap storm, OOM kill | Reduce context/concurrency/offload/cache; unload and restart cleanly | <=20 |
| R83-008 | Inventory TBW; quota/GC; host/device write counters; write-amplification benchmark | Free-space reserve or projected life below policy; SMART warning/error | Read-only/disabled cache; move cache to qualified replaceable SSD | <=24 |
| R83-009 | Sensor calibration, power cap, fan policy, sustained worst-case soak | Throttle/reset/error or temperature/power exceeds product gate | Lower cTDP/concurrency/clocks; improve cooling; single-node duty cycle | <=24 |
| R83-010 | Prefer upstream/distribution kernel; minimal patch manifest; CI and boot rollback entry | Patch fails rebase/build/boot/security review or lacks recovery image | Revert to supported kernel and USB4NET; defer custom transport | <=24 |
| R83-011 | Two-reviewer rule, runbooks, decision/evidence ledgers, restore drill, bounded ownership | Sole reviewer/owner, undocumented manual step, no backup during gate | Freeze scope; defer milestone; contract/reassign owner | <=24 |
| R83-012 | Evidence-based critical path with ranges and explicit research spikes | Gate slips twice, critical risk unowned, dependency date unknown | Rebaseline scope/date; ship single-node/minimal-cache increment first | <=24 |
| R83-013 | Loopback client default, authenticated encrypted peer protocol, least privilege, sandboxing, firewall | Unexpected listener, unauthenticated command, secret leak, advisory applicability | Isolate network; disable RPC/cache sharing; rotate credentials; roll back | <=20 |
| R83-014 | Pinned commits, upstream watch, compatibility tests, scheduled batch updates | Security/correctness advisory or patch-stack conflict beyond budget | Security-only cherry-pick; hold baseline; remove incompatible feature | <=24 |
| R83-015 | Strong content digest, compatibility fingerprint, atomic manifest, quarantine, corruption tests | Digest/schema/topology mismatch, short read, impossible state | Miss/recompute; quarantine evidence; restore last valid generation | <=20 |
| R83-016 | Timeouts, fencing/incarnations, idempotent output commit, fault matrix, single-node plan | Lost heartbeat/rail, collective timeout, stale rank, duplicate commit | Abort/retry from durable boundary or restart supported single-node mode | <=20 |
| R83-017 | Lockfile/SBOM/license/source bundles, hashes/signatures, offline rebuild | Unresolved license, missing object, hash mismatch, network needed to build | Quarantine artifact; restore prior signed bundle; block release | <=20 |
| R83-018 | Structured end-to-end request/worker/transport/cache/compute/result IDs and invariants | Success with zero executed work; trace gap; conflicting health signals | Fail closed, surface degraded state, retain raw evidence, restart safely | <=20 |

## Score-change rule

**[RECOMMENDATION]** Change a score only with a dated evidence note. Local measurements may alter likelihood/detectability for the tested tuple; they do not erase impact. Close a risk only when the failure is impossible by scope change or its residual risk is explicitly accepted. Superseded entries remain in history.
