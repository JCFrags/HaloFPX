---
section_id: "63"
title: "Durability and corruption tests"
status: "needs-machine-validation"
last_verified: "2026-07-19"
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

The accepted `6d21c34` L05n slice supplies the final synthetic protected-anchor create/reconciliation model [S63-21]. Its fresh CPU/WebUI-off control passed 33/33 HaloFPX and 7/7 inherited tests, followed by 200 authority/state-machine, 200 static-contract, and 200 independent golden-vector processes. Contract and implementation review found and closed create-attribution, repeated-reconciliation authority, post-linearization error, terminal-close validation, phase ownership, behavioral-matrix, and initialization-OOM defects before final ACCEPT. M63-01 and all persistence gates remain closed: synthetic absence, synchronization, and close claims do not establish a concrete registry, material writer, anchor backend, filesystem durability, restart/cross-process fencing, rollback resistance, or disposable-target recovery.

The accepted `cd4a8dde` L05o contract freezes the first concrete protected-registry laboratory boundary but contains no backend [S63-22]. Independent adversarial review closed twelve contract blockers across wire/layout closure, transition authority, selector identity, uncertainty, quarantine, credentials, locking, initialization, reserve accounting, result naming, inner-envelope authentication, and hostile recomputed-tag validation before final ACCEPT. The standalone checker passed eight fixtures and 3,260 mutations; the CPU/WebUI-off control remained 33/33 HaloFPX and 7/7 focused inherited tests. Read-only nimo-2 inventory selected the future disposable qualification host without creating a directory, loop device, mount, credential, service, model, cache, or other node state. M63-01 remains open until the excluded Linux backend, concrete syscalls, multi-process fencing, boundary `SIGKILL`, restart recovery, corruption/hostile-path/capacity/fault matrices, raw disposable-target evidence, and an independent implementation review pass. Filesystem/device/power-loss durability, whole-domain rollback resistance, persistence, and runtime linkage remain unclaimed.

The accepted `a7f0ba7c` L05o wire slice closes only the target-native parsing,
authentication, and semantic-validation prerequisite [S63-23]. Its Release
qualification passed 80/80 configured CTests, 36/36 HaloFPX tests, and 200
processes each of the C++ validator, static isolation contract, and independent
oracle. Recomputed-tag cases exercise root nonzero constraints, arbitrary
high-water and selector generations, independently wrong HEAD context, fully
repaired `H+2`, wrong operation binding, terminal recovery classes, and both
quarantine attribution shapes. The validator remains non-authoritative: future
recovery must verify PREPARE plus exact transition bytes before admitting a
successor HEAD. M63-01 remains open on the same concrete Linux syscall,
cross-process, crash/restart, hostile-path, corruption, capacity, and raw-node
evidence gates; no persistence or durability claim has opened.

The accepted L05p sequence through `849e3c84` closes only portable fake reads
through operation 5 and exact recovery/request classification [S63-24]. The
final candidate passed 84/84 Windows Release tests, 7/7 focused matrices on
Windows Release and Debug plus optimized and ASan/UBSan Linux, and 2,218
repeated final operation-5 processes. The separately corrected operations 1-4
credential profile passed another 2,200 exact-candidate repeat processes.
These results qualify bounded decoding, authentication, precedence, and
isolation from product linkage; they do not satisfy M63-01 because no real
filesystem, syscall, cross-process lock, process crash, restart, or node
mutation was exercised.

The accepted `cb20cc8a` L05q slice closes only fake operation-6 admission,
recovered terminalization, and restart projection [S63-25]. Each exhaustive
process covers 3,072 action/slot/history cases, 287 admitted and 1,393 forbidden
new products, 10,335 restart projections, 712 truncations, 5,696 bit flips, and
eight authenticated semantic attacks. Windows Release passed 84/84 twice; the
focused Windows, optimized Linux, and ASan/UBSan Linux gates passed, followed
by 3,208 repeat processes. M63-01 remains open: the target is still excluded,
fake-only, and has no filesystem primitive, Linux adapter, process-crash
harness, durable writer, persistence authority, provider, or inference edge.
The inherited Debug authority assertion is retained as a separate baseline
issue and is not evidence against the L05q target.

