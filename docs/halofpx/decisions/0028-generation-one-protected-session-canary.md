# ADR-0028: generation-one protected direct-session canary

Status: accepted for implementation after independent adversarial review. This
authorizes only the default-off disposable generation-one canary defined here,
not L08 or production persistence.

## Context

L08a proves an authenticated private miss, immutable direct-store publication,
restart hit, and corruption-safe recomputation. Its `HFPXLD01` object becomes
visible before a separately protected publication authority exists, so replay
of a copied valid object and ambiguous post-rename durability prevent L08 or
production-trusted-hit claims.

The excluded L05 coordinator, synthetic bootstrap, and discard-only registry
initializer are evidence and contract authorities, not product backends. They
must not be silently linked into the server. HaloFPX also lacks a server-owned
encoder for the full ADR-0003 authenticated manifest and a concrete qualified
CAS/attempt-fencing backend. Those remain genuine blockers to full ADR-0004
and L08 promotion.

## Decision

HaloFPX may implement one narrower Linux generation-one protected-session
canary. It composes the existing direct material with an authenticated external
anchor so an unanchored object can never become a server hit. It remains a
laboratory canary and does not claim the full v1 storage ABI or ordinary
generation advancement.

Three independent gates are required:

1. `HALOFPX_CONTEXT_STORE_CANARY=ON`;
2. a new `HALOFPX_CONTEXT_STORE_PROTECTED_CANARY=ON`, default `OFF`, Linux only;
3. explicit runtime mode `protected-rw-canary` with every required path and
   identity supplied.

When either build gate is off, the protected option, help text, source linkage,
and runtime surface are absent. Existing `direct-rw` behavior remains the L08a
control and is not silently upgraded.

## Roots, keys, and identity

The operator supplies an already-created owner-only data root, a distinct
owner-only protected-anchor root, an owner-only exact 32-byte key file, a
stable 128-bit store UUID, the existing exact compatibility authority, and the
L08a quota/reserve/entry limits. Data and anchor roots must be different,
non-nested canonical directories. Each root is pinned by opened descriptor,
owner, device, mount identity, mode, and no-follow checks. One nonblocking
writer lock is held for each root for the provider lifetime.

The operator key derives disjoint scope, direct-manifest, and anchor authority
by exact domain-separated HMAC. All quoted domains include their terminating
NUL. Integer lengths and generations are unsigned big-endian:

```text
K_scope = HMAC-SHA-256(K_operator,
  "halofpx.protected-canary.scope-key.v1\0" || store_uuid)

K_direct = HMAC-SHA-256(K_operator,
  "halofpx.protected-canary.direct-manifest-key.v1\0" ||
  store_uuid || namespace_id || key_id_len:u16be ||
  "halofpx-protected-direct-v1" || key_generation:u64be)

K_anchor_master = HMAC-SHA-256(K_operator,
  "halofpx.protected-canary.anchor-master.v1\0" ||
  store_uuid || namespace_id || key_id_len:u16be ||
  "halofpx-protected-anchor-v1" || key_generation:u64be)
```

Both closed key generations are exactly one. `K_scope` is then used only by
the existing ADR-0002 namespace derivation, whose canonical preimage binds the
policy key ID, authentication issuer, principal, security domain, policy epoch,
private scope class, and compatibility root. `K_anchor_master` is supplied to
the ADR-0008 KDF, which again binds its anchor-key ID, store UUID, namespace,
and generation. Raw and derived key bytes are never stored in either root and
are wiped at lifetime boundaries. Rotation and key fallback are unavailable.

The canonical anchor path is exactly
`<anchor-root>/<namespace-id-lower-hex>/<session-id-lower-hex>.anchor`.
Both identifiers are validated 32-byte values rendered as exactly 64 lowercase
hexadecimal characters. The scope directory and anchor are opened only through
the pinned anchor-root descriptor with `openat`-family no-follow operations;
absolute paths, separators, traversal, prefixes, normalization, alternate
spellings, and enumeration-based selection are forbidden.

The exact private namespace remains ADR-0002. Each opaque 256-bit session ID is
also the checkpoint-lineage ID. A session is single-use and supports only
generation one. Its anchor body binds:

- the configured store UUID;
- exact private namespace;
- fixed nonzero policy, manifest-key, authority, and anchor-key generations;
- checkpoint lineage equal to the exact session ID;
- generation one with a null predecessor; and
- the selected digest of the exact authenticated `HFPXLD01` manifest.

