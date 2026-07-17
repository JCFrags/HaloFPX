---
section_id: "58"
title: "Rank-local restore procedures and checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "fewtarius/CachyLLama"]
  software_versions: ["ROCmFPX a5605a72768c6562241b248e268e33dc92787394", "CachyLLama 6be745998f568e379ea197fcf827baec73ff9940"]
  hardware_revisions: ["two gfx1151 Strix Halo hosts; exact topology pending"]
related_sections: ["27", "52", "54", "57", "61", "63", "75"]
---

# Rank-local restore procedures and checks

## Safety and authorization boundary

Source audit and ordinary read-only restore comparison require no root access. Every deletion, truncation, corruption, process kill, link interruption, reboot, or other injected fault must instead use a separately created disposable cache/store and disposable service instance whose resolved paths, store UUID, ports, process IDs, cgroup, byte/inode ceiling, and cleanup command are recorded before execution. Refuse any target that resolves to the production cache, model store, repository/workspace, boot/root filesystem, or sole evidence copy. Preserve out-of-band recovery access and stop on unexpected target resolution, host instability, or evidence-path pressure. Physical, kernel, device, or cable faults require the approved Section 80 procedure and operator authorization; record required privilege and a cleanup/evidence receipt.

## Source-code audit

No root access is required. At the exact pins, inventory `llama_split_mode`, tensor split parsing, RPC device registration, sequence serialization and CachyLLama server state adapters. Preserve file/blob hashes and build manifest.

```bash
git -C ROCmFPX checkout a5605a72768c6562241b248e268e33dc92787394
git -C CachyLLama checkout 6be745998f568e379ea197fcf827baec73ff9940
rg -n "SPLIT_MODE|tensor_split|pipeline_parallel|state_seq|supports_rpc" ROCmFPX
rg -n "state_seq|dft_data|spec_data|kv_ssd" CachyLLama/tools/server CachyLLama/common
```

## Required two-host experiments

For replication, layer/pipeline, tensor and proposed MoE-aware plans:

1. Record exact plan manifest, rank ownership ranges, model/tokenizer/template/build hashes and local object inventories.
2. Generate a checkpoint from a known prefix. Reboot/restart both ranks, restore using independent local reads, replay a suffix and compare against full recomputation under deterministic sampling.
3. Trace NVMe and both USB4 links. Assert control-plane bytes stay below the configured ceiling and no state payload crosses the links.
4. In the disposable store only, delete/corrupt/truncate one rank component; change rank ID, device order, world size, split ratio, layer range, dtype/layout and plan epoch independently. Each must miss with a field-specific diagnostic.
5. In the disposable service deployment only, delay rank 1 before and after `READY`; replay stale readiness/nonces; kill a rank during staging, readiness, live commit and first suffix step. Verify no partial output/state is accepted.
6. Exercise fallback: compatible single-node checkpoint if one exists, otherwise recompute. Confirm the system never treats distributed rank files as a single-node state.
7. Compare parallel local restore with serialized restore and recomputation. Preserve p50/p95/p99 timing, bytes, thermals, page faults, NVMe queues and profiling overhead.

## Manifest checks

Each rank independently verifies canonical fingerprint, plan digest, checkpoint/generation, logical rank, owned ranges, component schemas, token range, file size and cryptographic digest. Coordinator verifies complete expected rank set, identical global manifest digest and current attempt nonce.

## Acceptance

- No mixed checkpoint generations or topologies.
- No live publication before all-ready.
- No state payload over USB4 in the normal path.
- All corruption becomes miss/recompute.
- Explicit timeouts and retry limits; no indefinite collective wait.
- Single-node fallback remains usable after rank failure.

No benchmark result exists yet; record experiments under `experiments/` before using **[MEASURED]**.
