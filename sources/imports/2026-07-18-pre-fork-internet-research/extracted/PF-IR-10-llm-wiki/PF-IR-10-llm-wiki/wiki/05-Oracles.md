# Oracle classes

The machine-readable definitions are in `manifests/comparator-profiles.json`.

## Exact-output oracles

Use exact bytes, token vectors, template UTF-8, canonical JSON trees, normalized SSE event order, allowed grammar-token sets, and GGUF structural fields. Dynamic API identifiers/timestamps are projected before comparison only when the profile says so.

## Numerical comparators

- Sampler probability vectors: absolute tolerance `1e-5`, relative tolerance `0`.
- Restored F32 logits: absolute tolerance `1e-5`, relative tolerance `0`.
- Token order and continuation token IDs remain exact.

## Metamorphic/state comparators

RNG, sampler clone/reset, context restore, sequence isolation, recurrent rollback, and speculation are evaluated as semantic relationships within a candidate. A fixed seed does not create a cross-fork exact RNG oracle unless the RNG algorithm/state contract is separately proven identical.

## Expected rejection

Malformed inputs and unsupported custom tensor types compare a coarse rejection class: load/request/template/grammar/unsupported-type. Diagnostic wording is never an exact oracle.

## Explicitly not applicable

Opaque save-state bytes are implementation/version artifacts. Cross-fork correctness is continuation/logit equivalence, not serialization equality.
