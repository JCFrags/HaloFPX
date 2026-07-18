---
section_id: "63"
title: "Durability and corruption tests"
status: "needs-machine-validation"
last_verified: "2026-07-18"
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

The accepted `b8123fe5` offline coordinator slice covers order, exact predecessor and manifest-anchor binding, in-process root serialization, and fail-closed ambiguous anchor replacement [S63-08]. It does not satisfy this writer-harness requirement: its scripted backend is not a durable-filesystem simulator, and no M63-01 crash/recovery evidence has yet been promoted.

The accepted `4366e493` simulator adds high-level live/durable crash projection and 134,400 repeated core cases [S63-09]. It still does not satisfy M63-01: byte/capacity/short-I/O faults, asynchronous stale-attempt fencing, concrete process/filesystem boundaries, and retained disposable-target recovery evidence remain open.

The accepted `3ae385d2` slice supplies the final exact predecessor CAS and nonzero attempt identity [S63-10]. Its clean CPU matrix passed 13/13 HaloFPX and 7/7 inherited controls; 100 coordinator plus 100 simulator processes passed, including 134,400 repeated core fault cases. M63-01 still requires authenticated attempt registration and per-operation late-completion fencing, canonical bytes, byte/capacity/short-I/O faults, concrete process/filesystem boundaries, and disposable-target recovery evidence.

The accepted `d85ee807` slice propagates the attempt identity through all 23 synchronous lifecycle operations, gates acknowledgement on durable close, and expands the repeated core matrix to 147,200 cases [S63-11]. Focused tests reject wrong, abandoned, committed, replayed, and uncertain IDs and prove ambiguous begin/abandonment blocks fresh attempts. M63-01 still requires a persistent authenticated registry, real asynchronous cancellation/completion tests, reconciliation, canonical bytes, byte/capacity/short-I/O faults, and concrete disposable filesystem/process recovery.

The accepted `8537a830` L05e slice supplies canonical authenticated anchor bytes, exact domain separation, a checked independent golden encoder, bounded hostile-input rejection, and an authenticated-only carrier [S63-12]. Its clean control passed 16/16 HaloFPX and 7/7 inherited tests, followed by 100 C++ and 100 independent Python processes. It does not open M63-01: the provisional coordinator's store/predecessor representation must first be reconciled, and bootstrap, protected key authority, cross-process exact-envelope CAS, byte/capacity/short-I/O faults, and disposable filesystem/process recovery remain open.

The accepted `29cd9581` L05f slice completes that coordinator wire reconciliation [S63-13]. The clean CPU/WebUI-off control passed 16/16 HaloFPX and 7/7 inherited tests. Exact anchor, coordinator, and simulator binaries then passed 100 processes each, including 147,200 repeated core simulator cases. The first independent review found and the implementation closed a same-key-tuple/different-master authority gap before final ACCEPT. M63-01 remains closed pending an authenticated protected key/authority registry, explicit administrative bootstrap, cross-process coordination, real asynchronous completion/reconciliation, byte/capacity/short-I/O faults, and concrete disposable filesystem/process recovery.

The accepted `d610e82f` L05g slice supplies only the memory authority and opaque bootstrap plan [S63-14]. Its clean CPU/WebUI-off control passed 18/18 HaloFPX and 7/7 inherited tests, followed by 100 anchor and 100 authority processes. Independent review found and closed incomplete key-derived temporary cleanup before final ACCEPT. M63-01 and bootstrap execution remain closed pending authority-admitted manifest proof, protected registry/high-water and replay storage, external administrative credential/token policy, conclusive absent-state proof, exact create-if-absent and ambiguity reconciliation, cross-process coordination, and concrete filesystem faults/recovery.

The accepted `c4cd76f0` L05h slice supplies that authority-admitted manifest proof [S63-15]. Its clean CPU/WebUI-off control passed 18/18 HaloFPX and 7/7 inherited tests, followed by 100 manifest-auth and 100 authority processes. Review corrected an attempt-identity wording overclaim, and the full matrix added only exact authority paths to L04a/L04b offline allowlists before final ACCEPT. M63-01 remains closed pending external operator/token authorization, protected registry/high-water and replay storage, conclusive absence, create-if-absent execution and ambiguity reconciliation, cross-process coordination, and concrete filesystem faults/recovery.

