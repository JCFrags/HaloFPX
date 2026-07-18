# Portable Linux minimum contract

## Candidate publication state machine

1. **Acquire root ownership.** Hold the dedicated root lock; run startup recovery and profile validation.
2. **Open the parent directory.** Retain a directory descriptor for all relative namespace operations and final synchronization.
3. **Create an unpublished inode.** Prefer `O_TMPFILE` after `T002`; otherwise create a random/unique named temporary with `O_CREAT|O_EXCL` in the same directory.
4. **Optional reservation.** `fallocate` may reserve space, but every later result is still checked.
5. **Exact write.** Use explicit offsets. Accumulate positive short results. Retry `EINTR` only under a defined policy. Treat zero with bytes remaining as failure. Treat any terminal error as transaction failure.
6. **Validate.** Confirm exact size and object header/hash before publication.
7. **Synchronize the file.** `fdatasync` for payload + retrievability metadata; `fsync` when additional inode metadata is part of the contract.
8. **Publish atomically.** `linkat`/`RENAME_NOREPLACE` for create-only; same-filesystem `renameat2`/`renameat` for replacement. No in-place overwrite.
9. **Synchronize changed directories.** At minimum the parent directory. If a design changes two directories, synchronize both and test that exact sequence.
10. **Acknowledge.** Only after the final directory synchronization returns success.
11. **Recover.** On startup, validate every candidate before selection; remove/quarantine orphan temps and corrupt generations.

## Error transition table

| Result | Required transition |
|---|---|
| Positive short write/CQE | Continue at the next explicit offset; do not publish or sync as though complete |
| `EINTR` | Retry according to bounded policy, preserving exact offset and remaining length |
| Zero with remaining bytes | Abort to avoid an infinite loop |
| `ENOSPC` / `EDQUOT` | Abort; clean unpublished object when possible; keep prior live object; no ack |
| `EIO` | Abort and fence the cache root; no further persistent acknowledgements |
| File sync failure | Abort; never publish an unsynchronized object |
| Link/rename failure | Abort or resolve idempotent collision; no ack |
| Directory sync failure | Treat publication as unacknowledged/indeterminate across power loss; fence or force recovery |
| Close failure | Record; do not retry close; decisive sync should already have run |

## Crash boundaries

Process death and machine power loss are different tests. `SIGKILL` validates application cleanup and namespace sequencing, but only host-level VM/hardware power cuts exercise filesystem journal replay, block flushes, controller caches and firmware. An `fsync` success is meaningful only within a storage stack that honors the flush/FUA path.
