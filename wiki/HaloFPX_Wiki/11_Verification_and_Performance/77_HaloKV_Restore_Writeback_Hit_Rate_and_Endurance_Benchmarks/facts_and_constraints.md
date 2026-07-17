---
section_id: "77"
title: "HaloKV Benchmark Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: ["fio 3.41 documentation", "NVMe Base 2.3"]
  hardware_revisions: ["exact NVMe devices pending inventory"]
related_sections: ["14", "21", "56", "57", "58", "63", "65", "73"]
---

# Facts and Constraints

## Donor behavior

**[VERIFIED]** CachyLLama commit `6be7459` stores cache format v3 in `index.bin` and checkpoint files containing target, optional draft/MTP, speculative blobs, compatibility hash, and up to 4,096 prefix tokens. Hot/warm entries are RAM-resident and cold entries SSD-only. [S77-001]

**[VERIFIED]** Donor writes truncate final paths directly and have no payload checksum or temp-file-plus-rename transaction. `fsync` is enabled by default but can be disabled; compatibility identity is narrower than exact model/tokenizer/template/runtime/backend/state ABI identity. [S77-001][S77-002]

**[INFERENCE]** Donor benchmark success cannot establish HaloKV crash safety, distributed consistency, or silent-corruption rejection. The proposed benchmark must include negative outcomes and recomputation.

## Hit vocabulary

**[RECOMMENDATION]** Record mutually exclusive lookup outcomes:

| Outcome | Definition |
|---|---|
| exact valid hit | full requested prefix and state validate |
| partial valid hit | validated prefix shorter than requested; remainder recomputed |
| system-prefix hit | validated system boundary state reused under approved scope |
| continuation hit | validated same-conversation checkpoint reused |
| incompatible miss | candidate rejected by identity/version/topology |
| corrupt miss | candidate fails bounds, digest, structure, or semantic validation |
| absent miss | no candidate |
| operational failure | I/O, permission, timeout, disk-full, or rank-coordination failure |

Report request hit rate, token-weighted hit rate, bytes restored, tokens/prefill time avoided, false-positive acceptance count, lookup candidates scanned, and evictions. A file found is not a hit.

## Latency and distributed critical path

**[RECOMMENDATION]** Split restore into lookup, index traversal, queue wait, storage read, decode/decompress/map, integrity/compatibility validation, GPU/state installation, cross-rank ready barrier, and residual prefill. For rank-local restore report each rank and `max(rank_ready)`, because the slowest required rank bounds distributed readiness.

## Storage measurement

**[VERIFIED]** fio provides direct/buffered and io_uring engines, queue-depth controls, verification, steady-state criteria, JSON/JSON+ output, latency bins, and synchronization controls. Buffered-write errors can be delayed unless sync/direct behavior is used. [S77-005]

**[VERIFIED]** NVMe SMART Data Units Written are host-written 512-byte units reported in thousands and rounded up; Percentage Used is a vendor-specific estimate of life consumed and can exceed 100. SMART also exposes unsafe shutdowns, media/data-integrity errors, temperature, and controller busy time. [S77-006][S77-007]

**[INFERENCE]** SMART write deltas are coarse, device-wide host-write observations. They include unrelated workloads and do not directly equal NAND writes. Isolate the device/workload and label `host write amplification = device host-write delta / logical HaloKV bytes committed`; do not call it NAND amplification without vendor telemetry.

## Durability semantics

**[VERIFIED]** POSIX `fsync()` requests transfer of file data to the associated storage device and waits for completion/error, while exact transfer nature is implementation-defined. POSIX `rename()` atomically changes directory naming semantics, but durable transactions also require a tested protocol and filesystem/device assumptions. [S77-003][S77-004]

No source establishes the exact HaloFPX SSD endurance or power-loss behavior.
