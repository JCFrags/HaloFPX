# ADR-0025: Linux registry-lab pre-initialization primitives

- Status: accepted for L05s/M63-01a implementation; registry mutation remains closed
- Date: 2026-07-19

## Decision and boundary

The first concrete Linux lane after L05r is L05s/M63-01a: a Linux-only
pre-initialization primitive provider. It qualifies the credential channel,
candidate-root identity checks, `openat2` containment, and OFD locking required
by ADR-0018 without initializing or mutating a registry.

The target is named
`halofpx-context-store-registry-lab-linux-preinit`. It exists only when
`CMAKE_SYSTEM_NAME` is exactly `Linux` and
`HALOFPX_REGISTRY_LAB_LINUX_PREINIT=ON`. The option defaults off, the target is
`STATIC EXCLUDE_FROM_ALL`, its header is internal and uninstalled, and only
focused Linux tests may link it. A separate
`HALOFPX_REGISTRY_LAB_LINUX_MUTATION` option also defaults off; enabling it in
this milestone is a configuration error because no mutation source is admitted.

This lane has no public API, server/provider/cache/product edge, environment or
command-line runtime option, automatic path discovery, fake-to-concrete
conversion, `concrete_registry_lab_observation`, registry proof, reusable
absence result, or persistent-write authority.

## Internal API and one-shot ordering gate

The target owns one uninstalled `noexcept` orchestration entrypoint. Its fixed,
bounded request carries independently pinned expected public identities from
the launcher receipt: canonical allowlisted parent, candidate root, and fixture
paths; filesystem and subvolume UUIDs; mount ID; `st_dev`; UID; fixture device
and inode; and public test key ID/generation. Candidate root and fixture are
canonical-disjoint and neither may be an ancestor of the other.

One private process-scoped one-shot session enforces credential admission
before any root or fixture syscall. Failure before that gate proves zero
root/fixture access. The returned audit contains only public comparisons,
counts, and ordering facts. It contains no fd, secret, credential, path handle,
`preflight_context_v1`, fake value, or object reusable by a mutation target.

The target depends only on selected-base SHA code where hashing is needed and
Linux/libc primitives. It neither includes nor links the L05r fake, registry
wire authority, material, anchor, server, cache, restore, or inference targets.

## Credential primitive

Credential admission consumes only fixed descriptor 3 before root access and
requires descriptor 4 to be absent. Descriptor 3 must pass `fstat` as a regular
zero-link inode, `fstatfs` as shmem/tmpfs, exact bounded
`/proc/self/fd/3` identity `/memfd:halofpx-registry-lab-credential (deleted)`,
and exact `F_GET_SEALS` equality with
`F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE`. A bounded child-fd
scan must find no alias to the same device/inode. The launcher separately proves
that it closed all parent copies before exec; the provider does not claim it
can detect a duplicate retained by another process.

The package is the ADR-0018 credential grammar with printable registered-ASCII
key ID, nonzero generation, and exactly 32 secret bytes. Exact size, offset-zero
`pread`, complete bounded read, and EOF are required; trailing bytes reject.

The child sets `FD_CLOEXEC`, successfully `mlock`s a fixed non-relocating owner
before copying the secret, and closes descriptor 3 on every path. The complete
package scratch, partial read state, secret owner, and every derived copy are
explicitly wiped on success and error; ordering is scratch wipe, owner wipe,
`munlock`, then any result visibility. It returns only public audit facts and
the independently expected key-tuple comparison; the tuple is not authority.

Malformed package, seals, fd 4, or same-process alias is `invalid_request` with
no root access. Missing required kernel support or failed `mlock` is
`unsupported` with no root access. Unexpected descriptor/read/syscall failure
is `io_failure` with no root access. Qualification uses a child process whose
launcher creates, seals, duplicates, and relinquishes the memfd exactly as
ADR-0018 requires.

## Read-only candidate-root primitive

