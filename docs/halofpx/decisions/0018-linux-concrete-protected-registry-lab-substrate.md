# ADR-0018: Linux concrete protected-registry lab substrate

- Status: accepted for L05o implementation after independent adversarial review; no production or persistence enablement
- Date: 2026-07-18

## Decision and boundary

L05o is the first concrete-storage milestone. It is a standalone, Linux-only,
`EXCLUDE_FROM_ALL` laboratory backend that qualifies authenticated exact-
envelope protected-registry compare-and-advance on an explicitly disposable
root. It returns only a move-only `concrete_registry_lab_observation` whose
name, representation, and factories are private to L05o.

L05o does not accept, convert, wrap, relabel, or inherit any L05m or L05n
synthetic value. It does not return an L05k or L05l authority proof and cannot
be consumed by material, anchor, server, provider, cache-hit, restore, or
inference code. No server option, environment variable, production target, or
automatic root discovery is added. Feature-off behavior remains the control.

The first real qualification host is nimo-2. Its measured host is Linux
7.1.3-1-cachyos with Btrfs UUID
`292d4132-608a-4b9a-9293-46aa8b110134`; `/var/tmp` is subvolume `@tmp` on that
filesystem. The exact read-only capture is retained in
[`l05o-nimo2-readonly-preflight.json`](../evidence/l05o-nimo2-readonly-preflight.json).
This inventory selects a host, not a durability claim. Mutation uses a newly
created, preallocated 1 GiB loopback image, a new disposable Btrfs filesystem,
and a private mount created for one run beneath the admitted scratch parent.
The host Btrfs subvolume is only the backing store. The harness must prove the
loop device, image, mount, and root are new and disjoint from every existing
block device and protected path before formatting or mounting. Windows
may test the serializer, state machine, and fake-system-call harness only. NFS,
network filesystems, overlay filesystems, Windows filesystems, the workspace,
model stores, evidence stores, home, boot, `/`, and any live cache root are
inadmissible. nimo-1 write qualification waits for its own recorded root and
reserve decision.

## Disposable-root admission

The harness must print and retain the resolved host, kernel, process identity,
root path, parent path, mount target, mount ID, filesystem type and UUID,
subvolume, device identities, free bytes, requested reserve, and root marker
before arming. It must reject the root unless all of these are true:

1. an operator created an empty, dedicated directory beneath an explicitly
   allowlisted scratch parent for this run;
2. the canonical resolved root is neither the allowlisted parent itself nor a
   protected path, and contains no workspace, model, source, evidence, home,
   boot, deployment, or live-service data;
3. the root is owned by the effective service user, mode `0700`, on one local
   mount, and its device and mount identity remain unchanged;
4. the 1 GiB image has been completely allocated, the host has at least 64 GiB
   available after allocation, the loop filesystem has at least 256 MiB free
   before initialization and every first mutating syscall, and the fixed
   maximum live authority bytes described below cannot exceed 16 MiB; and
5. the root contains only the exact versioned marker and fixed layout admitted
   below.

The root marker is immutable after initialization and binds a random nonzero
256-bit lab-root identity, format major/minor, store UUID, registry ID and
epoch, filesystem UUID, mount ID, owner UID, key ID/generation, capacity 512,
the permanent lock inode's device/inode identity, the exact initial-HEAD
digest, and the complete path-policy commitment. Initialization and destructive fault
tests are separate explicit commands. A normal request can never initialize,
repair, clear, or delete a root.

The scratch parent and canonical root are absolute Linux path byte strings of
`1..4096` bytes restricted to ASCII `0x21..0x7e`, with no NUL. The canonical
root must equal `realpath` before initialization and must remain reachable by
the already-open parent/root dirfds afterward; string resolution is never
repeated during an operation. The path-policy commitment is exactly:

```text
SHA-256(
  "halofpx.registry-lab-path-policy.v1\0" ||
  uint64be(parent length) || exact parent bytes ||
  uint64be(root length) || exact root bytes ||
  filesystem UUID[16] || subvolume UUID[16] ||
  uint64be(mount ID) || uint64be(st_dev) || uint64be(owner UID) ||
  uint32be(root mode 448, octal 0700) ||
  uint32be(authority-file mode 384, octal 0600) ||
  uint64be(attempt capacity 512) ||
  uint64be(maximum logical authority bytes 16777216) ||
  uint64be(loop-image bytes 1073741824) ||
  uint64be(required host reserve bytes 68719476736) ||
  uint64be(required loop free bytes 268435456))
```

