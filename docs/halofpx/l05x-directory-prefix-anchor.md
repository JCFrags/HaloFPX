# L05x discard-only directory-prefix anchor

L05x extends the L05w authenticated writer-lock anchor through the fixed empty
directory prefix required by M63-01b and ADR-0026. The inherited
`initialize_writer_lock_anchor_once()` call remains the compatibility control
and stops at the L05w extent. The new `initialize_directory_prefix_once()` call
uses the same file-private admission and anchor implementation, then creates
only `envelopes`, `attempts`, and `staging`, in that order.

The feature remains Linux-only, default-off, excluded from normal builds, and
isolated from product, install, and export graphs. Every invocation that
reaches the inherited mutation latch remains permanently classified as
`initialization_discard_required`.

## Admitted sequence and authority barriers

L05x inherits the exact L05v sealed-input authentication, fd3/fd4-before-root
ordering, pinned Btrfs parent/root/fixture admission, reserve/read-only checks,
fixture OFD lock, generated identifiers, and L05w `writer.lock` create,
file-sync, OFD-lock, and sole-entry proof.

After that fully qualified prefix, L05x performs this bounded sequence:

1. Create `envelopes` with `SYS_mkdirat(root_fd, name, 0700)`, retrying only
   `EINTR`. Because the caller's umask may reduce that initial mode to `0000`,
   anchored-open the new name with
   `O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW`, validate and pin its type,
   owner, device, mount, canonical child path, nonzero inode, and umask-subset
   mode, then repair that exact pinned inode using raw
   `SYS_fchmodat2(path_fd, "", 0700, AT_EMPTY_PATH)`. Keep the `O_PATH` fd held
   while anchored-reopening the name read-only and comparing its identity to the
   pin. Only then require exact mode `0700`, complete Btrfs
   filesystem/subvolume identity, canonical location, and emptiness before
   `fsync` of the readable directory fd.
2. Revalidate the complete accumulated prefix before creating `attempts`. The
   barrier includes parent/root/fixture identities, reserve/read-only state,
   exact root layout, the held writer and fixture lock descriptions and
   identities, and the created directory's identity and emptiness.
3. Repeat the same create, validation, directory-sync, and accumulated-prefix
   barrier for `attempts`, then for `staging`.
4. Revalidate the complete three-directory prefix before synchronizing the root
   directory.
5. `fsync(root_fd)` once, then perform a final anchored revalidation proving the
   exact four-entry root layout: `writer.lock`, empty `envelopes/`, empty
   `attempts/`, and empty `staging/`.

The writer and fixture OFD locks remain held through the final revalidation.
Cleanup retains the L05w ordering: close non-lock descriptors; wipe, unlock,
and unmap authenticated state; explicitly unlock and close writer then fixture
locks; release the root guard; and restore the signal mask. L05w calls leave
all L05x audit facts false.

## Failure and synchronization boundary

Every failure after the inherited mutation latch remains sticky
`initialization_discard_required`, including create collisions, returned I/O or
capacity errors, read-only transitions, late-completion disagreement, identity
or layout substitution, cleanup failure, response loss, and process death. An
already nonempty candidate root observed before this invocation mutates remains
`preexisting_root_discard_required`. No post-latch root may be adopted,
repaired, or retried for authority.

The directory and root `fsync` calls are process-crash ordering qualification
primitives. They are not proof of power-loss durability, storage-device cache
flush, controller firmware behavior, filesystem recovery after sudden power
loss, or cross-kernel persistence semantics.

L05x does not write a marker, `HEAD`, envelope, attempt, staging file, or
temporary publication record. It performs no rename, unlink, repair, adoption,
or completed initialization and enables no server persistence, cache hit, or
restore path. The result remains discard-required even when every L05x audit
fact is true.

## Qualification isolation

The exact production initializer archive remains two objects and admits exactly
four callable definitions. It contains no test fault hooks and does not link or
call the qualification controllers. Its project imports remain limited to the
two-stage registry digest lineage and the protected-facts verifier.

Crash and returned-fault qualification use separate Linux x86-64 test-only
controllers. Both are required to remain default-excluded, outside CTest and
outside product, install, and export graphs. The crash controller targets the
exact production binary at the ten L05x mutation boundaries: three `mkdirat`,
three `fchmodat2`, three directory `fsync`, and one root `fsync`, at syscall
entry and exit. The returned-fault controller changes syscall results externally and
must not add hooks to the production archive.

The qualified evidence set contains:

- clean 520/520 Release builds on both Strix Halo nodes, feature-off controls
  of 39/39 on nimo-1 Release and 39/39 on nimo-2 ASan/UBSan, and the Windows
  feature-off control at 40/40;
- the focused initializer suite at 6/6 on nimo-1 Release, nimo-2 Release, and
  nimo-2 ASan/UBSan, including the inherited L05w stop-boundary assertions;
- 100/100 fresh Release roots on each node and 25/25 fresh ASan/UBSan roots on
  nimo-2, plus default, `0077`, and hostile `0777` umask canaries on both
  nodes;
- 1,000/1,000 exact-production crash injections: ten boundaries, entry and
  exit, 25 repetitions per cell per node, retaining 775,400 raw JSONL events;
- 1,628/1,628 generic returned-fault cases across both nodes. The nimo-2
  814-case matrix sanitized the external controller and targeted the exact
  qualified Release production binary; it emitted no sanitizer diagnostics.
  A fully sanitized-target controller smoke that exceeded its wait bound is
  retained as excluded development evidence and is not counted as a pass;
- successful pre-latch read-only and reserve-exhaustion controls on both nodes,
  each with zero candidate-root mutation; and
- clean locked reference clones, unchanged inference services, exact source,
  archive, executable, controller, image, receipt-manifest, and evidence-bundle
  hashes, and independent ACCEPT reviews of the production seam and both
  controllers.

The exact identities and reconciliation details are pinned in the receipt.
This qualification admits only the default-off discard-only L05x prefix. It
does not open completed initialization, recovery, adoption, server persistence,
or cache-hit authority.

## Rollback

Source rollback removes or reverts the L05x API, directory-prefix implementation,
focused tests, qualification controllers, and CMake/static-audit routing while
preserving the committed L05w compatibility control. All affected targets are
default-off and excluded from normal builds. No product, service, deployment,
or persistent server state is changed by the source slice. Any qualification
root that crossed the latch remains discard-only; whole disposable media may be
retained as immutable evidence but never adopted or repaired.

Final hashes, run counts, evidence identities, cleanup receipts, and nonclaims
are pinned in `evidence/l05x-directory-prefix-anchor-receipt.json`. The
independent promotion review is recorded in
`reviews/2026-07-19__l05x-directory-prefix-anchor__review__v01.md`.
