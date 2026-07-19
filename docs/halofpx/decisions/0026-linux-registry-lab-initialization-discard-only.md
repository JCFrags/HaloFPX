# ADR-0026: Linux registry-lab initialization with discard-only recovery

- Status: accepted for L05t/M63-01b implementation; promotion remains closed
- Date: 2026-07-19
- Milestone: L05t / M63-01b

## Decision and build boundary

M63-01b may implement one administration-only transition from a newly created,
empty, disposable Btrfs root to the fixed initialized registry-lab layout. It
may not reopen or repair an interrupted root, compare-and-advance, publish
quarantine, link a provider, restore cache state, or enter inference.

The Linux target is
`halofpx-context-store-registry-lab-linux-initializer`. It is built only when
both `HALOFPX_REGISTRY_LAB_LINUX_PREINIT=ON` and the new default-off
`HALOFPX_REGISTRY_LAB_LINUX_INITIALIZER=ON` on exact Linux. Initializer without
preinit, either option off Linux, or `HALOFPX_REGISTRY_LAB_LINUX_MUTATION=ON`
remains a configuration error. The general mutation gate stays closed because
normal CAS, recovery, and quarantine are not admitted here.

The target is `STATIC EXCLUDE_FROM_ALL`, owns one uninstalled internal header,
and may be linked only by its excluded qualification executable and archive
audit. It privately links the target-native registry-lab wire/auth lineage. It
has no install, export, fake-engine, publication, server, provider, cache,
model, llama, llama-common, HIP, Vulkan, RPC, WebUI, or product edge.

ADR-0025 remains unchanged. Its `qualify_once()` requires fd 4 absent and
returns no credential, dirfd, lock, callback, or reusable mutation authority;
the initializer neither calls nor links it. The initializer owns a reviewed
behavioral port of its narrow Linux primitive algorithms with parity tests. A
future common-code refactor is a separate milestone and must requalify L05s.

## Controller-pinned authority and credential ordering

The controller creates a new, fully allocated 1 GiB loopback Btrfs filesystem,
an empty candidate root, and the separate ADR-0025 non-authority fixture. It
pins parent, root, fixture, mount, filesystem/subvolume UUID, owner, modes,
reserve, device, and fixture-lock inode facts. All paths and devices remain
disjoint from sources, evidence, models, deployments, live caches, home, boot,
and known services.

Before any parent, root, or fixture syscall, the child consumes:

1. fd 3, exact identity `/memfd:halofpx-registry-lab-credential (deleted)`,
   containing the ADR-0025 credential package; and
2. fd 4, exact identity `/memfd:halofpx-registry-lab-predecessor (deleted)`,
   containing one canonical authenticated v1 predecessor registry envelope of
   `1..1024` bytes.

Both descriptors independently require regular zero-link shmem inodes, exact
`F_SEAL_SEAL|F_SEAL_SHRINK|F_SEAL_GROW|F_SEAL_WRITE`, exact bounded offset-zero
read plus EOF, `FD_CLOEXEC`, and fail-closed alias scans. They must be distinct
device/inode pairs, and no other descriptor may alias either. The initializer
parses directly from fixed locked storage; it does not call the wire parser that
would create an unmlocked transient credential. Package, predecessor, partial
read, derived-key, and secret scratch are explicitly wiped. Secret storage is
wiped before `munlock`; both memfds close before root access.

The launcher receipt independently pins the predecessor's exact registry-lab
digest, registry ID, epoch, authority-base scope commitment, policy commitment,
high water, key ID/generation, and key-continuity commitment. The initializer
authenticates and compares every field and exact byte before root access.
Authenticity under the supplied key alone is insufficient.

The current registry-lab digest helper is private to the wire implementation.
Implementation must first add a narrow no-I/O
`context_store_registry_lab_registry_envelope_digest_v1()` API with independent
golden-vector coverage. The digest names the immutable predecessor and binds
the initial selector; the protected-registry carrier digest is a different
domain and cannot substitute.

Any fd, seal, alias, parse, authentication, compatibility, or receipt mismatch
closes both descriptors, wipes owned state, and returns with zero root/fixture
access.

## Initializer fencing and pre-mutation admission

After credential admission, the initializer repeats complete ADR-0025 anchored
parent/root/fixture admission and final revalidation. It acquires the separate
fixture's exact whole-file `F_OFD_SETLK` writer lock with the five-second
monotonic deadline and root-keyed same-process guard. This is the stable
cross-process fence before the root has a permanent lock.

While holding the fixture lock, the candidate root must remain the exact empty
mode-0700 directory on the pinned writable Btrfs mount with at least 256 MiB
available and within the 16 MiB logical-authority bound. Identity, layout,
read-only state, and reserve are rechecked immediately before mutation.