Every domain in L05o includes exactly the one shown NUL byte. The two UUIDs
are decoded 16-byte values, never textual spellings. Integer encoding here is
unsigned big-endian with the stated fixed width.

Initialization is a distinct test-administration operation. It accepts one
exact authenticated v1 predecessor registry envelope through sealed memfd 4,
with the same seal, one-copy, startup, and `FD_CLOEXEC` requirements as fd 3
and an exact length of `1..1024`. No request or path supplies it. Initialization
verifies it under the already admitted test credential, creates the three fixed
directories and permanent empty lock inode with no replacement, validates the
lock's owner/mode/type/link/device/inode, and synchronizes each new directory
and the root. It can then encode the lock-bound initializing marker, write and
read back that authenticated marker and the immutable predecessor, synchronize
both files and their directories, write/read-back/synchronize the initial
authenticated selector, publish it as `HEAD`, synchronize the root, then write and synchronize an
authenticated initialization-close field inside the marker before reopening
the complete root. The marker initially encodes `initializing`; the only
admitted replacement encodes `initialized` and the exact initial-HEAD digest.
It succeeds only on the still-empty newly admitted directory. It returns no
registry authority or reusable absence result and can never run again for that
root. Initialization faults leave the root inadmissible; recovery consists only
of retaining the test evidence and discarding that exact disposable root. The
first initialization syscall that creates or changes a file is its ambiguity
boundary; no later error may classify the root as untouched.

The backend opens the admitted root once and thereafter uses only dirfd-
relative fixed names. Linux `openat2` resolution requires `RESOLVE_BENEATH`,
`RESOLVE_NO_SYMLINKS`, `RESOLVE_NO_MAGICLINKS`, and `RESOLVE_NO_XDEV`; where a
kernel lacks any required guarantee, L05o is unsupported rather than weakened.
Every opened authority file also requires `O_NOFOLLOW`, a regular file, owner
UID match, exact mode `0600`, link count one, bounded length, and the expected
device/mount identity. Absolute names, `..`, empty components, user-derived
filenames, symlinks, hardlinks, unexpected entries, mount substitution, and
cross-device publication fail closed before mutation.

## Credential boundary

The registry authentication key is purpose-specific and arrives only through
sealed Linux `memfd` descriptor 3. The launcher creates the descriptor with
`MFD_ALLOW_SEALING`, writes one credential package, applies
`F_SEAL_SEAL|F_SEAL_SHRINK|F_SEAL_GROW|F_SEAL_WRITE`, duplicates it to fd 3 for
the exec without `FD_CLOEXEC`, and closes every other copy. Before threads or
filesystem access, the child sets `FD_CLOEXEC`, proves the object is a memfd,
proves all four seals and exact length, parses it once, copies the 32-byte
secret into successfully `mlock`ed memory, and closes fd 3. Failure of any
step, including `mlock`, is unsupported with no root access. No systemd,
path-based, environment, interactive, or fallback credential channel is
admitted by L05o.

Fd 4 is forbidden during normal reopen/CAS/recovery and is consumed only by the
one initialization command. Both fds must be absent after startup admission.

The credential package is exactly: 16 bytes `HaloFPXRegKey01\0`, unsigned
16-bit big-endian registered-ASCII key-ID length `1..128`, exact key-ID bytes,
unsigned 64-bit big-endian nonzero generation, unsigned 16-bit big-endian
secret length fixed to 32, then exactly 32 secret bytes. Trailing bytes reject.
A request cannot supply key bytes, a key path, descriptor number, key ID, or
generation. The secret is never written to the registry root,
environment, command line, receipt, log, crash record, or evidence manifest.
For L05o the package must contain the same admitted registry-authentication
tuple and 32-byte master that authenticate the embedded ADR-0013 predecessor
and ADR-0014 successor. L05o never uses that master directly for its outer
objects: it derives `K_lab` under the distinct normative L05o KDF domain. A
different tuple/master, even one producing otherwise valid outer records,
rejects before root mutation.

The golden vector's repeated-byte master is intentionally public synthetic
format material and is forbidden as a node/run credential. Node qualification
generates a fresh ephemeral test master, derives matching inner envelopes, and
retains only nonsecret hashes and key tuple. No captured credential-package
bytes from a node run may enter evidence.
The authenticated root marker is the authoritative expected key ID/generation
after initialization. On reopen they must equal the sealed package. Secret and
derived-key storage is wiped before unlock and destruction on every path.

