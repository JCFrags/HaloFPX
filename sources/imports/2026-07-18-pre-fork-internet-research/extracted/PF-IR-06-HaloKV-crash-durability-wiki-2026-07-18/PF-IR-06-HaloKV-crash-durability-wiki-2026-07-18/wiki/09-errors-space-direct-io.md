# Errors, allocation, direct I/O and discard

## ENOSPC and EDQUOT

Space failure is a multi-stage result. It can occur during inode creation, `fallocate`, a buffered write, delayed allocation/writeback, file sync, link/rename, directory expansion, or directory sync. Btrfs can fail for metadata even when data bytes appear available. The transaction must check every stage and preserve the old object until new publication is fully acknowledged.

Capacity admission can reserve headroom and use `fallocate`, but reservation is not proof that later filesystem metadata and namespace operations will succeed. On reflink/shared extents, `FALLOC_FL_UNSHARE_RANGE` can have specific reservation value, yet the immutable design should avoid mutable shared extents entirely.

## EIO

EIO is not an object-local retry signal by default. It can represent media, device, transport, filesystem or writeback failure affecting more than one descriptor. Fence the root, stop acknowledgements, capture kernel logs and mount state, and recover on a verified profile. Retrying a write into the same immutable candidate does not prove consistency, especially for DIO.

## Direct I/O

DIO is optional. Query `STATX_DIOALIGN`; use the reported memory, offset and length alignment. If unavailable or zero, disable DIO unless a filesystem-specific query is explicitly selected. Do not assume page size, filesystem block size or 4096 bytes is sufficient.

Treat any DIO error as rendering the attempted range inconsistent. Because live objects are immutable, discard the unpublished inode and retry from a fresh inode only after root health is established. Do not mix DIO with buffered I/O or writable mmap for the same object. Avoid `fork` while DIO uses private-mapped memory.

## Discard and hole operations

Discard/TRIM, hole punching, collapse/insert range and zero-range operations are space/layout controls. They are not durability or secure-deletion primitives. Do not apply them to published immutable objects. Eviction is namespace removal followed by directory synchronization when deletion durability is part of the acknowledgement.