The provider may inspect only an explicitly pinned ASCII immediate-child root
under an exact launcher-provided allowlisted parent inside a new 1 GiB loopback
Btrfs mount. The controller's independent preflight opens and closes the parent
before launching the provider child and pins its public identity in the
receipt. The provider receives no inherited parent/root/fixture dirfd and makes
zero parent/root/fixture access until its credential gate succeeds. The root,
fixture, parent, backing image,
mount, loop device, workspace, source, evidence, models, home, boot,
deployments, live caches, services, and existing block devices must be proven
disjoint before provider execution. Protected paths and every descendant are
inadmissible. Evidence remains outside the disposable filesystem.

After credential admission the provider opens the parent, compares it with the
pinned receipt, verifies the request has the exact lexical immediate-child
relation, then opens the root through the parent dirfd with `openat2`,
`O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW`, mode zero, and
`RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS | RESOLVE_NO_MAGICLINKS |
RESOLVE_NO_XDEV`. It never resolves the root string again. Fd-based checks
prove canonical identity, UID, mode `0700`, device, `statx` mount ID, the
parent/mount Btrfs filesystem UUID and subvolume UUID, equality to the same
mount/device, and bounded empty `getdents64` completion. `ST_RDONLY` or
mountinfo read-only state rejects without probing writes; absence of those
flags is recorded only as "not reported read-only," never as proof of
writability.

Every observation must exactly equal the independently pinned request. The
provider creates, writes, synchronizes, renames, removes, or repairs nothing.

The qualification controller retains proof that the 1 GiB image was newly and
fully allocated and that at least 64 GiB host reserve remained afterward. The
provider independently verifies at least 256 MiB candidate-filesystem reserve
and reports the fixed 16 MiB future logical-authority bound. Success remains a
non-authoritative observation; it does not call the root initialized, durable,
fresh, or safe for mutation.

## `openat2` and OFD-lock fixture primitives

Containment and locking are qualified on a separate, explicitly non-authority
fixture directory and inode precreated before any contender opens the lock.
The fixture directory is opened with the exact root `openat2` flags and admitted
through the same anchored parent, protected-path, UID/mode, Btrfs, and
mount/device checks as the candidate root. Its exact layout is the single
`primitive.lock` entry. The expected lock device and inode are pinned before
execution, so replacement by a different otherwise-correct inode rejects.

The provider opens only `primitive.lock` relative to the fixture dirfd with
`openat2`, `O_RDWR | O_CLOEXEC | O_NOFOLLOW`, mode zero, no create/truncate,
and all four required resolution flags. The inode must be a regular file, owner
UID match, mode `0600`, link count one, length zero, and exactly match the
pinned device, inode, fixture device, and mount identity.

The provider acquires a process-local root-specific non-reentrant guard before
the OFD operation. It submits an exact whole-file write lock (`l_whence =
SEEK_SET`, `l_start = 0`, `l_len = 0`) through `F_OFD_SETLK`; retries `EINTR`
immediately; retries `EAGAIN`/`EACCES` every 10 ms against `CLOCK_MONOTONIC` for
at most five seconds; then returns `busy`. It never breaks/deletes a lock,
forks, duplicates/exports the fd, or invokes a callback while held.

Credential and package state are wiped before lock close/result visibility;
fds close before guard release. The provider never writes lock-file contents.

Launchers fork all contenders before any lock opens. Tests prove same-process
non-reentrancy, cross-process exactly-one ownership, bounded busy behavior,
kernel release after `SIGKILL`, `FD_CLOEXEC`, and reacquisition. The negative
inherited-alias case must time out and never stale-break; the positive death
case proves no alias survived. Symlink, hardlink, wrong mode, nonempty file,
traversal, cross-mount, and replacement attacks reject before lock acquisition.

## Closed statuses and promotion tests

