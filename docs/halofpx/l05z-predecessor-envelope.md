# L05z discard-only predecessor-envelope publication

**Status: ACCEPTED AND FROZEN FOR IMPLEMENTATION.** This document fixes the
implementation contract for only step 4 of
[ADR-0026](decisions/0026-linux-registry-lab-initialization-discard-only.md).
It makes no implementation, build, test, crash-qualification, durability, or
promotion claim.

L05z extends the L05y initializing-root-marker boundary by publishing only the
already authenticated predecessor envelope. It does not publish `HEAD`, change
`root.marker`, complete initialization, make the envelope reachable, admit a
provider, enable persistent writes, or create cache-hit, restore, server, or
inference behavior. Every invocation that reaches the inherited mutation latch
remains `initialization_discard_required`, including a fully executed step 4.

The feature remains Linux-only, default-off, `STATIC EXCLUDE_FROM_ALL`,
uninstalled, unexported, and absent from product targets. The general registry
mutation gate remains closed. L05w, L05x, and L05y remain exact compatibility
controls and must leave every L05z audit fact false.

## Accepted envelope identity

The published object is the exact byte sequence received on fd 4 and already
admitted by L05v. It is never decoded and re-encoded for publication. Its
length is bounded to `1..1024` bytes by the accepted
[registry-lab CDDL](contracts/context-store-registry-lab-v1.cddl).

The object name is derived only as follows:

```text
digest = SHA-256(
  "halofpx.registry-lab-registry-envelope.v1\0" || exact_fd4_bytes
)
basename = "e-" || lowercase_hex_64(digest) || ".cbor"
path = "envelopes/" || basename
```

The basename is exactly 71 non-NUL bytes and the final relative suffix
`/envelopes/e-<64-lowercase-hex>.cbor` is exactly 82 non-NUL bytes. The fixed
path-length policy counts bytes excluding the terminating NUL. With a 4096-byte
maximum admitted path, a candidate-root length of 4014 is admitted and 4015 is
rejected. Path construction uses 4097 bytes of storage so the maximum admitted
4096-byte path and its NUL terminator fit without weakening the policy.

The registry-lab envelope digest above is distinct from the protected-registry
carrier digest. The carrier digest cannot name this object. Digest success is
not authentication.

The original fd 4 bytes, a dedicated bounded envelope-readback buffer, and the
placement-owned registry-lab credential remain inside the existing successfully
`mlock`ed `secure_inputs` mapping. No raw envelope, credential, witness, fd, or
reusable authority enters an audit object, ordinary returned carrier, log,
exception, persistent heap object, or unwiped scratch. The accepted target-native
digest and authentication helpers use bounded stack scratch that transiently
contains copied authenticated bytes and explicitly wipes that scratch before
return; L05z adds no separate unbounded or returned copy. L05z must use the facts-only
`context_store_verify_protected_registry_facts_v1` verifier, then independently
require exact length, exact EOF, byte equality, registry-lab digest equality,
and equality of every L05v launcher pin. It must not use the positive
`context_store_verify_protected_registry_v1` carrier because that carrier owns
a copy of the envelope outside the locked input mapping.

The required launcher comparisons include registry ID, nonzero registry epoch,
authority-base scope commitment, policy commitment, consumed high-water mark,
key ID, key generation, and key-continuity facts. The credential remains alive
through final envelope validation and closure of every non-lock L05z descriptor,
then is destroyed exactly once before whole-mapping wipe, zero verification,
`munlock`, and `munmap`.

## Admission before the first step-4 mutation

L05z begins only after the exact L05y final barrier. Before opening
`staging/initialize-envelope.tmp`, it must:

1. authenticate the exact fd 4 bytes with the facts-only verifier and match all
   launcher pins, bytes, length, EOF, and registry-lab digest;
2. construct the exact lowercase digest name and complete all component,
   overflow, fixed-capacity, path-length, and logical-byte checks;
3. revalidate the held fixture and writer locks, pinned parent/root identities,
   filesystem and subvolume UUIDs, mount ID, owner, Btrfs type, writable state,
   and reserve;
4. revalidate `root.marker` as the exact L05y regular file by pinned device,
   inode, mount, owner, mode, link count, length, bytes, authentication, and
   content digest;
5. prove the exact five-entry top-level layout: `root.marker`, `writer.lock`,
   `envelopes/`, `attempts/`, and `staging/`; prove all three directories empty
   and prove `HEAD` absent; and
6. close every write-capable `root.marker` descriptor successfully before the
   first step-4 mutation. Close is attempted once and is never retried. A close
   error is sticky discard-required.

Subsequent anchored read-only `root.marker` reopens must match the same pinned
file identity and authenticated bytes. They are not required or claimed to be
the same open file description. Step 4 never chmods, truncates, opens writable,
renames, republishes, or otherwise mutates `root.marker`.

