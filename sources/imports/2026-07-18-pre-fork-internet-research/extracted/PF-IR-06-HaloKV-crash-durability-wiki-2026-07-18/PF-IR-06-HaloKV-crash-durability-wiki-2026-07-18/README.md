# PF-IR-06 — HaloKV persistent-cache crash-durability wiki

**Access date:** 2026-07-18  
**Kernel semantic anchor:** Linux 7.1.3 `199c9959d3a9b53f346c221757fc7ac507fbac50`  
**liburing anchor:** 2.15 `d41bf9220ec39277ff235379e9089d9e0fd6c2a5`

> **No durability claim.** This bundle selects source-backed candidate primitives and a proof plan. It does not establish that a future or current implementation is durable. The deployed filesystem was not supplied and remains a release-blocking input.

## Start here

1. Open `index.html` for the styled wiki landing page.
2. Read `wiki/01-executive-decision-record.md`.
3. Review `matrices/guarantee-source-test.md` and `matrices/experiment-matrix.md`.
4. Run `scripts/inspect_fs_profile.sh` on both cache roots.
5. Verify evidence hashes with `python3 scripts/verify_manifest.py`.

## Candidate decision

Use immutable self-validating objects and a single-writer root. Create an unpublished inode in the destination directory, write exactly, validate, synchronize the file, atomically publish, synchronize the directory, then acknowledge. `O_TMPFILE`, direct I/O, `fallocate`, XFS `wsync`, reflink and Btrfs-specific settings are optimizations or conditional profiles—not portable durability primitives.

## Bundle map

- `wiki/` — research synthesis and decision record
- `matrices/` — claim ledger, source register, guarantee→source→test table, experiment matrix
- `raw/` — pinned source excerpts and official metadata/standards receipts
- `scripts/` — profile, publication, short-write, kill and manifest probes
- `manifests/` — SHA-256 manifests
- `licenses/` — source-license inventory
- `VERSION-PINS.md` — immutable revisions and documentation versions

## Deployment gate

Persistent writes remain disabled until the selected implementation passes the applicable `T001–T040` matrix on the exact deployed filesystem, mount options and block/device stack, including real host-level power cuts.
