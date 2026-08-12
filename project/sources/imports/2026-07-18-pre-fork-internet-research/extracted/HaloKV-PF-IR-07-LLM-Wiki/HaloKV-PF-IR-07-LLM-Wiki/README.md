# HaloKV PF-IR-07 — encryption, tenant identity, key lifecycle, backup, and deletion

This folder is a self-contained static “LLM Wiki” research bundle for the
standards-backed option set and residual-risk ledger required to unblock
`OPEN-SCOPE-01`.

## Open

Open `index.html` directly, or run:

```bash
python3 serve.py
```

Then browse the local address printed by the server. The wiki has no external
runtime dependencies; source links require network access only when followed.

## Decision summary

* Object-level AEAD is mandatory for multi-user persistent cache objects and manifests.
* LUKS2/dm-crypt is the recommended rank-local volume baseline where supported.
* fscrypt v2 is conditional defense in depth, not object authentication.
* Private deduplication is scoped no wider than authorized tenant/project sharing.
* Public/system prefixes have a separate publisher authority and namespace.
* Any unknown/corrupt/unauthenticated/wrong-key/mismatched/stale object is `MISS_RECOMPUTE`.
* Cache backup is disabled by default; restore is quarantined and re-encrypted.
* Logical deletion, cryptographic erasure, discard, and media sanitization remain distinct claims.

## Contents

* `wiki/` — 16 research and decision chapters, Markdown and HTML.
* `runbooks/` — 8 operational runbooks plus index, Markdown and HTML.
* `matrices/` — claims, option comparison, threat/control mapping, key hierarchy, minimum profile, human decisions, and residual risks in CSV/JSON.
* `schemas/` — CDDL and JSON Schema design artifacts.
* `examples/` — fail-closed pseudocode, nonce allocator, policy, and negative-test template.
* `sources/` — raw primary documents, exact revisions/access dates, license notices, extracted text, catalog, and archival limitations.
* `manifests/` — SHA-256 inventories and root manifest.
* `validation/verify_bundle.py` — offline integrity verifier.

## Integrity

Run from the folder root:

```bash
python3 validation/verify_bundle.py
```

A matching hash verifies consistency with this bundle's manifest; it does not
replace upstream signature or publisher-authenticity verification. The systemd
XML and cryptsetup release-archive verification limitations are documented in
the source catalog and source notes.

## Scope caveat

This bundle does not select the principal authority, sharing policy, key owner,
escrow, retention, or physical-media disposition authority. It also makes no
certification, compliance, FIPS-validation, cryptographic-erasure, or secure-
deletion guarantee for a deployment without deployment-specific evidence.

Research access date: **2026-07-18**. Bundle version: **1.0.0**.