The accepted `31e4d6c` L05i slice supplies the external authenticated token wire but not consumption [S63-16]. Its clean CPU/WebUI-off control passed 21/21 HaloFPX and 7/7 inherited tests, followed by 200 token, 200 authority, and 200 independent golden-vector processes. Review found and closed body-shape, full-authentication-input, KDF framing, and noncanonical-fixture defects before final ACCEPT. M63-01 and bootstrap execution remain closed pending protected registry/high-water and replay storage, atomic token consumption, credential/principal policy, conclusive absence, create-if-absent and ambiguity reconciliation, cross-process coordination, and concrete filesystem faults/recovery.

The accepted `76494b59` L05j slice supplies an authenticated registry declaration and removes raw dynamic registry fields from authority input [S63-17]. Its clean CPU/WebUI-off control passed 24/24 HaloFPX and 7/7 inherited tests, followed by 200 registry, 200 authority, and 200 independent golden-vector processes. Review found and closed public-helper oversized-ID handling, Windows signed-`char` non-ASCII admission, and key/carrier/private-binding coverage gaps before final ACCEPT. M63-01 and bootstrap execution remain closed: the snapshot has no protected-origin or latestness proof, old valid snapshots remain admissible, and durable high-water advancement, atomic token consumption, credential/principal policy, conclusive absence, create-if-absent and ambiguity reconciliation, cross-process coordination, and concrete filesystem faults/recovery are still absent.

The accepted `354bfe3` L05k slice supplies the offline exact compare-and-advance consumption contract [S63-18]. Its clean CPU/WebUI-off control passed 27/27 HaloFPX and 7/7 inherited tests, followed by 200 successor, 200 authority/consumption, and 200 independent golden-vector processes. Independent review found and closed six blockers covering secret cleanup, execution authority, exact positive-state evidence, proof completeness, distinct-command concurrency, and re-entrant quarantine observation before final ACCEPT. M63-01 and bootstrap execution remain closed: there is no concrete protected backend, restart-surviving attempt/quarantine state, cross-process CAS, reconciliation, protected key custody, rollback proof, conclusive absence, create-if-absent, or concrete filesystem fault/recovery evidence.

The accepted `dbbdef1` L05l slice supplies only ambiguity reconciliation for the exact uncertain L05k operation retained by the same backend [S63-19]. Its clean CPU/WebUI-off control passed 29/29 HaloFPX and 7/7 inherited tests, followed by 200 authority/reconciliation, 200 successor-golden, and 200 independent reconciliation-golden processes. Independent review first required a separate commitment vector and serializer, a two-fresh-attempt race, re-entrant quarantine evidence, and complete binding/outcome coverage before final ACCEPT. M63-01 and bootstrap execution remain closed: no concrete protected registry, restart-surviving or cross-process fence, protected key custody/rollback proof, durable bootstrap-material proof, protected-anchor inspection/create-if-absent, or concrete filesystem fault/recovery evidence exists.

The accepted `b60c2ee` L05m slice supplies only synthetic bootstrap-material preparation after direct or reconciled authority consumption [S63-20]. Its clean CPU/WebUI-off control passed 31/31 HaloFPX and 7/7 inherited tests, followed by 200 authority/material, 200 static-contract, and 200 independent golden-vector processes. Independent review found and closed aggregate-limit underflow, moved-from ownership, missing independent commitment recomputation, incomplete provenance access, ineffective isolation checks, post-positive exception classification, and adversarial-matrix gaps before final ACCEPT. M63-01 and bootstrap execution remain closed: the proof is permanently non-convertible, and there is no concrete source, protected registry, restart/cross-process fence, real filesystem synchronization, protected-anchor inspection/create-if-absent, persistent writer, or disposable-target recovery evidence.

## M63-01 crash-point matrix

Terminate only the disposable writer/coordinator after every write, sync, rename, and acknowledgement boundary. Restart and record recovered generation. Repeat for approved process-kill, isolated reboot harness, and controlled power-loss cases. Never use the production store; reboot/power tests require the Section 80 operator-approved hardware procedure.

## M63-02 corruption matrix

Within copied disposable fixtures, flip bits, truncate, append, swap object names, alter lengths/digests, duplicate generations, remove rank shards, and inject stale topology fingerprints. Acceptance: every affected generation is rejected/quarantined and inference recomputes correctly.

## M63-03 filesystem/device matrix

For the pinned filesystem/mount/kernel/SSD firmware, verify performance, turn-durable, and strict acknowledgement promises. Record flush latency, writeback errors, SMART unsafe shutdowns/media errors, recovery time, and leaked unreachable bytes.

## Promotion rule

No mode label is shipped until its exact failure model passes. The proposed protocol must also pass P63-00 before implementation approval, but model checking cannot substitute for M63-01..03. `fsync` return errors, ENOSPC, EDQUOT, EIO, directory-sync failure, and rank timeout are commit failures, not warnings.
