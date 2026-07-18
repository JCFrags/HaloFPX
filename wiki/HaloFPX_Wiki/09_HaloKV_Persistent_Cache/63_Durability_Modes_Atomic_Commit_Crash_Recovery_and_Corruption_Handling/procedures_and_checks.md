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

Pin TLA+ Tools `v1.7.4` at tag commit `5a47802b5c391f59ecdd44117981f4ff8c0656ba`, verify and record the downloaded `tla2tools.jar` SHA-256, and run separate checkpoint-atomicity safety and reduced liveness configurations. Exercise two-rank preparation/publication, exact predecessor and protected-anchor identity, writer crash/restart/transfer, corruption/removal, stale fingerprints, digest/predecessor and cross-lineage replay, rejection, recomputation, abandonment, and recovery. Require negative counterexamples for premature acknowledgement, mixed-generation recovery, newest-unanchored selection, digest replay, and cross-lineage anchor replay. P63-00 satisfied this formal-model gate on 2026-07-18; its acceptance opens only implementation of the disabled offline writer/fault harness [S63-07].

The writer harness must then crash at every concrete publication boundary and inject ENOSPC, EDQUOT, EIO, read-only, and sync failures before L05 exit. Passing the formal model does not authorize persistent server writes or canary use.

## M63-01 crash-point matrix

Terminate only the disposable writer/coordinator after every write, sync, rename, and acknowledgement boundary. Restart and record recovered generation. Repeat for approved process-kill, isolated reboot harness, and controlled power-loss cases. Never use the production store; reboot/power tests require the Section 80 operator-approved hardware procedure.

## M63-02 corruption matrix

Within copied disposable fixtures, flip bits, truncate, append, swap object names, alter lengths/digests, duplicate generations, remove rank shards, and inject stale topology fingerprints. Acceptance: every affected generation is rejected/quarantined and inference recomputes correctly.

## M63-03 filesystem/device matrix

For the pinned filesystem/mount/kernel/SSD firmware, verify performance, turn-durable, and strict acknowledgement promises. Record flush latency, writeback errors, SMART unsafe shutdowns/media errors, recovery time, and leaked unreachable bytes.

## Promotion rule

No mode label is shipped until its exact failure model passes. The proposed protocol must also pass P63-00 before implementation approval, but model checking cannot substitute for M63-01..03. `fsync` return errors, ENOSPC, EDQUOT, EIO, directory-sync failure, and rank timeout are commit failures, not warnings.
