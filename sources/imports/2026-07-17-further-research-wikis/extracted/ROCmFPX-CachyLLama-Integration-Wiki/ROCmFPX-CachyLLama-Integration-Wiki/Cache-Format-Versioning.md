---
title: Cache Format Versioning
description: Canonical ROCmFPX persistent-store format, compatibility, migration, and security rules.
status: Proposed format contract
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# Cache Format Versioning

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


## Decision

Define a new canonical format: **ROCmFPX Context Store v1**, namespace `.rocmfpx-context-store-v1`. Do not adopt CachyLLama's `KVRC` v3 checkpoint record, `KVSM` v1 system record, or `KVPG` page record as the canonical format. Those reviewed records are native C++ structures with fixed token arrays and version fields; the reviewed code does not establish a portable, checksummed compatibility envelope suitable for the canonical cross-platform store. [S14] [S15] [S16] [S17]

## Store layout

```text
<root>/.rocmfpx-context-store-v1/
  FORMAT
  store.json
  models/
    <model-set-digest>/
      scopes/
        <opaque-scope-digest>/
          entries/
            <entry-id>/
              manifest.json
              tokens.le32
              target.state
              draft.state              # optional
              speculative.bin          # optional
              recurrent.bin            # optional/future codec
          quarantine/
          .staging/
```

Names are illustrative but the hierarchy is normative: format → model set → opaque scope → committed entry. Retention indexes are rebuildable and are not the source of truth.

## `FORMAT`

A small ASCII file permits safe rejection before parsing complex metadata:

```text
ROCMFPX-CONTEXT-STORE
major=1
minor=0
```

Readers reject an unknown major. A known major with a newer minor is accepted only when every listed `required_feature` is understood.

## Manifest contract

`manifest.json` is UTF-8, duplicate-key rejecting, bounded in size, and contains at least:

| Field group | Required data |
|---|---|
| Format | `format_major`, `format_minor`, `required_features[]`, `optional_features[]`. |
| Provenance | writer ROCmFPX commit, upstream base commit, build ID, state codec ID. |
| Model compatibility | target model-set digest, optional draft model-set digest, architecture, tokenizer digest, chat-template digest, tensor/KV types, context layout, model metadata fingerprint. |
| Sequence | token count, position range, sequence semantics, checkpoint boundary, token digest. |
| Components | name, mandatory flag, file name, byte length, SHA-256 or BLAKE3 digest, codec/version. |
| Scope | opaque scope digest and scope kind; no raw user or prompt text. |
| Lifecycle | entry ID, generation, created/last-used timestamps, access count, retention class. |

Numeric values are range-checked before conversion to platform types. Component paths are fixed relative names; manifests cannot name arbitrary paths.

## Model-set digest

Persistent reuse requires a strong identity. The stable key is:

```text
SHA-256(
  ordered target GGUF shard SHA-256 values ||
  ordered draft GGUF shard SHA-256 values ||
  tokenizer canonical digest ||
  critical runtime cache/layout parameters
)
```

A cached sidecar may avoid rehashing large files on every startup, but the sidecar itself must be validated against file identity. A filename, mtime, size-only tuple, or donor-style compact compatibility hash is insufficient for stable cross-restart correctness.

## Component transaction

Target, draft, speculative, and required recurrent components are one logical entry:

1. Create a unique directory below `.staging` with owner-only permissions.
2. Write each component with exclusive creation; verify actual length and digest.
3. Write and sync `manifest.json.tmp`, then atomically rename to `manifest.json` inside staging.
4. Sync files and staging directory.
5. Atomically rename the staging directory to `entries/<entry-id>`.
6. Sync the `entries` directory.
7. Update rebuildable index/retention metadata after the entry is committed.

Readers ignore `.staging` and entries without a valid manifest. No component is applied to a context until every mandatory component validates.

## Compatibility rules

| Condition | Reader action |
|---|---|
| Unknown format major | Reject store/entry; no mutation. |
| Newer minor, all required features known | Read; preserve unknown optional metadata. |
| Unknown required feature | Reject entry. |
| Model/tokenizer/template/cache layout mismatch | Reject entry as incompatible. |
| Missing target or required draft/spec/recurrent component | Quarantine and cold fallback. |
| Digest/size mismatch or parse overflow | Quarantine; increment corruption metric; cold fallback. |
| Older compatible minor | Read; new writer emits current minor into a new entry. |

## Migration policy

- No in-place format mutation.
- New versions read old supported entries and write new entries beside them or into a new versioned root.
- A migration tool is offline, read-only with respect to the source, and transactional with respect to the destination.
- Donor-format migration, if approved, parses donor files in a sandboxed/offline tool and emits canonical v1; the server never auto-imports on startup.
- Rollback binaries are never pointed at a newer persistent root unless their reader support is proven.

## Scope and privacy

Explicit user IDs are transformed with an operator-held keyed digest such as HMAC-SHA-256. FNV-based donor directory compatibility may exist only inside an offline importer. Raw IDs, tokens, and prompt text do not appear in directory names or routine logs.

## Permissions and path safety

- root/model/scope/entry directories: owner-only access;
- component and manifest files: owner read/write only;
- reject symlinks, hard-link surprises, device files, and paths escaping the opened root;
- use directory-relative safe-open semantics in implementation;
- quotas account for committed payload, staging headroom, metadata, and quarantine separately;
- encryption at rest is not implied by permissions; reserve a required-feature bit for authenticated encryption if the threat model requires it.

## Future v2 boundary

Page deduplication, content-addressed components, encryption, or page-level SSD paging should trigger a major/minor design review. None is required to ship v1 whole-checkpoint persistence.


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
