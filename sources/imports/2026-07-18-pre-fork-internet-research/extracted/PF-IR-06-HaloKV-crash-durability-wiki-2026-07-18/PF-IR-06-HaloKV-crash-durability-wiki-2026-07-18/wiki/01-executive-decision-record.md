# Executive decision record

## Decisions that can be unblocked now

### OPEN-FMT-01 — candidate format primitive

<span class="badge r">RECOMMENDATION</span> Use immutable, self-describing objects. Minimum fields: magic, format version, header length, declared payload length, object/cache identity, content hash, generation/epoch, and flags. The live namespace points only to a fully validated object. The format must support rejecting truncation, stale generation, wrong key, wrong cache type and hash mismatch during recovery.

This is a source-backed design option, not a finalized wire/on-disk format. Endianness, hash choice, header extensibility and compatibility policy remain implementation decisions.

### OPEN-STORAGE-01 — candidate storage primitive

<span class="badge r">RECOMMENDATION</span> One parent directory and one writer per cache root. Create an unpublished inode in that directory, write exactly, validate, `fdatasync`/`fsync` it, atomically publish with `linkat` or same-filesystem `renameat2`, `fsync` the directory, then acknowledge.

- `O_TMPFILE` is an optimization after a runtime probe; named `O_CREAT|O_EXCL` is the fallback.
- Create-only uses `linkat`/`RENAME_NOREPLACE` semantics.
- Replacement uses whole-inode `rename`; published files are never mutated in place.
- `RENAME_EXCHANGE` is optional and does not remove synchronization steps.

## Supported-filesystem posture

| Filesystem | Posture | Minimum profile |
|---|---|---|
| Deployed filesystem | **Unknown / release blocker** | Record with `T001`; certify exact mount and block stack |
| ext4 | **Provisional baseline** | `data=ordered`, barriers enabled, no unsupported overlay/network layer, explicit app sync |
| XFS | **Supported candidate** | Local XFS, explicit sync, delayed-allocation and power-cut tests |
| Btrfs | **Conditional** | Immutable objects, default COW/checksum behavior, near-full metadata/data + snapshot/reflink tests |

## Writer policy

<span class="badge r">RECOMMENDATION</span> HaloKV and the RPC model-tensor cache use separate roots and dedicated lock files. One writer owns each root through an exclusive OFD lock or `flock` for the process lifetime. Locks are advisory and local; they are not durable fencing or multi-host consensus.

## Minimum pre-write gate

Persistent writes remain disabled until the exact implementation passes: deployment discovery, all kill boundaries, data- and metadata-ENOSPC, delayed EIO at sync, overwrite/create collision, recovery validation, io_uring cancel/late-CQE/resource teardown, and host-level power cut over the deployed stack. See `../matrices/experiment-matrix.md`.