The internal result vocabulary is limited to `ok_non_authoritative`,
`invalid_request`, `unsupported`, `busy`, `unavailable`, and `io_failure`.
No result encodes registry state or positive storage authority.

Root, fixture, and lock mapping is closed:

| observation | status |
|---|---|
| malformed/noncanonical/protected/overlapping path, hostile layout or type, or pinned identity mismatch | `invalid_request` |
| missing `openat2`/`statx`/Btrfs/OFD kernel capability, `ENOSYS`, capability-specific `EINVAL`, or unsupported filesystem | `unsupported` |
| reserve below the fixed floor or mount/statvfs reported read-only | `unavailable` |
| `EAGAIN`/`EACCES` through the complete five-second OFD deadline or process-local reentrancy | `busy` |
| unexpected open/read/stat/ioctl/clock/fcntl failure | `io_failure` |
| any post-open identity replacement, race, or internal disagreement | `invalid_request` after closing all fds and exposing no positive audit |

There is no fallback from missing/unsupported `openat2`, Btrfs identity, or OFD
semantics to `openat`, classic `F_SETLK`, `flock`, path-based lookup, or a
weaker resolve mask.

Promotion requires:

- feature-off configuration proving both options off and the target absent;
- compile-gate rejection when mutation is requested;
- Linux optimized plus ASan/UBSan builds, full HaloFPX/inherited controls, and
  focused tests;
- sealed/malformed/wrong-fd credential matrices with wipe and fd-closure audit;
- canonical/ownership/mode/empty-layout/mount/Btrfs/reserve root matrices;
- `openat2` hostile-path, fixture-layout, and inode-replacement matrices;
- OFD contention, timeout, `SIGKILL`, inherited-alias, exec-inheritance, and
  reacquisition matrices;
- before/after tree, name, byte, device, inode, and mount equality proving the
  provider performed no create/write/sync/rename/unlink;
- source/link/archive audits proving no product, fake-engine, wire-authority,
  donor, material, anchor, cache, restore, inference, or write edge;
- immutable reference-clone recheck; and
- final independent adversarial review.

The retained node evidence includes exact source/tree/executable hashes,
toolchain/kernel, mountinfo, loop/image/device identities, filesystem and
subvolume UUIDs, UID/modes/reserve, commands/exits/timestamps, per-case PIDs,
before/after path/inode facts, raw-log hashes, and known-service PID/health
before and after. The cleanup receipt resolves and removes only the exact
preflight-identified non-authority image, mount, root, fixture, and loop device.
It never removes evidence.

Qualification requires at least 100 fresh-process credential/root probes and
100 contention/death pairs in optimized and ASan/UBSan builds unless a reviewed
receipt justifies a stricter count. Raw syscall traces or equivalent retained
evidence must prove the provider made no mutating or synchronization syscall.

Windows runs the feature-off/static contract only; the Linux target and tests
must not exist there. nimo qualification must not stop, restart, or alter the
known-good inference services.

## Nonclaims and next gates

L05s does not satisfy M63-01, initialize a registry, authenticate marker/HEAD,
execute or project fake operations 1-76 through syscalls, issue a concrete
operation-69 CSPRNG identity, persist a quarantine, test a process crash at a
publication boundary, or establish filesystem/device durability. It makes no
cache, inference, distributed, performance, or rollback-resistance claim.

ENOSPC, EDQUOT, EIO, EROFS publication, short regular-file I/O, file/directory
sync, initialization crash, quarantine publication, and normal CAS are
explicitly unexercised. They are not primitive-lane failures and cannot be
claimed from injected fake coverage.

After this lane passes, M63-01b may implement initialization only, with
discard-only recovery. M63-01c may map concrete reopen and sticky quarantine;
M63-01d may add normal compare-and-advance and recovery. Each remains a
separate default-off decision and qualification gate. Provider linkage and
persistent enablement remain later gates; only M63-03 can support a declared
device/power-loss durability label.