The logical regular-byte preflight is the exact authenticated `root.marker`
length plus the exact predecessor-envelope length; `writer.lock` contributes
zero bytes. Addition is overflow-checked and the inherited 16 MiB maximum is
unchanged. The inherited 256 MiB free-space reserve, filesystem identity,
writable-state, containment, expected-layout, and lock barriers are repeated at
each mutation boundary required by ADR-0026.

## Exact step-4 publication order

All name operations use held directory descriptors and the inherited contained
`openat2` policy. All positive validation compares pinned device/inode/mount and
metadata identity; a separately opened file is never described as the same
open file description.

1. Open and pin held `staging/` and `envelopes/` directory descriptors and
   validate the exact empty pre-publication layout.
2. Exclusively create `staging/initialize-envelope.tmp` with
   `openat2(O_CREAT|O_EXCL|O_RDWR|O_CLOEXEC|O_NOFOLLOW)` and mode `0600`.
3. Pin the temporary identity; validate regular type, link count one, owner,
   device, mount, fixed path, and zero initial length; apply fd-bound mode
   `0600` and revalidate the exact mode and identity.
4. Complete a bounded offset-zero `pwrite` of the exact fd 4 bytes with checked
   offset and overflow accounting. Only bounded `pwrite` and `pread` loops
   retry `EINTR`; zero progress fails. No other syscall retries `EINTR`.
5. Complete bounded offset-zero `pread` and exact EOF into the locked readback
   buffer. Require exact bytes, facts authentication, every launcher pin, and
   registry-lab digest equality.
6. `fsync` the temporary file.
7. Revalidate the complete admission barrier. At this point `staging/` contains
   exactly the one pinned temporary, while `envelopes/` and `attempts/` remain
   empty and the initializing marker remains unchanged.
8. Anchored-open the temporary read-only. Match the pinned temporary identity,
   not an open-file-description identity, and repeat metadata, length, exact
   EOF, bytes, facts, pins, and registry-lab digest checks. No name-based
   operation may intervene between this final validation and publication.
9. Publish only with
   `renameat2(staging_fd, "initialize-envelope.tmp", envelopes_fd,
   digest_basename, RENAME_NOREPLACE)`. There is no emulation and no retry of
   any returned or ambiguous result.
10. Synchronize the held destination `envelopes/` directory first, then the
    held source `staging/` directory. Root-directory synchronization is neither
    required for this cross-directory rename nor an admissible substitute.
11. Anchored-open the final digest-named object read-only. Match the original
    pinned temporary device/inode/mount identity and exact metadata, then
    repeat exact length, EOF, bytes, facts, pins, and digest/name validation
    before setting any positive final audit fact.
12. Reopen `root.marker` read-only and prove its original pinned identity,
    bytes, authentication, and digest. Prove the exact five-entry root layout,
    exactly one expected regular object in `envelopes/`, empty `attempts/` and
    `staging/`, and absent `HEAD`.
13. Close every non-lock descriptor without retry, preserve both held locks
    through the final barrier, and then perform the inherited credential,
    secure-mapping, lock, guard, and signal cleanup order.

Directory validators use exact expected-layout states. L05z does not weaken a
generic "allow nonempty" flag. Extra, duplicate, uppercase, wrong-width,
wrong-digest, symlink, hardlink, directory, cross-mount, or substituted names
fail validation.

## Failure and cleanup authority

Every error, signal, death, lost response, controller failure, late completion,
unexpected result, identity mismatch, close disagreement, or ambiguous
publication after the inherited latch is sticky whole-root discard. A collision
on either the fixed temporary name or final digest name fails even if the
existing object contains identical authenticated bytes. L05z never adopts,
deduplicates, unlinks, truncates, repairs, retries, resumes, or performs
per-entry cleanup. Whole disposable media may be removed only after retained
evidence and identity checks under the inherited cleanup authority.

Fully authenticated, exact launcher-pinned old predecessor bytes can be
accepted in a fresh process because the protected-registry facts establish
integrity, not latestness. Same-process re-entry remains rejected by the
one-shot latch. Neither case authorizes residual-tree adoption.

## Qualification contract

Implementation promotion requires separate hashed controllers outside product
and CTest graphs. The exact production binary must be killed at syscall entry
and exit for these seven mutating syscall families:

1. exclusive temporary creation;
2. temporary `fchmod`;
3. every envelope `pwrite` occurrence;
4. envelope-file `fsync`;
5. no-replace cross-directory `renameat2`;
6. destination `envelopes/` directory `fsync`; and
7. source `staging/` directory `fsync`.

At 25 repetitions per boundary per node, the minimum two-node matrix is
`7 families * 2 entry/exit boundaries * 25 repetitions * 2 nodes = 700`
exact-production `SIGKILL` cells. Every cell must prove the exact killed PID,
`WIFSIGNALED`/`SIGKILL`, residual inventory, released locks, sticky discard
classification, retained raw evidence, and whole-media cleanup. Expected
residual projections include absent or partial temporary, complete temporary,
visible final before both directory synchronizations, and complete final after
both synchronizations; none is adopted.