Immediately before creating `writer.lock`, the initializer latches
`initialization_discard_required`. It then creates that file no-replace as the
first root mutation, pins its complete identity, synchronizes it, and acquires
and holds its own whole-file OFD writer lock. It revalidates that `writer.lock`
is the root's only entry. Both locks remain held through final validation;
permanent lock cleanup precedes fixture-lock cleanup. No fork, dup, descriptor
export, callback, stale-lock break, path re-resolution, or weaker fallback is
admitted.

Every within-root lookup uses held dirfds, fixed target-owned names, `openat2`,
`O_NOFOLLOW`, and `RESOLVE_BENEATH|RESOLVE_NO_MAGICLINKS|
RESOLVE_NO_SYMLINKS|RESOLVE_NO_XDEV`.

## Generated identities, wire state, and layout

Before root mutation except for fields that depend on the new lock inode, the
initializer uses complete `getrandom(..., 0)` loops to obtain a nonzero 256-bit
lab-root ID and nonzero 128-bit store UUID. `EINTR` retries; missing support,
zero progress, or error fails closed. Time, PID, path, deterministic test bytes,
caller data, and PRNG-library fallback are forbidden on node runs. The public
audit and receipt may retain the complete nonsecret root ID and store UUID.

After the permanent lock is pinned, the accepted wire API constructs and
self-verifies:

- the initializing root marker with CBOR null initial-HEAD field;
- selector generation one choosing the exact predecessor;
- the initialized marker binding the exact initial-HEAD digest; and
- the exact registry-lab predecessor digest.

The initializer's fixed lowercase-hex formatter constructs the immutable
filename from that digest only after exact fd-4 authentication and launcher-
pinned digest comparison. The wire API does not construct filesystem names.

Both markers bind the same root ID, store UUID, registry ID/epoch, path-policy
commitment, filesystem UUID, mount ID, owner UID, key tuple, capacity 512,
permanent lock device/inode, and fixed limits. Encoding occurs only after the
lock device/inode is known. Independent test code must reproduce every final
byte and digest; implementation self-verification is not called independent.

Successful final layout is exactly:

```text
root.marker
writer.lock
HEAD
envelopes/
attempts/
staging/
envelopes/e-<64 lowercase hexadecimal registry-lab digest>.cbor
```

Directories are mode `0700`. Regular authority files are mode `0600`, owned by
the effective UID, regular, link count one, bounded, and on the pinned mount.
`writer.lock` is empty and permanent. `attempts/` and `staging/` are empty.

Initialization alone also admits these fixed transient names:

```text
staging/initialize-root.tmp
staging/initialize-envelope.tmp
staging/initialize-head.tmp
staging/initialize-marker.tmp
```

The normative CDDL layout comment now admits exactly these initialization-only
names; its checker must enforce them before code opens. Any retained transient
name is discard-only and is rejected by normal reopen. No user-derived name,
`O_TMPFILE`, `linkat`, arbitrary temporary name, or fallback is admitted.

## Exact initialization sequence

Each primitive revalidates its dirfd, mount, expected layout, identity, and
reserve before mutation. Exact mode is verified after every create; process
umask may only remove permission bits and is corrected with fd-based `fchmod`
before publication.

1. create `writer.lock` with `openat2`, `O_CREAT|O_EXCL|O_RDWR|O_CLOEXEC|
   O_NOFOLLOW`, mode `0600`; pin, `fchmod`, validate, `fsync`, and OFD-lock it;
2. `mkdirat` no-replace `envelopes`, `attempts`, and `staging` mode `0700`;
   anchored-open, `fchmod`, validate, and `fsync` each, then `fsync` root;
3. create `staging/initialize-root.tmp` no-replace, offset-zero bounded-write
   the initializing marker, exact readback plus EOF, authenticate, `fsync`,
   rename no-replace to `root.marker`, then sync root and staging;
4. create `staging/initialize-envelope.tmp` no-replace, offset-zero bounded-
   write exact fd-4 bytes, exact readback/auth/digest, `fsync`, rename no-replace
   to the digest-named `envelopes` object, then sync envelopes and staging;
5. create `staging/initialize-head.tmp` no-replace, offset-zero bounded-write
   the initial selector, exact readback/authentication plus selected-predecessor
   reopen, `fsync`, rename no-replace to `HEAD`, then sync root and staging;
6. create `staging/initialize-marker.tmp` no-replace, offset-zero bounded-write
   the initialized marker, exact readback/authentication, and `fsync`;
7. re-open and authenticate the exact initializing marker and staging marker,
   atomically `renameat2(staging_fd, "initialize-marker.tmp", root_fd,
   "root.marker", 0)`, then sync root and staging; and
8. anchored-reopen and validate the complete final layout, initialized marker,
   `HEAD`, exact predecessor, empty attempts/staging, permanent lock, all
   identities, limits, and absence of every transient name.