L05o tests use a fresh run-specific test credential. This proves only that the
backend consumes the declared channel. Production issuance, principal
authentication, rotation, revocation freshness, escrow, recovery, and key
destruction remain closed.

## Cross-process writer authority

Each root has one permanent, never-replaced lock inode created only during
initialization. Mutation and recovery require `F_OFD_SETLK` with a whole-file
write lock. Acquisition retries only `EINTR`; `EAGAIN` or `EACCES` is retried
every 10 ms against `CLOCK_MONOTONIC` for at most five seconds, then returns
`busy_no_mutation`. Every other error is unsupported/no mutation. The opened
lock fd is `O_CLOEXEC`; the process may not fork after it is opened. A process-
local non-reentrant guard is acquired before the OFD operation and rejects
same-process recursion before filesystem mutation. No dup, descriptor export,
or callback while locked is admitted.

The exclusive lock is held from preflight revalidation through terminal-record
directory synchronization. Lock-file contents are empty and confer no
authority. There is no stale-lock break, PID takeover, or lock deletion. Kernel
release after process death requires a new holder to run recovery before any
new operation. This coordinates only compliant processes running as the
admitted UID; a malicious same-UID process that bypasses the protocol is outside
L05o's claim and blocks production promotion.

## Frozen persistent layout and records

The normative wire grammar is
[`context-store-registry-lab-v1.cddl`](../contracts/context-store-registry-lab-v1.cddl).
The independent checker and fixed vector named by the qualification record must
reproduce every envelope, domain, KDF input, authentication input, digest, and
tag before this ADR can become accepted.

The root contains exactly `root.marker`, the empty permanent `writer.lock`,
`HEAD`, optional `QUARANTINE`, and directories `envelopes`, `attempts`, and
`staging`. Immutable registry envelopes are named
`envelopes/e-<64 lowercase hexadecimal envelope digest>.cbor`. For slots
`000..511`, admitted journal names are `attempts/<slot>.prepare` and exactly
one of `<slot>.close` or `<slot>.abort`; admitted retained staging names are
`staging/<slot>.successor.tmp`, `staging/<slot>.head.tmp`, and the single
root-wide `staging/QUARANTINE.tmp`. No other name, case, width, character, file type, or
nested directory is valid. A slot filename comes only from the backend's
bounded integer formatter, never from caller text.

`HEAD` is an authenticated selector, never a registry envelope. It binds the
lab-root identity, exact selected immutable-envelope digest, selected envelope
length, registry high-water, key tuple, and selector generation. Every CAS and
recovery first authenticates `HEAD`, resolves that one exact immutable filename,
authenticates and bounds the complete envelope, re-derives its digest, and only
then requires selector registry ID, epoch, high-water, key ID/generation,
declared length, and digest to equal the authenticated resolved envelope before
byte-comparing it with an expected envelope. There is no directory scan for
"latest" state. Unknown, duplicate, oversized, malformed, or extra authority
material quarantines the root. Unreachable immutable envelopes and staging
files remain retained for later administrative accounting; request processing
never deletes them.

`root.marker`, `HEAD`, terminal records, and `QUARANTINE` are at most 1,024
bytes; a `PREPARE` is at most 4,096 bytes; each registry envelope is at most
1,024 bytes. Across all 512 slots, immutable envelopes, selectors, journal,
and retained staging, the backend refuses any mutation that could make regular-
file logical bytes exceed 16 MiB. It computes the remaining worst-case bytes
before the first mutating syscall and repeats `statvfs` reserve admission there.
External space consumption after that check is treated as I/O uncertainty, not
as proof the host reserve survived.

Each attempt ID is a nonzero 256-bit identity and may occupy one slot exactly
once. Slots never wrap, evict, overwrite, compact, or reuse. Capacity plus one
returns `capacity_exhausted_no_mutation`. Each slot contains one immutable
authenticated, deterministic, length-delimited `PREPARE` record and exactly
one authenticated terminal `CLOSE` or `ABORT` record. Records bind:

- lab-root identity and path-policy commitment;
- attempt ID, slot, record kind, and key ID/generation;
- complete exact predecessor and successor envelopes and their domain-
  separated digests;
- the L05o operation commitment;
- recorded phase and terminal classification; and
- previous-record digest where applicable.