The accepted `181a7bd8` L05r slice closes only fake sticky-quarantine
publication operations 69-76 [S63-26]. Its exact candidate admits 155 of 960
operation products, rejects 805, and covers 497 restart projections including
18 state-only failure/death frontiers. Windows Release passed 40/40 HaloFPX
tests; clean optimized and ASan/UBSan Linux passed 39/39 each. The retained
exhaustive layer adds 25/25 optimized and 18/18 sanitizer processes; eight
sanitizer pairs overlapped after a controller reconnect and are explicitly
reported as concurrency stress. M63-01 remains open because all mutation is
still inside the excluded fake authority: no Linux filesystem syscall,
cross-process lock, process-kill recovery, persistent writer, provider, cache,
restore, or inference path was exercised.

The accepted `b80ab1d6` L05s slice closes only the nonmutating Linux
pre-initialization primitive gate [S63-27]. Windows feature-off passed 40/40;
nimo-1 Release and nimo-2 ASan/UBSan each passed 42/42. Across the two nodes,
the retained qualification includes 300 accepted fresh processes, 300 accepted
contention pairs, and 150/150 final-executable attacks split evenly among
anchored traversal, a real second-Btrfs-mount cross-mount substitution, and
actual lock-inode replacement. Lifetime probes prove the five-second OFD busy
deadline remains pinned while a fork-inherited alias lives, releases only after
that alias dies, and is not retained across exec. Before M63-01 may advance,
the next initializer contract must separately enumerate every mutation and
crash boundary. L05s itself creates no registry state, publishes no bytes, and
has no product, provider, cache, restore, inference, HIP, Vulkan, or RPC edge.

The accepted `e918d1f8` L05t slice closes only the no-I/O initializer linkage
seam frozen by ADR-0026 [S63-28]. The public target-owned digest admits exactly
1..1024 registry-envelope bytes and preserves caller output on every failure.
The Linux-only `STATIC EXCLUDE_FROM_ALL` archive contains one object and one
global definition; its unsanitized form imports only that digest, while the
sanitized audit admits only the corresponding ASan/UBSan runtime symbols.
Windows feature-off passed 40/40; both nodes passed the focused Linux Release
matrices; nimo-2 passed the focused ASan/UBSan matrix with zero findings; and
2,400 fresh seam/wire processes failed zero times. M63-01 remains open because
the slice never opens a root or credential, mutates a filesystem, publishes
bytes, restores state, links a provider/server/inference path, or establishes
durability or performance.

The accepted `940af075` L05u slice closes only the inherited sealed-input
transport boundary [S63-29]. The process blocks signals, privately unshares its
descriptor table, requires one task, validates exact-name zero-link tmpfs
memfds at fd 3 and fd 4, rejects aliases, reads bounded bytes into a dedicated
locked mapping, revalidates the descriptor identities, and wipes, unlocks,
unmaps, and closes everything before returning. Each Linux invocation covers
1,625 fresh child cases plus a multithreaded-launch rejection; nimo-1 Release,
nimo-2 Release, and nimo-2 ASan/UBSan passed the focused four-test matrix. The
positive result is only `transport_validated_no_root_access`. Before any root
or `writer.lock` access, the next slice must authenticate the credential and
every launcher-pinned predecessor field inside locked storage and discard it;
L05u itself grants no reusable authority and performs no registry mutation.

The accepted `c201c8dc` L05v slice closes the predecessor-authentication and
launcher-pin consistency boundary before root access [S63-30]. The exact fd4
envelope is structurally parsed and HMAC-authenticated under the locked fd3
secret; the registry-lab digest, registry ID/epoch, authority scope, policy,
high water, key tuple, and key continuity must all match the caller receipt.
The facts-only verifier returns no carrier, secret, descriptor, or private
authority binding, and every final failure clears the aggregate authentication
and receipt-match facts. Each Linux input invocation covers 3,303 fresh child
cases plus a shared-thread rejection; the three qualified Release/sanitizer
invocations covered 9,909 fresh processes plus three thread-sharing cases.

Treat this result as integrity under a caller-supplied credential, not as
issuer or origin proof. A fully matched old snapshot may succeed in a fresh
process; process-local one-shot consumption does not prove latestness,
revocation, monotonicity, or rollback resistance. Root/fixture admission and
the discard-only `writer.lock` anchor remain the next separately gated slice.
L05v itself opens no root, mutates no registry, and admits no persistence,
provider, cache, restore, or inference edge.

