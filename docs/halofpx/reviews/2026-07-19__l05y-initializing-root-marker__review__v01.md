# L05y initializing root marker independent review

**Result: ACCEPT. No actionable code, security, provenance, rollback, or
promotion-gate finding remains.**

## Reviewed contract and source

L05y implements only ADR-0026 step 3 after the qualified L05x prefix: it
constructs and publishes one authenticated `initializing` `root.marker` through
the fixed `staging/initialize-root.tmp` name. The marker contains a null initial
`HEAD`; no envelope, `HEAD`, attempt, completed initialization, reopen, repair,
adoption, provider, cache hit, restore, or inference edge is present.

All ten source SHA-256 values in the receipt match the reviewed worktree. The
initializer remains Linux-only, default-off, `STATIC EXCLUDE_FROM_ALL`,
uninstalled, unexported, and absent from product graphs. Its archive contains
two objects, five admitted callable definitions, nine admitted HaloFPX imports,
and no production fault hook. The L05w and L05x entrypoints remain stop-boundary
controls and expose no L05y-positive fact.

The secret-bearing wire credential is placement-constructed in the locked
input mapping only after the fd 3/fd 4 transport, authentication, launcher-pin,
identity, alias, and close checks. Raw credential input is immediately wiped and
zero-verified. The credential is destroyed exactly once before destruction of
the locked input object and whole-mapping wipe, zero verification, `munlock`,
and `munmap`.

Path-policy derivation, typed marker admission, bounded canonical encoding,
content digest, and semantic self-verification finish before the temporary is
opened. Publication uses exclusive anchored create, fd-bound mode and identity
checks, bounded offset-zero I/O, exact EOF/readback authentication, file
`fsync`, then an immediate read-only name-to-inode, bytes, EOF, and
authentication revalidation before the sole `RENAME_NOREPLACE`. Root and
staging directory synchronization and final authenticated five-entry layout
validation follow while both OFD locks remain held. Ambiguity or cleanup
failure remains sticky whole-root discard; no per-entry cleanup is admitted.

The Windows authority test calls `_set_error_mode(_OUT_TO_STDERR)` and clears
`_CALL_REPORTFAULT` with `_set_abort_behavior`, preserving assertion text and a
nonzero exit while suppressing abort/ReportFault UI. Its source contract and the
qualified Windows execution both passed.

## Evidence reconciliation

- Windows Release passed 40/40 HaloFPX tests. The final log hashes to
  `e91dd0e39be294f14d921d5f13c8f7e4ffd0f4b32f245deb1d7ba94e713de985`;
  the authority executable hashes to
  `971c1858a8d27bad83b6deb5a74d5790a3a37db5554de55d293567b9dff0cb21`
  and `llama-server` to
  `647017d1889f8b91660541e7eb9995ed8cb7ca698e93fcb0c9b51e4dd2aa059b`.
- Nimo-1 Release passed 520/520 initializer-build tests, 48/48 HaloFPX,
  6/6 focused L05y, 7/7 inherited, 581/581 feature-off build, and 39/39
  feature-off HaloFPX tests. Nimo-2 ASan/UBSan passed 517/517, 48/48,
  6/6 focused L05y, 349/349 feature-off build, and 39/39 feature-off HaloFPX
  with zero accepted sanitizer findings.
- The sole inherited `test-gguf` UBSan failure reproduces on the untouched
  selected base. Candidate and base source both hash to
  `9c513b99b9052324395c4f3fd73626b155c43f9a867813d15274f31a633eceea`;
  normalized signatures both hash to
  `35fd665ba5f862efd4f3f81101588e34a0d2ba8335c7c55aeba1f99631533ec4`.
  It is an inherited matched-base exception, not an L05y regression.
- Live qualification passed 100 Release roots per node plus 25 nimo-2
  ASan/UBSan roots. All 225 had the exact five-entry layout. Independent,
  target-free marker reconstruction matched all 225 markers; the nimo-2
  125-marker aggregate is
  `b241093f1b9c81920a8ff3cdb4a5e6ef39539aad364031a947356c89087770fb`.
