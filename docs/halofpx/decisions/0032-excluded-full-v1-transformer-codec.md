# ADR-0032: excluded full-v1 transformer snapshot codec

Status: accepted for the default-excluded L08g memory boundary. This decision
does not admit a product codec, filesystem authority, publication, or live
restore.

## Context

ADR-0029 authenticates and verifies complete full-v1 snapshots, ADR-0030 reads
them from an exact Linux root, and ADR-0031 synchronizes synthetic fixture bytes
into a disposable root. None can produce full-v1 bytes from the admitted L07
transformer snapshot or turn a verified candidate back into an owned L07
snapshot. That missing semantic bridge blocks a useful full-v1 server canary.

The owner directed risk-proportionate progress: close this specific codec seam,
retain feature-off behavior, and defer broad publication fault matrices until a
protected writer opens those risks.

## Decision

`halofpx-context-store-v1-transformer-codec` is a `STATIC EXCLUDE_FROM_ALL`
target. It accepts only the L07 target-only, world-one, rank-zero, transformer,
memoryless-greedy profile with a complete identity, nonempty bounded tokens and
opaque sequence-state bytes, all 16 nonzero compatibility component digests,
explicit single-rank topology facts, a producer identity, and a borrowed active
manifest signing key.

The encoder emits exactly two immutable full-v1 object frames:

- `halofpx.tokens.i32be.v1` with codec
  `halofpx.tokens.int32be.v1`, containing exact signed 32-bit tokens in
  big-endian order; and
- `halofpx.target-seq-state.opaque.v1` with codec
  `halofpx.llama-state-seq-ext.opaque.v1`, containing the exact L07
  `llama_state_seq_*_ext` bytes.

Both descriptors use schema `1.0`, are required, bind the same canonical token
sequence digest, logical position/output boundary, rank-zero ownership digest,
and compatibility root. The manifest is canonical full-v1 DCBOR, uses the
existing manifest KDF and HMAC domains, and is verified again through the L04b
authenticator before the encoded result exposes its bounded admission copy.
Frame sizes are computed and admitted before payload or frame allocation.

The decoder accepts only an already authenticated and frame-verified L08d
candidate. It exact-checks identity, producer, profile, world size, topology
epoch, ordered two-object roster, stream and codec IDs, schema, required/rank
facts, ownership, compatibility, logical boundaries, token digest, token count,
and every expected token before it returns a complete owned L07 snapshot. It
does not call the live restore API. Every failure returns an empty snapshot.

The encoder and decoder are `noexcept`; allocation and internal failures map to
closed statuses. The derived manifest key exists only after all allocating
authentication-message work and is wiped immediately after the non-throwing
HMAC operation or derivation failure.

## Consequences and closed gates

L08g supplies target-native semantic full-v1 bytes and an all-or-nothing decode
boundary. Its profile and codec IDs are local to this excluded seam. The
machine-readable L02 contract intentionally retains empty product admission
registries; L08g does not change that authority.

The implementation does not acquire keys or roots, publish files or anchors,
register persistent attempts, reconcile restart uncertainty, mutate a
`llama_context`, link into the server, or enable persistence. Opaque state and
payload allocator-remanence hardening remains a product-integration gate.

The next safe milestone is a generation-one protected full-v1 material carrier
and persistent attempt/reconciliation authority, followed by the default-off
server canary. Multi-generation replacement, retention, administration,
distributed state, exhaustive fault injection, soak, and final non-regression
remain later gates.

Rollback is one coherent revert. No donor code, GPL llama-ai implementation,
CachyLLama transplant, WebUI asset, remote, dependency, model, or deployment is
introduced by this decision.
