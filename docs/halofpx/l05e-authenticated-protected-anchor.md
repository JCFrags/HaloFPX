# L05e authenticated protected-anchor codec

Status: implemented offline; persistent writes remain disabled.

## Outcome

L05e freezes the protected-anchor wire and authentication contract in ADR-0008
and the v1 CDDL. The target-owned C++ codec encodes and verifies the exact closed
deterministic-CBOR envelope in memory with a 1 KiB bound. It derives a purpose-
separated anchor key, authenticates the original canonical input bytes, computes
the domain-separated full-envelope digest, and compares trusted authority and
replay state only after authentication.

The only positive status is `authenticated_unadmitted`. The target is
`EXCLUDE_FROM_ALL` and is not linked into `server-context`, `llama-server`, the
provider seam, or the L05 publication coordinator. It performs no filesystem or
path operation and cannot publish, restore, or create a cache hit.

## Frozen distinctions

- Store UUID is exactly 16 bytes on wire; namespace and lineage IDs are 32 bytes.
- Anchor-authentication key generation and selected manifest-key generation are
  separate fields.
- Writer-authority epoch is authenticated anchor identity.
- Generation one has a null predecessor; later generations require a digest.
- Attempt ID is absent and conveys no authority.
- Exact canonical envelope bytes/digest, not selected parsed fields, are the
  future protected CAS unit.
- Authentication alone is not anti-rollback storage. Protection depends on the
  external anchor/key/authority state remaining outside the cache root.

## Golden vector and tests

The C++ encoder and the independent Python standard-library encoder reproduce a
229-byte envelope, tag
`41b4d7a3821784aa8776ac4dad38db57ffea381e892d597e0efca1b9717274a3`,
and envelope digest
`0e2ecaa98c3b05cedc60b5ca3b5947ddd49f62790367edde905be20967be55a3`.
The generator and receipt are retained under `tests/halofpx-anchor-golden.py`
and `evidence/l05e-anchor-golden-vector.json`.

Focused Release tests cover round trip, every-byte truncation, trailing and
oversized input, duplicate/unknown/reordered fields, indefinite and non-shortest
forms, wrong versions/algorithm, malformed lengths, invalid/NUL/non-ASCII key
IDs, authenticated body and tag mutation, key lifecycle, authority mismatch,
old/future replay, predecessor rules, output atomicity, input immutability, and
concurrent determinism. The L02 locked-contract test was intentionally advanced to the
reviewed authenticated-anchor schema while its writer gate remains closed.

The clean Release build passed all 16 HaloFPX CTests and the seven focused
inherited tests. One hundred fresh C++ codec processes and one hundred fresh
independent Python golden-check processes also passed; exact hashes and counts
are retained in `evidence/l05e-anchor-repeat-receipt.json`.

## Remaining gates

A concrete writer is still blocked on:

1. reconciling the offline coordinator's provisional 32-byte store identifier
   with the frozen 16-byte UUID and null-predecessor wire representation;
2. a separately authenticated first-store bootstrap transition;
3. a protected anchor-key registry and authority source;
4. a cross-process exact-envelope CAS substrate with ambiguity fencing;
5. Linux filesystem-specific no-follow, no-replace, sync, crash, and power-loss
   qualification; and
6. the later quota, reserve, retention, observability, rollback, and
   administrative gates.

No reference repository, node, service, model, runtime hook, or persistent root
was changed for L05e.
