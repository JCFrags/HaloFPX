# Temporary files, publication and overwrite behavior

## O_TMPFILE

`O_TMPFILE` creates an unnamed inode in the specified directory's filesystem. It avoids a discoverable temporary name and can be linked with `linkat(..., AT_EMPTY_PATH)` when created without the `O_EXCL` modifier. Support and linking behavior are runtime capabilities, not assumptions. ext4, XFS and Btrfs have Linux support, but the deployed mount, security policy and kernel must pass `T002`.

Fallback: a named temporary created by `openat(parent, name, O_CREAT|O_EXCL|...)`. The name must be unique, never interpreted as live, and cleaned by recovery.

## Exact-length writes

Neither `write` nor an io_uring write CQE promises the whole requested count. The implementation owns a `(base, total, completed)` state. Every attempt uses the remaining slice at an explicit file offset. A positive short result is progress, not success. Zero with remaining bytes is a terminal protocol error.

Do not use an implicit shared file offset in concurrent asynchronous paths. It complicates ordering and can couple unrelated operations.

## Atomic publication is not durable publication

- `rename` replacement is atomic for readers in the namespace.
- `RENAME_NOREPLACE` gives create-if-absent behavior on supported filesystems.
- `RENAME_EXCHANGE` swaps two names atomically.
- None of these operations substitutes for prior file synchronization or subsequent directory synchronization.
- Cross-mount rename fails with `EXDEV` and is outside the primitive.

## Overwrite policy

A live cache object is immutable. Replacement writes a separate inode, synchronizes and validates it, then atomically replaces the name. Readers with an old open descriptor may continue to read the old inode, which is acceptable for immutable generations. The application must not patch, truncate, clone-and-mutate, punch holes in, or map a published object writable.

## Unlink and eviction

Eviction durability is a separate namespace transaction. If it matters that a deletion survive crash before reclaim/ack, unlink the entry and synchronize its parent directory. Space reclamation and discard can lag; neither is part of the logical deletion acknowledgement unless explicitly selected and tested.
