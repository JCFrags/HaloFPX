---
section_id: "80"
title: "Fault Injection Sources"
status: "needs-machine-validation"
last_verified: "2026-08-12"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: ["Linux documentation accessed 2026-07-17"]
  hardware_revisions: []
related_sections: ["54", "56", "65", "78", "79"]
---

# Sources

Accessed 2026-07-17. Kernel interfaces are version-sensitive and must be matched to the target kernel before use.

| ID | Source and revision | Claims supported | Limitations/conflicts |
|---|---|---|---|
| S80-01 | [Linux fault-injection framework, accessed 2026-07-17](https://docs.kernel.org/fault-injection/fault-injection.html) | Allocation, block-I/O, NVMe, function, and fail-at-N controls | Moving documentation; exact target kernel config/interface requires inspection |
| S80-02 | [Linux `dm-flakey`, accessed 2026-07-17](https://docs.kernel.org/admin-guide/device-mapper/dm-flakey.html) | Controlled intermittent block failure/corruption | Destructive to test data; disposable target only |
| S80-03 | [Linux cgroup v2, accessed 2026-07-17](https://docs.kernel.org/admin-guide/cgroup-v2.html) | Scoped `memory.max` and cgroup OOM behavior | Kernel/config and service integration must be validated |
| S80-04 | [Linux USB4/Thunderbolt documentation, accessed 2026-07-17](https://docs.kernel.org/admin-guide/thunderbolt.html) | Device/topology/control model | Does not identify the target rail/retrain mechanism |
| S80-05 | [ROCmFPX scripts/tests at `a5605a7`](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394/scripts) | Fork state, prompt-cache, and regression mechanisms | Mechanism inventory, not target corruption-rejection evidence |
| S80-06 | [llama.cpp fragmented restore test at `788e07d`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tests/test-state-restore-fragmented.cpp) | Fragmented multi-sequence state-save/restore test pattern | Does not cover HaloKV or distributed crash consistency |
| S80-07 | [Project Section 56, verified 2026-07-16](../../09_HaloKV_Persistent_Cache/56_CachyLLama_Cache_Semantics_and_Porting_Map/README.md) | Fail-closed cache/state epoch, checksum, and version guidance | Wiki synthesis; implementation and machine proof remain open |
| S80-08 | [2026-08-12 nimo-2 HMM/global-OOM safety incident](../../../../../docs/halofpx/evidence/2026-08-12-target-hmm-oom-incident/README.md), base `b77f2bce6e7875ab065e09894f45915585c9f156` | Raw OOM victims, both service restart chains, stale coordinator RPC state, and real recovery request | Accidental production incident; not an authorized injection, calibrated memory test, or performance result |
