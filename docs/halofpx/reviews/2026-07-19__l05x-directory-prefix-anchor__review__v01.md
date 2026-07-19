# L05x directory-prefix anchor independent review

**Result: ACCEPT. No actionable finding or genuine blocker remains.**

## Reviewed contract

The final source conforms to ADR-0026 and the current Section 63 Wiki boundary.
It preserves `initialize_writer_lock_anchor_once()` as the L05w compatibility
extent and adds only the discard-required `envelopes`, `attempts`, and `staging`
prefix. Each fixed directory is created no-replace, pinned through `O_PATH`,
corrected on the pinned inode with
`SYS_fchmodat2(path_fd, "", 0700, AT_EMPTY_PATH)`, identity-matched during the
read-only reopen, validated as empty on the pinned Btrfs authority, and
synchronized before the accumulated-prefix and final root barriers.

The writer and fixture OFD locks remain held through final revalidation. Every
post-latch outcome remains discard-required. The inherited L05w call leaves all
L05x facts false. Static/archive review confirms two production objects, four
callable definitions, no production fault hook, default-off and
`EXCLUDE_FROM_ALL` routing, no product/install/export edge, and the general
mutation gate still closed. The separate Linux x86-64 controllers are also
default-excluded and outside CTest and product graphs.

## Evidence reconciliation

- The receipt parses as JSON and all nine recorded source SHA-256 values match
  the reviewed worktree, including production anchor
  `463a9e6d60c6b87ebc0190c284f03c8b73208cf8c3b35a21cee3f8df1ba6dea5`,
  ptrace controller
  `647a291844a9194eb922a3c7ab53dd58095b976ff2138d85345b1ebf6530faf7`,
  and returned-fault controller
  `433964c8517beaf26020c4f6ba8b924424e562de749bd000fbab3bc869f401a0`.
- Feature-off controls passed 40/40 on Windows and 39/39 on both Linux build
  classes. The focused initializer suite passed 6/6 on nimo-1 Release,
  nimo-2 Release, and nimo-2 ASan/UBSan.
- Live qualification passed 100 Release roots per node plus 25 ASan/UBSan
  roots on nimo-2: 225 qualified roots with the exact four-entry final tree.
- Exact-production ptrace qualification passed 1,000/1,000 runs: ten
  boundaries, entry and exit, 25 repetitions per cell per node. The retained
  logs contain 775,400 raw events.
- Returned-fault qualification passed 814/814 cases per node, 1,628 total.
  On nimo-2 the external controller was ASan/UBSan-instrumented while targeting
  the exact qualified Release production binary, with zero sanitizer stderr.
  The fully sanitized-target wait-bound failure remains excluded evidence and
  is not claimed as a pass.
- Retained evidence bundles were independently decompressed and inventoried.
  nimo-1 is 7,759,519 bytes, 10,401 entries, SHA-256
  `b6e8fe9ee3d359af794be478762a3b09ea3165dce8cc22886c700e147e2fba47`;
  nimo-2 is 8,988,115 bytes, 11,443 entries, SHA-256
  `de64eb0189984b18d8ad5069b5648285d459bef5838e16f437774b6236eed83f`.
  Ptrace indexes and summary, returned-fault maps/indexes/manifests, retained
  images, archives, controllers, and executable identities also reconcile.
- The inference and RPC services retain their recorded PIDs and health/listen
  state. The four immutable reference clones remain clean at their recorded
  commit and tree identities, including ROCmFPX
  `61f2f2d7bc4955e9bca821095ef69125837133b5` / tree
  `0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`.

## Provenance, rollback, and nonclaims

The implementation is target-native. No GPL llama-ai code or documentation,
CachyLlama code, donor format, new donor dependency, or direct cherry-pick was
admitted; no P3 record is required for this slice. Rollback remains source-only
because all affected targets are default-off and excluded, and qualification
media remains discard-only authority even when retained as immutable evidence.

This acceptance proves only the bounded process-crash-qualified directory
prefix. Root `fsync` is not power-loss or device-flush proof. L05x does not
write a marker, `HEAD`, envelope, attempt, staging file, or publication record;
does not rename, unlink, repair, adopt, or complete initialization; enables no
persistent server write, cache hit, restore, provider, inference, HIP, Vulkan,
RPC, WebUI, or L14Q behavior; and makes no inference-performance claim.
