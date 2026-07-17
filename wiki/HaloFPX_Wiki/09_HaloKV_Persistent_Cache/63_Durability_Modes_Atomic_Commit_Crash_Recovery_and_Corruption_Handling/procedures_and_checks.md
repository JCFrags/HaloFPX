---
section_id: "63"
title: "Durability and corruption tests"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["77", "80"]
---

# Procedures and checks

## Disposable-target and privilege gate

Before any mutation or fault, create a separate scratch filesystem/store and service instance; record the resolved path, backing device/loop image, store UUID, mount, ports, process IDs/cgroup, free-space and inode ceilings, evidence destination, out-of-band recovery route, stop conditions, and cleanup command. Refuse production cache/model/workspace/boot paths, real deployment devices, and sole evidence copies. File-fixture corruption and process termination normally need no root. Reboot, power-cut, kernel, dm/fault-injection, filesystem, controller, or physical-device tests require explicit operator approval through Section 80, minimum declared privilege, and a recovery/cleanup receipt.

## P63-00 model-checking prerequisite

Pin TLA+ Tools `v1.7.4` at tag commit `5a47802b5c391f59ecdd44117981f4ff8c0656ba`, verify and record the downloaded `tla2tools.jar` SHA-256, and run separate checkpoint-atomicity safety and reduced liveness configurations. Exercise every ordering of two ranks across prepare/durable plus manifest written/durable/published, coordinator crash/restart, corruption, missing rank, stale fingerprint, rejection, abandonment, and recovery. Include negative variants that allow premature durable-mode acknowledgement or mixed-generation recovery and require TLC to find counterexamples. This is a future gate: no model or TLC result exists as of 2026-07-17 [S63-06].

## M63-01 crash-point matrix

Terminate only the disposable writer/coordinator after every write, sync, rename, and acknowledgement boundary. Restart and record recovered generation. Repeat for approved process-kill, isolated reboot harness, and controlled power-loss cases. Never use the production store; reboot/power tests require the Section 80 operator-approved hardware procedure.

## M63-02 corruption matrix

Within copied disposable fixtures, flip bits, truncate, append, swap object names, alter lengths/digests, duplicate generations, remove rank shards, and inject stale topology fingerprints. Acceptance: every affected generation is rejected/quarantined and inference recomputes correctly.

## M63-03 filesystem/device matrix

For the pinned filesystem/mount/kernel/SSD firmware, verify performance, turn-durable, and strict acknowledgement promises. Record flush latency, writeback errors, SMART unsafe shutdowns/media errors, recovery time, and leaked unreachable bytes.

## Promotion rule

No mode label is shipped until its exact failure model passes. The proposed protocol must also pass P63-00 before implementation approval, but model checking cannot substitute for M63-01..03. `fsync` return errors, ENOSPC, EDQUOT, EIO, directory-sync failure, and rank timeout are commit failures, not warnings.