The accepted `80ab1edc` L05w slice closes only the discard-required
`writer.lock` anchor prefix [S63-31]. It retains the authenticated fd3/fd4
state in locked file-private storage, closes both descriptors before the first
parent/root/fixture syscall, pins exact Btrfs identities and reserve/read-only
state, holds the fixture OFD lock, and obtains nonzero root/store identifiers
before latching. After the latch it creates only `writer.lock` through
contained exclusive `openat2`, applies and verifies mode `0600`, validates and
file-syncs the inode, holds its exact whole-file OFD lock, and proves it is the
root's sole entry. Every latched ordinary outcome is
`initialization_discard_required`; a preexisting nonempty root is discard-only
without a new mutation.

Qualification retained 227 promoted live anchors and 400/400 exact-binary
ptrace kills across four writer-lock boundary syscalls and both entry/exit
phases, with 25 repetitions per group per node and 171,950 raw JSONL events.
Focused Release and ASan/UBSan suites passed 6/6, while feature-off controls
remained unchanged. Controllers deactivate the verified loopback medium as
authority and retain the detached image only as immutable evidence. Rollback
is source-only because the target remains default-off and excluded; any root
that crossed the latch must never be adopted or repaired. L05w does not sync
the root directory, complete initialization, publish state, enable persistent
writes, or add a provider/cache/restore/inference or performance edge.

The accepted `051084fa` L05x slice closes only the discard-required fixed
directory prefix [S63-32]. It preserves the committed L05w stop boundary, then
creates `envelopes`, `attempts`, and `staging` in that order. Each directory is
created no-replace, pinned through `O_PATH`, mode-corrected on that exact inode,
identity-matched during anchored read-only reopen, proven to be the expected
empty Btrfs child, and synchronized before the next accumulated-prefix
barrier. The final barrier precedes one root-directory `fsync`, after which an
anchored revalidation proves the exact four-entry root layout while the writer
and fixture OFD locks remain held. Every latched outcome remains
`initialization_discard_required`.

Qualification retained exactly 225 live roots, 1,000/1,000 exact-production
ptrace injections over ten boundaries and both entry/exit phases with 775,400
raw events, and 1,628/1,628 generic returned-fault cases. On nimo-2 the
returned-fault controller was ASan/UBSan-instrumented but targeted the exact
qualified Release production binary; the fully sanitized-target controller
smoke that exceeded its wait bound remains excluded development evidence.
Evidence bundles remain pinned as nimo-1
`b6e8fe9ee3d359af794be478762a3b09ea3165dce8cc22886c700e147e2fba47`
and nimo-2
`de64eb0189984b18d8ad5069b5648285d459bef5838e16f437774b6236eed83f`.
Source authority remains the exact ROCmFPX base
`61f2f2d7bc4955e9bca821095ef69125837133b5` / tree
`0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`; all four clean reference-clone
locks are pinned in the receipt.
Rollback is source-only because every target remains default-off and excluded;
latched qualification roots stay discard-only and may never be adopted or
repaired. Root `fsync` is process-crash ordering evidence, not power-loss or
device-flush proof. L05x does not create a marker or `HEAD`, write an envelope,
attempt, staging file, or publication record, complete initialization, enable
persistence, or add a cache-hit/restore, provider, inference, or performance
edge.

The accepted `f61778d9` L05y slice closes only ADR-0026 step 3: publication of
the discard-only initializing root marker [S63-33]. After the complete L05x
barrier, it derives and self-verifies the path policy and authenticated marker
before mutation, creates only `staging/initialize-root.tmp`, performs bounded
offset-zero write and exact EOF/readback authentication, file-syncs and
read-only revalidates the exact inode, then publishes only through
`renameat2(RENAME_NOREPLACE)`. Root and staging are synchronized before the
final authenticated five-entry root-layout proof. The marker binds the pinned
root/store, registry, key, filesystem, mount, owner, capacity, and writer-lock
facts, has state `initializing`, and has a null initial-HEAD field. It selects no
`HEAD`; every invocation that crossed the latch remains
`initialization_discard_required`, with no per-entry cleanup, adoption, repair,
completion, or retry authority.

