# Authority and target context

Read this file whenever planning or interpreting qualification work. Refresh volatile target facts before execution; these are scoped to the 2026-07-17 capture unless a newer evidence bundle supersedes them.

## Evidence and decision order

1. Active project `AGENTS.md`, `PROJECT_GOAL.md`, and explicit user authorization.
2. Exact source objects/commits, built artifact hashes, and retained machine evidence.
3. Canonical Wiki pages and their `section.yaml` applicability/status.
4. Approved decisions and experiment cards.
5. Candidate intake Wikis and reviews.
6. Recommendations in plans or this skill.

Use the Agent Harness route `sources -> Wiki -> knowledge -> candidate procedure -> validated skill`. A candidate procedure cannot approve itself. Preserve raw evidence and keep imported research separate from trusted synthesis.

The current implementation plan is `reviews/plans/2026-07-17__rocmfpx-llama-ai-fork-plan__draft__v03.md` with its v03 review. It authorizes no target mutation. Its stated execution boundary is local/read-only Phase 0A only until the listed governance, pin, provenance, license, and machine gates close.

## Canonical experiment sequence

Use Wiki Section 84 and its schema-valid draft cards as the experiment namespace:

1. `HLX-EXP-20260717-841`: identity, topology, clock admission.
2. `-842`: measurement system and evidence pipeline.
3. `-843`: matched single-node backend/model baselines.
4. `-844`: dual-link fabric and GPU-to-peer-GPU path.
5. `-845`: buffer visibility, copy, graph, synchronization.
6. `-846`: HaloKV correctness/durability/performance.
7. `-847`: cache-off then optionally cache-integrated distributed matrix.
8. `-848`: service envelope and soak.
9. `-849`: separately authorized fault/security/recovery/rollback.
10. `-850`: holdout and independent reproduction.

The checked-in cards are drafts with unresolved fields. Structural validation does not authorize execution.

## Measured target snapshot

- Both nodes: Nimo MME3L, Ryzen AI MAX+ 395, Radeon 8060S/gfx1151 40 CU, about 124 GiB usable memory, one NUMA node, CachyOS kernel `7.1.3-1-cachyos`, ROCm 7.2.4-family packages, Mesa 26.1.4, Crucial P310 1 TB NVMe, and 32 GiB swap.
- The package, swap, boot, and rollback-kernel tuples are not fully matched. Freeze them deliberately before matched claims.
- nimo-1 is the current private RPC worker. It had about 43 GiB free and about 112 GiB of RPC model-tensor transfer cache.
- nimo-2 is the current coordinator/LAN API owner. It had about 318 GiB free.
- The current deployed runtime is `charlie12345/rocmfp4-llama@4860505e...`, not the planned ROCmFPX integration fork. Preserve its executable hashes and configuration as rollback/baseline evidence.
- The running GGUF is about 121.86 GB and the API reports about 228.7B parameters. This proves that artifact loaded in that configuration, not that a 200-230 GB stored artifact will fit.
- Dual USB4NET uses `10.44.0.0/30` and `10.44.0.4/30`, MTU 9000, with one MPTCP connection and two observed subflows.
- Interface/domain mapping crosses between hosts. Bind by address and sysfs ancestry, not interface ordinal.
- The running 7.1.3 kernel exposes USB4 and USB4NET but not `thunderbolt_stream` or `/dev/tbstream*`.

Consult these project sources before execution:

- `sources/measurements/2026-07-17-strix-halo-live-inventory/`
- `knowledge/dual-node-transport-and-capacity-constraints.md`
- `reviews/intake/2026-07-17__dual-usb4-transport__review__v01.md`
- `reviews/intake/2026-07-17__dual-node-large-model__review__v01.md`
- Wiki Sections 18-21, 50, 55, 73, 75, and 84.

## Interpretation boundaries

- Package presence is not workload correctness.
- Healthy HTTP is readiness only.
- A Git commit plus executable digest identifies an artifact but does not prove reproducible build inputs.
- RSS, cgroup memory, GTT, and `MemAvailable` are different measures.
- Requested `--tensor-split` is not realized placement.
- Five-packet ICMP results prove reachability only.
- Interface signaling/negotiation is not application goodput.
- The existing RPC `--cache` stores model tensors; it is not attention KV/session/prefix persistence and does not satisfy HaloKV integrity rules.
