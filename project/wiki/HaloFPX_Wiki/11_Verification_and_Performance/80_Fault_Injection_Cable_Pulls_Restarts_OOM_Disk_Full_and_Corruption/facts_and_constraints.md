---
section_id: "80"
title: "Fault Injection Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-08-12"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: ["Linux fault-injection documentation accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["54", "56", "62", "65", "68", "76", "77", "78", "79", "81"]
---

# Facts and constraints

- **[VERIFIED]** Linux fault-injection facilities include allocation, block-I/O, NVMe, and function-level failure controls with probability, interval, count, task filters, and fail-at-N semantics [S80-01].
- **[VERIFIED]** `dm-flakey` can create a device-mapper target that periodically becomes unavailable or exhibits controlled I/O error/corruption behavior [S80-02].
- **[VERIFIED]** Cgroup v2 `memory.max` provides a scoped memory hard limit and can invoke OOM handling within the cgroup [S80-03].
- **[VERIFIED]** Linux documents USB4/Thunderbolt topology and device control, but actual dual-rail retrain and link-state observability must be established on HaloFPX [S80-04].
- **[VERIFIED]** Pinned ROCmFPX includes state-restore, prompt-cache, and regression mechanisms [S80-05]. Their existence does not prove corrupt-state rejection on HaloFPX.
- **[RECOMMENDATION]** Kernel fault injection and device-mapper tests require a loopback/sacrificial target and exact preflight verification; never target the workspace, model store, evidence store, boot disk, or production cache.
- **[RECOMMENDATION]** Disk-full testing must use a quota, loopback filesystem, or dedicated disposable mount. Do not fill the host filesystem.
- **[RECOMMENDATION]** Cable pulls and GPU resets require out-of-band management/console access and an operator stop condition.
- **[RECOMMENDATION]** Inject one fault at a time before testing combinations. Preserve the clean baseline and fault timeline.
- **[RECOMMENDATION]** Recovery is incomplete until leases, ranks, slots, KV/cache entries, temporary files, and partial writes are reconciled.
- **[RECOMMENDATION]** A retry may occur only at an idempotent, identified boundary. Duplicate output or tool execution is not acceptable.
- **[MEASURED]** An accidental 2026-08-12 nimo-2 global-OOM event killed the production RPC worker while it owned about 114 GiB `gpu_active` HMM state. The next real coordinator request exposed stale RPC state and caused the coordinator to restart; recovery required exact new identities and a real minimal inference [S80-08]. This is safety evidence, not a planned injection or benchmark.
- **[OPEN]** No fault-injection or recovery result was measured in this research pass.
- **[OPEN]** No planned fault-injection result was measured in this research pass.
