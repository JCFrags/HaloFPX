# L05y discard-only initializing root marker

L05y extends the qualified L05x directory prefix through only step 3 of
ADR-0026: creation and no-replace publication of the authenticated
`initializing` `root.marker`. The marker selects no `HEAD`, and every invocation
that reaches the inherited mutation latch remains
`initialization_discard_required`. This milestone does not complete
initialization and creates no envelope, `HEAD`, attempt, terminal record, cache
hit, restore path, provider edge, or inference behavior.

The feature remains Linux-only, default-off, `EXCLUDE_FROM_ALL`, uninstalled,
unexported, and outside all product targets. The general registry mutation gate
remains closed. The L05w writer-lock and L05x directory-prefix entrypoints are
compatibility controls and leave every L05y audit fact false.

## Locked construction boundary

The secret-bearing wire credential is placement-owned inside the existing
successfully `mlock`ed initializer input mapping. It is never copied to an
ordinary stack, heap, callback, exception, or audit object. Its lifetime begins
only after the fd 3 credential package and fd 4 predecessor have passed the
L05v transport, authentication, and launcher-pin contract. Its lifetime ends
exactly once before whole-mapping zeroization, zero verification, `munlock`,
and `munmap` on every path.

After the inherited L05x final prefix barrier and while both OFD locks remain
held, the initializer:

1. independently derives the path-policy commitment from the pinned parent and
   candidate-root paths, filesystem and subvolume UUIDs, mount ID, root device,
   and owner UID;
2. constructs the typed initializing-root value from the generated root ID and
   store UUID, authenticated registry ID/epoch and key tuple, pinned filesystem
   facts, exact held `writer.lock` device/inode, capacity 512, initializing
   state, and a null initial-HEAD field;
3. admits and encodes the value into fixed-capacity locked scratch; and
4. semantically verifies the complete encoded value and content digest using
   the accepted target-owned wire API.

The predecessor's opaque policy commitment is not the path-policy commitment.
Any derivation, admission, encoding, size, digest, or verification failure
occurs before the marker temporary is opened and leaves `staging/` empty.

## Exact publication sequence

Immediately before the first L05y mutation, the initializer revalidates the
complete L05x prefix, parent/root/fixture identities, both held lock identities
and descriptions, reserve, writable state, fixed path capacity, and emptiness
of `envelopes/`, `attempts/`, and `staging/`. It then performs only this
sequence:

1. anchored-open `staging/` and exclusively create
   `staging/initialize-root.tmp` with
   `openat2(O_CREAT|O_EXCL|O_RDWR|O_CLOEXEC|O_NOFOLLOW)` and mode `0600`;
2. pin the exact file identity, apply fd-bound mode `0600`, and validate regular
   type, link count one, owner, device, mount, fixed name, empty initial length,
   and exact mode;
3. complete a bounded offset-zero `pwrite` with overflow accounting, retrying
   only `EINTR` and rejecting zero progress;
4. complete a bounded offset-zero `pread`, require exact EOF and byte equality,
   and authenticate the readback before `fsync` of the marker file;
5. anchored-reopen the temporary read-only and revalidate identity, bytes,
   length, EOF, and authentication;
6. publish only with
   `renameat2(staging_fd, "initialize-root.tmp", root_fd, "root.marker",
   RENAME_NOREPLACE)`, with no emulation or retry of an ambiguous result;
7. synchronize the destination root directory and then the source staging
   directory; and
8. anchored-reopen and authenticate `root.marker`, prove the exact five-entry
   root layout, and prove empty `staging/` while both OFD locks remain held.

No L05y path unlinks, truncates, repairs, adopts, completes, or retries a
residual tree. A collision, late completion, partial I/O, corruption,
unexpected byte, unsupported rename, cross-device result, identity change,
reserve loss, read-only transition, close failure, signal, process death,
controller loss, or missing acknowledgement leaves the whole disposable root
discard-only.

## Qualification contract

Promotion requires inherited Windows/Linux feature-off controls, exact archive
and product-graph isolation, focused L05w/L05x compatibility tests, independent
marker-byte and digest reproduction, Release and ASan/UBSan builds, successful
fresh-root repetitions on both Strix Halo nodes, and retained service and
reference-clone checks.

The exact production binary is killed at syscall entry and exit for marker
create, mode correction, every marker write, marker-file synchronization,
no-replace rename, root synchronization, and staging synchronization. A
separately hashed test controller covers returned errors, `EINTR`, short and
zero I/O, malformed readback, truncation, append, bit corruption, collision,
late completion, `ENOSPC`, `EDQUOT`, `EIO`, `EROFS`, `EXDEV`, unsupported
rename, reserve loss, hostile substitution, unexpected names, and cleanup
failures. Qualification retains exact commands, binaries, raw events, residual
tree inventories, filesystem identities, and whole-media cleanup receipts.

## Nonclaims and rollback

File and directory synchronization are process-crash ordering evidence, not
power-loss, device-cache, firmware, or cross-kernel durability proof. L05y does
not establish normal reopen, repair, quarantine, completed initialization,
publication authority, persistent server writes, cache hits, restore,
tenant-sharing, distributed behavior, inference performance, or zero-regression
performance.

Source rollback removes the L05y extent, audit fields, marker construction and
publication helpers, focused tests, and separate L05y controllers while
preserving the committed L05w/L05x controls. Any qualification root that
crossed the latch remains discard-only; rollback never authorizes per-entry
cleanup or adoption.

## Qualification status

The frozen candidate completed its focused, inherited, feature-off, live-root,
exact-production crash, and deterministic returned-fault qualification on both
Strix Halo nodes. The retained evidence covers 225 successful fresh roots,
700/700 exact-production `SIGKILL` cells, and five 409-case returned-fault
lanes. Nimo-2 supplied complete Release and ASan/UBSan target coverage. On
nimo-1, the Release target, an ASan/UBSan controller against the Release target,
and a UBSan target all passed; an ASan-instrumented target under `ptrace`
reproducibly hung before and after a controlled reboot and with both nodes'
binaries, while ASan-only hung and UBSan-only passed. That host interaction is
retained as an explicit limitation and is not claimed as a pass. Complete
target-plus-controller ASan/UBSan evidence exists on nimo-2.

All qualified roots remained discard-only, qualification media was removed as
whole media after evidence retention, services recovered or remained
continuous, and the locked reference repositories remained unchanged. These
results do not change the nonclaims above. Promotion remains closed until the
independent final milestone review reconciles this contract, the receipt, the
retained bundles, and the frozen source hashes.