The selected digest is
`SHA-256("halofpx.direct-manifest.v1\0" || exact_manifest_bytes)`. This
domain is specific to the restricted canary and is not
`halofpx.manifest.v1`; the direct manifest must never be relabeled as the full
ADR-0003 manifest.

The anchor wire object is precisely the canonical ADR-0008
`authenticated-publication-anchor-v1` envelope, not a similar canary format.
The product canary uses a target-native canary-owned encoder/verifier that must
be byte-identical to the offline ADR-0008 codec under checked-in cross-golden
vectors. Product linkage must not include the excluded L05 anchor codec,
publication coordinator, simulator, bootstrap, registry, or synthetic backend.
Admission, idempotent retry, and reconciliation compare the complete canonical
envelope size and every byte; parsed-field or digest equality alone is never
sufficient.

## Publication and recovery

Publication is synchronous and ordered:

1. capture only the already-admitted transformer-sequence-v1 state;
2. publish and synchronize immutable direct material, returning an owned
   receipt containing its exact manifest bytes and selected digest;
3. encode the exact generation-one authenticated anchor;
4. write, synchronize, and re-read an anchor staging file;
5. publish the anchor without replacement and synchronize its parent;
6. re-open and authenticate the exact anchor and direct receipt; and
7. only then acknowledge publication.

An equal retry is idempotent only when authenticated direct material and anchor
bytes are exact. Any unequal collision is `conflict`, never replacement or
success. A failure before anchor visibility leaves unreachable material. A
conclusively absent anchor after an ambiguous create also leaves unreachable
material. If an ambiguous result first re-reads the exact authenticated
proposed anchor, the provider must then successfully synchronize the anchor
parent directory, re-open the anchor, and re-authenticate exact full-envelope
byte equality. Only that sequence may return a distinct
recovered-success-and-durable classification. Visibility before the fresh
synchronization is not durability. Any synchronization failure or different
or malformed observation quarantines that lineage and forces cold inference.

This bounded recovery is intentionally generation-one and deterministic. It
does not authorize ordinary anchor replacement, generation advancement,
authority transfer, late asynchronous work, or implicit retry after an
uncertain/malformed observation.

## Hit admission

Restore must authenticate in this order before any llama-state mutation:

1. inspect the exact direct manifest without returning payload;
2. construct the only admissible generation-one anchor expectation;
3. safe-open the exact canonical derived anchor path and authenticate the
   complete ADR-0008 envelope with the exact protected policy;
4. require exact store, namespace, session lineage, policy/key/authority
   generations, selected manifest digest, generation one, null predecessor,
   and exact complete canonical-envelope bytes;
5. reopen and revalidate the same immutable direct manifest and payload hashes;
6. apply the existing complete-state, profile, compatibility, and exact-token
   checks; and
7. only then restore the state.

Missing, unsupported, stale, corrupt, truncated, wrong-version, wrong-domain,
wrong-scope, wrong-store, wrong-session, wrong-generation, unexpected-entry,
or anchor/material disagreement is a miss or lineage quarantine and cold
recomputation. No unanchored direct object is a hit. A failed underlying state
restore clears the destination slot as in L08a.

## Qualification and limits

Essential promotion evidence for this canary is limited to:

- feature-off build/help/link equivalence;
- focused unanchored-miss, exact-anchor-hit, corrupt/wrong-anchor-miss,
  exact-present recovery, absent recovery, conflict, and cold-degradation
  provider tests;
- inherited direct-store, scope, transformer-codec, feature-off, and slot-state
  smoke;
- one nimo Linux server miss/write/restart/hit plus corruption-safe cold
  recomputation; and
- one independent implementation review.

Exhaustive filesystem/crash matrices, generation advancement, multi-writer,
retention, distributed state, and production enablement remain deferred unless
a concrete defect or promotion gate requires them.

This milestone does not close ADR-0004 or L08. Full promotion still requires a
canonical target-native v1 manifest encoder, complete in-engine compatibility
construction, concrete bootstrap/CAS/attempt-fencing and persistent
reconciliation, crash/power-loss qualification, administration, retention,
two-node ownership, rollback, and final zero-regression evidence. The protected
canary remains default-off and disposable; rollback is an OFF build or one
coherent revert. No donor implementation, GPL code, WebUI, remote, or release
surface is admitted.