- Exact-production ptrace qualification passed 700/700 cells: seven mutation
  boundaries, syscall entry and exit, 25 repetitions per cell per node. The
  retained totals are 1,124,300 JSONL events and 172,821,070 receipt bytes,
  with exact `SIGKILL`/`WIFSIGNALED`, sticky-discard, fully allocated 1 GiB
  media, and whole-media cleanup proofs.
- The returned-fault controller source hashes to
  `57fefdf4c9c7a5193fdfeec1cf7561bacd4466f2abaf4226fb528aa50741c4b9`.
  Both nodes carry the same 17,344-byte, 409-case manifest at
  `fa6300b129a3a275b6396382f696f1cb6de2b2624f30a608661942e3ea4476f8`.
  Its arithmetic reconciles as 324 ordinary pre/late errno cases, 18 ordinary
  EINTR-once cases, 38 close-marker occurrence-2/3 cases, 12 short-I/O cases at
  endpoints 1 and 228, six zero-I/O cases, and 11 hostile/collision/reserve
  cases. Pre/late retryable EINTR requires the first replacement plus observed
  successful retry; publication attempts, intended/retained bytes and
  identities, forbidden cleanup syscalls, hostile substitution, and exact audit
  vectors are all controller-enforced.
- Nimo-2 passed 409/409 Release and 409/409 full ASan/UBSan returned-fault
  cases. Nimo-1 passed 409/409 Release-controller/Release-target,
  409/409 ASan/UBSan-controller/Release-target, and 409/409
  Release-controller/UBSan-target cases. The five distinct promoted lanes total
  2,045/2,045 with no failure.
- An ASan-instrumented target under ptrace on nimo-1 reproducibly hangs at
  start for local and cross-node binaries, before and after controlled reboot;
  ASan-only hangs and UBSan-only passes. Bounded cleanup left no active residue.
  This is an explicit host/tool limitation, not a pass or sanitizer-clean claim.
  Complete target-plus-controller ASan/UBSan coverage passed on nimo-2.

Direct retained-artifact checks matched every recorded core, full-regression,
ptrace, and returned-fault bundle hash, size, and entry count. Deterministic
reconstructions are byte-identical, including nimo-1 returned-fault bundle
`f328e839737381cb348b582c8574dc7063f4aca2bc3868edc5c0e34d351fb312`
and nimo-2 bundle
`767d2986ed4381908aa5c63674a7b4e55187d2408f35ab7d09cf7cf3edc9d0d0`.
The authoritative ptrace bundles are
`20278a3bce90d1f163ed51018b5ea2cd53c923a40856b76bd6499959a25ae8f5`
and
`3c7847b369cea3492f40f9425baafea60ba28b12b4ab265084a8f844f5b02730`.

Nimo-1 recovered after its controlled reboot at PID 971 with HTTP 200,
`NRestarts=0`, and the preserved service command. Nimo-2 RPC remains continuous
at PID 3562775 on `0.0.0.0:50052` with `NRestarts=0`. There are no active L05y
qualification mounts, loop devices, or controller processes. The receipt was
corrected during this review to distinguish that fact from intentionally
retained, unmounted excluded/canary/live-smoke images; those images are evidence
only and are not counted in promoted totals.

## Provenance, rollback, and nonclaims

All four immutable reference clones remain clean at their recorded commits and
trees. The implementation repository has no remote. The target-native scope has
zero exact normalized four-line matches against the pinned llama-ai and
CachyLlama material. No GPL implementation or documentation, CachyLlama unit,
donor format, dependency, direct cherry-pick, license/NOTICE/SBOM change, or P3
obligation entered L05y.

The accepted ADR, frozen L05y contract, and canonical Section 63 Wiki agree on
the discard-only boundary. The Wiki correctly stops at committed L05x until the
accepted L05y commit and receipt identity exist; promotion must add that exact
post-commit identity rather than pre-claiming it.

Rollback is source-only because the feature is default-off and excluded. Any
root that crossed the latch remains disposable and discard-only; rollback never
authorizes adoption or per-entry cleanup.

This acceptance proves process-crash-qualified publication of only the
initializing marker. It does not prove power-loss/device-cache durability,
completed initialization, reopen, persistence enablement, cache hit/restore,
tenant sharing, distributed behavior, inference performance, or
zero-regression performance. HIP, Vulkan, ROCmFPX, TurboQuant, ROCmFP4, RPC,
WebUI, and L14Q runtime behavior remain unchanged.
