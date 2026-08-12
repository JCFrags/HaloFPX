# Provenance map

## Primary records

- `evidence/raw/commits/` — exact candidate identities and revisions.
- `evidence/raw/source-files.jsonl` — exact repository, commit, path, Git blob SHA-1, license, access date, and URL.
- `evidence/raw/source-ranges/` — selected literal upstream ranges with exact commit/path/blob identity, requested line bounds, local SHA-256, and MIT evidence.
- `evidence/licenses/` — captured license text, local/source hashes, and ROCmFPX third-party notice.
- `evidence/raw/research/upstream-asset-audit.json` — upstream binary review and exclusions.
- `evidence/source-excerpts/` — short review notes tied to primary source locators.

## Upstream-first research areas

The pinned upstream tests were used to identify tokenizer corpus conventions, chat/Jinja handling, grammar/schema paths, sampler vectors, save/load semantics, recurrent rollback, SSE parsing, tool-call tests, synthetic model generation, and speculative modes.

Publisher/open-data assets were considered after upstream assets. Exact binaries lacking sufficient terms or conversion-chain evidence were excluded rather than mirrored.

## Hash layers

- Per fixture/file/range/record: `manifests/fixtures.jsonl`.
- Generated asset summary: `qualification/generated-assets.json`.
- Whole-folder integrity: `MANIFEST.sha256`.
- Archive integrity: sibling `.sha256` files outside the folder.
