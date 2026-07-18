# L02 persistence safety contracts

Status: accepted; implementation and persistent I/O remain disabled.

L02 closes `OPEN-STATE-01`, `OPEN-SCOPE-01`, and `OPEN-FMT-01` at the
contract level. It does not admit a state codec, authorize a donor import, or
enable a provider. The normative machine-readable summary is
[`contracts/context-store-v1.json`](contracts/context-store-v1.json). If prose
and that file disagree, the more restrictive behavior applies until an
approved decision changes both.

## Decisions

1. [ADR-0001: complete-state admission](decisions/0001-complete-state-admission.md)
2. [ADR-0002: scope and authority](decisions/0002-scope-and-authority.md)
3. [ADR-0003: target-owned storage format](decisions/0003-target-owned-storage-format.md)
4. [ADR-0004: publication and failure](decisions/0004-publication-and-failure.md)
5. [ADR-0005: distributed ownership and threat model](decisions/0005-distributed-ownership-and-threat-model.md)

## Locked milestone behavior

- Feature off is the only enabled mode.
- Persistent reads, writes, migration, shared reuse, and anonymous reuse are
  disabled.
- No state profile or codec is admitted. A later codec must name every required
  stream and pass exact cold-versus-restored continuation tests for its bounded
  model/runtime/mode tuple.
- Every parse, integrity, compatibility, authorization, topology, completeness,
  or replay uncertainty results in a miss and cold recomputation. Malformed or
  corrupt material may also be quarantined; it is never partially restored.
- Donor formats are not internal ABI. A future legacy tool may inventory them
  only offline, read-only, bounded, and without making a server-visible hit.
- One writer owns a publication root. Persistent writing stays disabled until
  the publication model-check, crash matrix, quota/reserve, rollback, and
  operational gates pass.

## Failure taxonomy

| Class | Examples | Required result |
|---|---|---|
| authorization | absent principal, wrong security domain, shared-policy denial | opaque miss; do not reveal existence |
| compatibility | model, tokenizer, template, adapter, backend, quantization, sampler, grammar, tool, topology mismatch | diagnostic miss and recompute |
| unsupported | unknown major, codec, stream, critical field, durability mode | miss; no optimistic repair |
| malformed/corrupt | duplicate field, noncanonical encoding, bad length/hash, trailing bytes, truncation, hostile path | miss, bounded quarantine candidate |
| incomplete | missing required stream/rank/generation | whole-generation miss |
| replay | generation below protected high-water mark, stale nonce/epoch | miss and security event |
| storage | full/reserve exhausted/read-only/I/O or sync failure | fail publication; inference continues cold |

Metrics use bounded reason labels and never tokens, prompts, principals, paths,
templates, or full hashes. Cache failure is non-fatal to inference except an
explicit administrator security shutdown.

## Evidence reconciliation

These decisions implement the accepted v03 L02 lane and the canonical Wiki
recommendations in Sections 57-61, 63-64, and 71. They intentionally do not
freeze page size, segment layout, compaction, DAG policy, asynchronous I/O, or
an admitted codec. Those implementation choices require their own evidence and
tests. The Section 63 TLA+/TLC gate is required before the first writer
implementation, not before a disabled interface seam or synthetic read-only
parser.

The exact v1 integer-key field registry is
[`contracts/context-store-v1.cddl`](contracts/context-store-v1.cddl). V1 has no
extension map and no unregistered keys. CDDL constrains structure; the ADRs add
deterministic-CBOR, authentication, semantic, I/O, and safe-open rules that the
schema cannot express.

Canonical research, experiments, provenance, and source locks remain in
`C:\Users\britt\Documents\Custom_Inference_Project`; this directory contains
only implementation-local decisions, contracts, tests, and reviews.