Every persisted object except the intentionally empty lock inode has a closed
version, explicit lengths, SHA-256 content identity, and purpose-separated
HMAC-SHA-256 authentication. `root.marker` is authenticated before any of its
identity, mount, layout, or key fields are trusted. Noncanonical encoding,
unknown/duplicate/reordered fields, truncation, trailing data, wrong domain,
wrong key, invalid phase, or inconsistent chain quarantines the root. Exact
wire schemas, domains including their single NUL terminators, and independent
golden vectors must be frozen before implementation.

## Exact compare-and-advance

While holding the root lock, the backend revalidates the root, credential
tuple, marker, journal, and authenticated selector/resolved envelope. It
accepts only complete expected-predecessor and proposed-successor envelopes and
independently authenticates and byte-compares both. Parsed-field or digest
equality never substitutes for full canonical bytes.

The transition is closed to the ADR-0014 v1 predecessor and v2 successor. The
backend independently requires unchanged registry ID/epoch, public authority-
base scope, registry policy commitment, authentication key ID/generation, and
stable key-continuity commitment; successor high-water exactly predecessor
`H + 1` without overflow; successor predecessor-envelope digest exactly the
resolved current envelope; receipt sequence equal to the new high-water; and
the complete nonzero command ID, token digest, plan commitment, selected-
manifest digest, proposed-anchor digest, and operation commitment to recompute
exactly under ADR-0014. Any decrease, skip, fork, different-domain field,
receipt mismatch, altered continuity, or merely valid-key arbitrary envelope is
`invalid_transition_no_mutation`.

The operation order is:

1. create the unique `PREPARE` with no replacement, read it back, synchronize
   the file, and synchronize its attempt directory;
2. authenticate `HEAD`, resolve its one immutable envelope, and byte-compare
   that complete envelope with the exact predecessor;
3. write the successor to unique staging with bounded complete-write loops,
   read back and reauthenticate every byte, then synchronize the file;
4. publish the immutable successor using `renameat2(RENAME_NOREPLACE)` on the
   same filesystem, and synchronize its directory;
5. write a unique temporary selector containing the exact successor identity,
   read back and authenticate it, and synchronize the temporary file;
6. atomically rename the selector over `HEAD`, which is the linearization
   point, then synchronize the root directory;
7. re-open and authenticate `HEAD` and the exact selected successor;
8. create, read back, synchronize, and directory-synchronize `CLOSE`; and
9. only then return `advanced_process_crash_closed_lab_observation`.

An already-present successor or selector resolving the exact successor does
not attribute the transition to the current attempt and returns no positive
observation. A
definite predecessor mismatch before selector replacement is a terminal
`ABORT`. Any error, exception, timeout, short I/O, allocation failure, signal,
or lost response after the first mutating syscall, including partial creation
or writing of `PREPARE`, is uncertain until recovery. No
positive result is admitted from a response-supplied phase or filesystem
presence alone.

## Restart recovery and quarantine

Every lock acquisition after initialization begins with bounded recovery.
Recovery validates the initialized marker, `HEAD` selector, its resolved
envelope, all occupied attempt slots, and every referenced immutable envelope.
It never chooses state by timestamp, generation maximum, directory order, or
filename enumeration.

For one valid `PREPARE` without a terminal record:

- selector resolves an exact authenticated envelope byte-equal to successor:
  repeat required file and directory synchronization, durably write `CLOSE`,
  and return `recovered_successor_process_crash_closed_lab_observation`;
- selector resolves an exact authenticated envelope byte-equal to predecessor:
  durably write `ABORT` and return
  `recovered_not_applied_no_authority`; or
- absent, unreadable, malformed, unauthenticated, contradictory, or any other
  selector/resolved envelope: persist sticky quarantine if possible and return
  no authority.

Persistent quarantine is an authenticated object binding the lab-root ID,
public reason code, optional slot/attempt, last authentic selector digest if
available, observed phase, and key tuple. While locked, the backend writes the
fixed root-wide quarantine staging name with no replacement, reads it back,
synchronizes it, renames it to `QUARANTINE` with no replacement, and
synchronizes the root. Presence of `QUARANTINE` blocks mutation even when its
contents are malformed. Presence of a quarantine staging file, a partial
quarantine write, or failure to prove its directory synchronization returns
`quarantined_or_unavailable`; every future open also blocks because that
retained name is admitted only as unresolved quarantine state.

