# XFS profile

XFS is a realistic supported candidate. It is a metadata-journaling filesystem with buffered delayed allocation and extensive extent/B-tree allocation behavior. The portable application protocol remains unchanged.

## Material semantics

- Buffered end-of-file writes use delayed allocation. A write completion does not reserve all final metadata or establish persistence.
- XFS documents `wsync`, which makes namespace operations synchronous. It can be studied for HA behavior, but the portable contract still issues explicit directory synchronization and checks it.
- Historical `barrier`/`nobarrier` mount options were removed. Do not infer that the storage stack is therefore power safe; flush/FUA propagation still requires `T040`.
- Online discard can materially affect performance; scheduled `fstrim` is normally preferable. Discard is outside durability.
- Reflink-capable XFS can share extents. The selected immutable-object design avoids mutating shared live extents. Do not use clone-and-patch as the core write primitive.
- New atomic-write/exchange-range facilities are filesystem-specific and are not selected as the portable minimum without a separate format and fault-test decision.

## Required XFS matrix

Run the complete baseline, with emphasis on delayed-allocation ENOSPC, metadata reservation, reflink/shared extent behavior if used, XFS shutdown/read-only transitions under EIO, DIO alignment discovery, and host power cut through the deployed block stack.