Windows feature-off passed 40/40. Nimo-1 Release passed 520/520 initializer
build, 48/48 HaloFPX, 6/6 focused, 7/7 inherited, 581/581 feature-off build,
and 39/39 feature-off HaloFPX tests. Nimo-2 ASan/UBSan passed 517/517,
48/48, 6/6 focused, 349/349 feature-off build, and 39/39 feature-off HaloFPX
with zero accepted sanitizer findings. The sole inherited `test-gguf` UBSan
failure reproduced exactly on the untouched selected base: candidate/base
source SHA-256 is
`9c513b99b9052324395c4f3fd73626b155c43f9a867813d15274f31a633eceea`
and the normalized signature is
`35fd665ba5f862efd4f3f81101588e34a0d2ba8335c7c55aeba1f99631533ec4`.
It is retained as a matched-base exception, not an L05y regression.

Live qualification passed 100 Release roots per node plus 25 nimo-2
ASan/UBSan roots: 225/225 exact five-entry layouts and independent marker
reconstructions. Exact-production ptrace qualification passed 700/700 across
seven mutation boundaries, entry/exit, and 25 repetitions per cell per node,
retaining 1,124,300 JSONL events and 172,821,070 receipt bytes. Five 409-case
returned-fault lanes passed 2,045/2,045. Authoritative ptrace bundles are
nimo-1 `20278a3bce90d1f163ed51018b5ea2cd53c923a40856b76bd6499959a25ae8f5`
and nimo-2
`3c7847b369cea3492f40f9425baafea60ba28b12b4ab265084a8f844f5b02730`;
returned-fault bundles are
`f328e839737381cb348b582c8574dc7063f4aca2bc3868edc5c0e34d351fb312`
and
`767d2986ed4381908aa5c63674a7b4e55187d2408f35ab7d09cf7cf3edc9d0d0`.
The full-regression bundles are
`ebc3d437b96c6906fedb82ce40209f63e5b5b4bd8be417a7e131c5d594dc6081`
and
`206e8e27e15f65460a1dfad72dc8464f2d4584bcee7d0b969deb788267bef75f`.

An ASan-instrumented target under ptrace on nimo-1 reproducibly hung at start
for local and cross-node binaries, before and after a controlled reboot;
ASan-only hung and UBSan-only passed. Bounded cleanup left no active residue.
This is a host/tool limitation and is neither a pass nor a sanitizer-clean
claim; complete target-plus-controller ASan/UBSan evidence passed on nimo-2.
After reboot, nimo-1's preserved service recovered at PID 971 on port 8081 with
HTTP 200 and `NRestarts=0`; nimo-2 RPC remained continuous at PID 3562775 on
`0.0.0.0:50052`, also with `NRestarts=0`.

Rollback remains source-only; latched roots remain whole-media discard-only.
The next initialization work is the exact predecessor envelope, initial
`HEAD`, initialized-marker construction/replacement, completed final-layout
validation, and only then separately gated normal reopen/recovery. L05y proves
process-crash ordering for only the initializing marker; it does not prove
power-loss/device-cache durability, completed initialization, persistence,
cache hit/restore, provider/inference behavior, or performance.

## M63-01 crash-point matrix

Terminate only the disposable writer/coordinator after every write, sync, rename, and acknowledgement boundary. Restart and record recovered generation. Repeat for approved process-kill, isolated reboot harness, and controlled power-loss cases. Never use the production store; reboot/power tests require the Section 80 operator-approved hardware procedure.

## M63-02 corruption matrix

Within copied disposable fixtures, flip bits, truncate, append, swap object names, alter lengths/digests, duplicate generations, remove rank shards, and inject stale topology fingerprints. Acceptance: every affected generation is rejected/quarantined and inference recomputes correctly.

## M63-03 filesystem/device matrix

For the pinned filesystem/mount/kernel/SSD firmware, verify performance, turn-durable, and strict acknowledgement promises. Record flush latency, writeback errors, SMART unsafe shutdowns/media errors, recovery time, and leaked unreachable bytes.

## Promotion rule

No mode label is shipped until its exact failure model passes. The proposed protocol must also pass P63-00 before implementation approval, but model checking cannot substitute for M63-01..03. `fsync` return errors, ENOSPC, EDQUOT, EIO, directory-sync failure, and rank timeout are commit failures, not warnings.