Multiple unresolved prepares, contradictory terminal records, missing
referents, or an unprovable synchronization boundary also produce
`quarantined_or_unavailable`. Quarantine survives process restart and blocks
every mutation. L05o has no repair, rollback, quarantine-clear, import, or
garbage-collection API.

## Status and failure contract

Statuses are closed and separate definite no-mutation, definite terminal,
uncertain, quarantined, busy, unsupported, resource-exhausted, and positive
lab-observation results. Errors expose no secret, raw path, predecessor/successor
existence across an authority boundary, or reusable absence result. Logging is
bounded to operation ID, phase, public reason code, and truncated public
identifiers. No prompt, token, principal, secret, full hash, or model content
is logged.

ENOSPC, EDQUOT, reserve exhaustion, read-only state, EIO, partial I/O,
interruption, and synchronization failure never advance a response to success.
An unconfirmed post-`PREPARE` outcome remains sticky uncertain/quarantined;
inference is unaffected because L05o has no runtime link.

## Rollback and threat limit

For this lab milestone rollback means stopping the standalone tool, retaining
its evidence, and discarding only the preflight-identified disposable root.
The schema is never shared with an old binary. No existing deployment, model,
cache, configuration, or reference clone is changed.

The filesystem design can detect ordinary inconsistent or partial rollback
inside the admitted root. It cannot prove freshness if an attacker or rollback
mechanism restores the entire registry root together with its credential and
configuration. A production rollback-resistance claim requires an external
monotonic trust anchor such as a qualified TPM counter, remote authority, or
operator-pinned head, or a separately accepted narrower threat model. L05o
must state this limit in every result and may not be called rollback-resistant.

## Required qualification before promotion

Before code, independent adversarial review must accept the threat/failure
model, frozen layout and golden vectors, credential contract, OFD-lock
semantics, exact CAS/linearization/synchronization order, restart recovery,
quarantine, reserve policy, disposable-root preflight, old-binary exclusion,
and rollback limit.

Local Windows qualification covers independent serialization/commitment
oracles; every-field/domain/NUL/order/length mutation; a fake-system-call fault
at every before/after boundary; replay, capacity, races, and exception paths;
static exclusion and forbidden synthetic conversions; feature-off contract;
and inherited regressions. It makes no filesystem-durability claim.

nimo-2 qualification uses a newly identified disposable root and retains raw
preflight, build, command, executable-hash, filesystem, journal, kernel, and
cleanup evidence. It requires:

- a clean CPU build and focused/inherited controls;
- multi-process exactly-one compare-and-advance and bounded lock contention;
- lock-holder `SIGKILL`, process restart, descriptor inheritance, and
  re-entrancy cases;
- deterministic `SIGKILL` before and after every prepare, write, readback,
  file-sync, no-replace rename, directory-sync, selector-replace, reread, and
  terminal-close boundary;
- wrong, missing, revoked, replaced, and malformed test credentials;
- symlink, hardlink, traversal, extra-entry, owner/mode, mount, and root-marker
  substitution attacks;
- truncation, bit corruption, duplicate/trailing bytes, missing referents,
  replay, full 512-slot history, and capacity plus one; and
- bounded ENOSPC/EDQUOT, read-only, short-I/O, EIO, and synchronization faults
  using only a preflight-verified loopback, qgroup, or device-mapper disposable
  tier that cannot target host, source, model, deployment, or evidence data.

M63-01 process-crash evidence may be promoted after these tests and independent
review. No turn-durable or strict label is admitted until M63-03 qualifies the
exact filesystem, kernel, mount, SSD, firmware, controller, flush path, and
declared power-loss model. nimo-1 and reboot/power-loss tests require separate
preflight receipts and rollback preparation.

## Consequences and next gate

After the normative CDDL/golden artifacts and this ADR pass independent review,
there is no known blocker to implementing and process-crash qualifying L05o on
a disposable nimo-2 loopback target. This is not a durability-mode claim.
Production enablement
remains blocked by production credential custody and rotation, an approved
persistent root/reserve/quota policy, principal and administrative scope,
whole-domain rollback authority, and exact device power-loss qualification.

After L05o, a separate accepted milestone may consume a new concrete registry
proof to qualify the bounded bootstrap-material writer. Material and protected-
anchor operations, server/provider wiring, cache admission, restore, and
persistent feature enablement remain prohibited until their own gates pass.