The returned-fault controller must enumerate every L05z occurrence for opens,
identity and layout revalidations, `fchmod`, bounded `pwrite`, bounded `pread`,
EOF checks, facts authentication, digest/name checks, file and directory
`fsync`, rename, final reopen, every close, and inherited cleanup. It must cover
pre-operation and late completion; first-occurrence retry and successful
completion only for bounded `pread`/`pwrite` `EINTR`; short and zero progress;
truncation; append; bit corruption across semantic, tag, and digest regions;
malformed lengths; duplicate and unexpected CBOR fields; hostile paths and
inode substitution; hardlinks and symlinks; cross-mount changes; identical and
unequal collisions; filename/content mismatch; wrong case and width; replay;
`ENOSPC`, `EDQUOT`, `EIO`, `EROFS`, `EXDEV`, unsupported no-replace rename;
reserve exhaustion; read-only transition; response loss; and cleanup failure.
The controller must prove absence of `unlinkat`, `truncate`, `O_TRUNC`, repair,
or adoption behavior.

Every run or fault cell uses newly prepared, fully allocated 1 GiB Btrfs media
with fresh root, fixture, memfds, credentials, predecessor bytes, and generated
identities. Promotion requires at least 100 clean Release roots per node,
independent target-free reconstruction of exact bytes/digest/name and unchanged
marker/layout, Release and promoted sanitizer lanes, inherited feature-off and
L05w/L05x/L05y compatibility controls, archive/import/link/install/export and
product reverse-edge checks, service recovery, reference-clone cleanliness,
retained raw bundles, and an independent final milestone review. A sanitizer
lane not completed is recorded as a limitation, never inferred as a pass.

### Admitted response-loss profile

`L05Z-RSP-LOSS-FULL-001` is one test-only semantic case outside the frozen
8,612-case returned-fault compatibility authority. It raises the extended
semantic total to 8,613 without changing the compatibility subset. Its identity
is the qualified L05z stdout-audit transaction being suppressed before its
admitted kernel write, followed by an exact fake full-success return, zero
consumer bytes, child and launcher status zero, and external whole-root
discard. Syscall kind, fragment count, and live response length are physical
profile metadata, not semantic identity.

The admitted Linux x86-64 glibc profile is exactly one complete `SYS_write` to
fd 1. Six discovery runs, three on each node, observed this profile and no
`SYS_writev`; response lengths were dynamic and are therefore bounded at
65,536 bytes rather than frozen to one length. The controller pins the pipe's
FIFO device, inode, and mount before fork; admits only the launcher's sole
direct `PTRACE_EVENT_FORK` child after exact executable, argv, and fd-1 checks;
captures the attempted bytes at syscall entry; substitutes harmless `ENOSYS`;
requires the real `-ENOSYS` exit; and replaces it with the exact requested
count. The captured transcript must pass the full qualified phase-13 audit
oracle, while the private consumer pipe must return zero bytes followed by EOF.
The receipt retains only length, profile, count, transcript hash, status, and
boolean proofs. The controller overwrites its owned captured-transcript buffer
after validation. Ordinary parser temporaries created by `exact_audit` are
destroyed normally; this milestone does not claim secure erasure of every
transient allocator copy.

The bounded `writev` decoder is parser infrastructure only and is not a live
admitted transport profile. A live `writev`, second fragment, wrong lineage,
wrong fd, rebound pipe, malformed vector, unexpected exit result, or nonzero
consumer byte fails this qualification. Other fd-1 mechanisms such as
`sendfile`, `splice`, `vmsplice`, `tee`, `copy_file_range`, or `io_uring` are
also out of profile. They may be detected only after bytes reach the private
controller pipe, so this case proves pre-kernel suppression only for the
admitted one-`SYS_write` profile and makes no universal output-suppression
claim.

## Provenance, rollback, and nonclaims

L05z is target-native and requires no donor implementation or documentation,
new dependency, direct cherry-pick, license, attribution, NOTICE, SBOM, or P3
change. Exact source hashes, archive members, imports, callable definitions,
controller hashes, commands, environments, raw results, and before/after
reference identities must be retained. Raw evidence remains separate from the
contract, receipt, decision, and Wiki promotion.

Rollback is source-only: remove the L05z extent, audit fields, publication
logic, focused tests, excluded controllers, and documentation while preserving
the qualified L05y control. Any root that crossed the latch remains discard-only
and is never made adoptable by rollback.

This frozen contract does not establish implementation completion, test or
crash qualification, power-loss or device-cache durability, normal reopen,
repair, quarantine, latestness, rollback resistance, completed initialization,
publication authority, persistent server writes, cache hits, restore, tenant
sharing, rank or distributed authority, single-node fallback, inference
performance, or zero-regression performance. HIP, Vulkan, ROCmFPX,
TurboQuant, ROCmFP4, RPC, WebUI, and L14Q behavior remain unchanged.
