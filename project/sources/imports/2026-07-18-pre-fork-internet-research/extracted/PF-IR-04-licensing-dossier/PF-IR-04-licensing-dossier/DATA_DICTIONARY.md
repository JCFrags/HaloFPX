# Data dictionary

## Common status fields

- `promotion_status`: conservative operational disposition, not a legal determination.
- `license_declared`: license asserted by the producer/repository/package metadata.
- `license_concluded`: evidence reviewer conclusion; `NOASSERTION` when unresolved.
- `claim_label`: literal classification of the evidence source or gap.
- `git_blob`: Git SHA-1 object identity for exact file content where available.
- `capture_path`: local dossier path to exact or line-normalized evidence.

## Hashes

- Git blob SHA-1 binds a capture to a repository file object.
- SHA-256 binds dossier files and proposed release artifacts.
- `hashes/SHA256SUMS` excludes only the hash index files themselves to avoid circularity.
