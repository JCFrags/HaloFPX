# Source audit and reproducibility

Start with `../VERSION-PINS.md`, `../matrices/source-register.md`, and `../manifests/raw-manifest.json`.

## What is preserved

- immutable kernel/liburing/btrfs revisions and upstream blob IDs;
- current release receipts for Linux, liburing, man-pages, btrfs-progs and xfsprogs;
- normalized excerpts for source-controlled documents and implementation paths, each tied to an immutable revision and upstream blob ID;
- normalized official web/standards receipts with explicit non-byte-exact labels;
- license attribution per capture;
- local SHA-256 hashes and complete relative-path manifests;
- claim/source/test cross-references.

## What is not preserved

Full upstream tarballs are not embedded. Their size is unnecessary for the claim audit, and this bundle does not falsely label excerpts as full files. A future air-gapped evidence refresh can add signed release tarballs and signatures without changing claim IDs.

## Verification

```sh
python3 scripts/verify_manifest.py
sha256sum -c manifests/SHA256SUMS
```

The first command verifies the raw-capture manifest and then the full checksum list. Regeneration should use a new access date and retain the prior bundle for diff review.