All visible creates are no-replace. Initial publications use
`renameat2(RENAME_NOREPLACE)` with no emulation. The final marker close is the
only replacing rename. Writes use fixed-capacity complete `pwrite` loops with
explicit offset/overflow accounting, `EINTR` retry, and zero-progress failure;
the create is exclusive and the file is never truncated or reopened writable.
Reads use bounded complete `pread` and exact EOF. Files use `fsync`; directories
use held real directory fds. Each cross-directory rename synchronizes both
destination and staging source directories. `EXDEV`, unsupported rename, or
any disagreement fails closed.

Only after step 8 do scratch/keys wipe, non-lock fds close, secret storage wipe
and unlock, permanent OFD unlock/close, fixture OFD unlock/close, and guard
release occur. A bounded public audit becomes visible after that ordering.

## Discard-only recovery and closed results

Before the latch, results are limited to `invalid_request_no_mutation`,
`unsupported_no_mutation`, `busy_no_mutation`, `unavailable_no_mutation`, and
`io_failure_no_mutation`. A nonempty root observed before this invocation's
mutation receives `preexisting_root_discard_required`: the invocation performs
no new mutation, returns no authority, and the controller must discard that
exact candidate filesystem rather than treat it as a generic invalid request.

From the latch through result visibility, every error, exception, lost
response, signal, process death, controller loss, or missing acknowledgement is
`initialization_discard_required`. A killed process has no ordinary result. A
complete-looking final tree after a lost response remains discard-only.

M63-01b never adopts, repairs, completes, deletes, or retries an interrupted
root. A fresh read-only inspector may inventory bytes for evidence but returns
no authority. Cleanup removes only the whole exact disposable mount, loop,
image, and run directory after identity checks and evidence retention; it never
repairs or deletes individual root entries. A new attempt uses new disposable
media, root, fixture, memfds, credential, predecessor, and identities.

Only a live invocation completing cleanup and returning its audit reports
`initialized_non_authoritative`. The audit contains public identities, phase
ordinals, counts, and equality facts only. It carries no fd, secret, raw
credential, authority witness, absence proof, or object consumable by another
target.

## Qualification and retained evidence

Promotion requires:

- Windows/Linux feature-off controls and configuration-negative tests proving
  the initializer absent and the general mutation gate still fatal;
- exact archive/import/link/install/export/reverse-product isolation audits;
- independent bytes and digests for both markers, initial selector, predecessor
  name/digest, path policy, and final layout;
- fd 3/fd 4 name, seal, length, EOF, distinctness, alias, ordering, malformed,
  wrong-key, wrong-pinned-predecessor, wrong scope/policy/high-water/continuity,
  truncation, append, and single-bit matrices;
- every guard, lock, admission, CSPRNG, encode, create, chmod, mkdir, open,
  revalidation, write, readback, file sync, rename, directory sync, final reopen,
  wipe, close, unlock, and release boundary;
- EINTR, short/zero I/O, ENOSPC, EDQUOT, EIO, EROFS, reserve loss, EXDEV,
  collision, read-only, inode substitution, symlink, hardlink, cross-mount,
  unexpected-name, concurrent initializer, late completion, and lost response;
- exact final production binaries driven by an external `ptrace` controller,
  killed by exact PID at syscall entry and exit for every mutating boundary,
  with `WIFSIGNALED` and `SIGKILL`, OFD release, tree inventory, and discard-only
  classification; no internal production crash hook is admitted;
- a separately hashed test-instrumented library build for deterministic returned-
  fault products at every primitive boundary, never mislabeled as the exact
  production binary;
- at least 100 clean successful initializations per node and promoted build
  class, at least 25 exact-production `SIGKILL` repetitions per mutating crash
  boundary per node, and ASan/UBSan coverage of every deterministic boundary;
- full feature-off, HaloFPX, focused inherited, optimized Linux, and Linux
  ASan/UBSan suites; and
- source/executable hashes, commands, exits, timestamps, kernel/toolchain,
  mount/Btrfs/loop identities, raw ptrace/syscall logs, all retained trees,
  before/after service PID/health, reserve, cleanup receipts, reference-clone
  cleanliness, and final independent milestone review.

Imported executable fixtures remain untrusted unless separately hash-pinned and
promoted. Qualification media is disposable; evidence is outside it.

## Provenance, rollback, and nonclaims

Implementation is target-native. No GPL llama-ai implementation or docs,
CachyLlama code, donor format, direct cherry-pick, or new dependency is
admitted; no P3 record is required. Reference clones remain untouched.

Rollback removes the new default-off initializer routing, excluded target,
tests/audits, and contract changes. Feature-off product graphs and known-good
services remain unchanged, so this milestone makes no inference-performance
claim.

M63-01b proves only bounded disposable-root initialization under process-crash
tests. It does not prove power-loss durability, device flush behavior,
latestness, rollback resistance, production key custody, normal reopen,
quarantine, CAS, cache hits, restore, tenant scope, distributed behavior,
inference, HIP/Vulkan behavior, or zero-regression performance. M63-01c remains
the earliest lane that may propose concrete reopen and sticky quarantine under
a separate accepted ADR.
