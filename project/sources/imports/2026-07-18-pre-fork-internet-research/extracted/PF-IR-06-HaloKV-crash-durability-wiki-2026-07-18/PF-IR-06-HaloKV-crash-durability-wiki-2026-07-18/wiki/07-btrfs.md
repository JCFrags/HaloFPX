# Btrfs conditional profile

Btrfs is realistic only as a conditional alternative. Copy-on-write, checksums, compression, snapshots, subvolumes, reflink/deduplication and global metadata allocation materially change space-failure and recovery behavior.

## Hazards

- A logically small write can require new data and metadata extents; free-space readings do not by themselves guarantee the transaction will complete.
- snapshots and reflinks retain shared extents and can amplify later COW allocation.
- compression changes physical allocation and write granularity.
- `nodatacow` implies `nodatasum`, disables compression for affected new files, and permits interrupted in-place updates to be partial. It does not justify mutable published cache objects.
- mount options generally apply to the whole filesystem based on the first mounted subvolume, not independently per cache subvolume.
- `commit=` and `flushoncommit` change global transaction behavior but do not replace explicit application synchronization.
- `discard=async` is an allocation/reclamation setting, not a durability primitive.

## Candidate posture

Use immutable objects and the same file-sync → publish → directory-sync sequence. Prefer the normal COW/checksum profile rather than NOCOW for live immutable objects unless a separate test record establishes otherwise. Avoid relying on reflink clones as private mutable copies.

## Mandatory extra tests

`T037` must cover data and metadata near-full states, multiple snapshots, reflinked objects, compression on/off, qgroups/quotas if enabled, orphan temps, repeated replacement and host power cuts. Support remains conditional until this exact profile passes.
