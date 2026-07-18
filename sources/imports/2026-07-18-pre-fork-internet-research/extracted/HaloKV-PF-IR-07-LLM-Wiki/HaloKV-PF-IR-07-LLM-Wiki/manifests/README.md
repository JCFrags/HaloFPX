# Bundle manifests

* `SHA256SUMS` covers every file outside the `manifests/` directory.
* `SOURCE_SHA256SUMS` covers every file under `sources/`.
* `files.json` is the size/hash inventory for every file outside `manifests/`.
* `manifest-root.json` records bundle metadata and hashes of the primary manifests.
* `ROOT_SHA256SUMS` covers the manifest files themselves, except that it cannot include its own hash.

Run `python3 validation/verify_bundle.py` from the bundle root. A matching hash
proves byte consistency with this bundle's recorded state, not upstream
publisher authenticity or signature verification.
