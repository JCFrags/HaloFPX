# Decision

## Proposed acceptance rule

A fixture can enter the approved differential run only when all of these are true:

1. Its exact bytes are self-generated under CC0 or its exact redistribution terms are captured.
2. Its locator and SHA-256 appear in `manifests/fixtures.jsonl`.
3. The candidate and exact commit are identified.
4. Local source review confirms the proposed applicability.
5. Any `QUALIFICATION-REQUIRED` asset succeeds in isolated execution.
6. A human approves the exact accepted-asset manifest.

## Current result

The external provenance/license inputs are sufficiently packaged to unblock review of OPEN-TEST-01 and OPEN-API-01. The live execution decision is deliberately not made here.

## Semantic policy

| Domain | Oracle |
|---|---|
| GGUF container, tokenizer IDs, template bytes | Exact |
| Canonical API JSON and normalized SSE | Exact after declared projection/normalization |
| Sampler probabilities and restored logits | Numerical tolerance |
| RNG clone/reset, save/restore, speculation | Metamorphic semantic property |
| Malformed inputs and unsupported fork types | Coarse expected-rejection class |
| Opaque state serialization bytes | Not applicable cross-fork |

## Non-negotiable caveat

The tiny Llama GGUF is structurally verified and reproducible, but no candidate has loaded it. Its status is therefore `QUALIFICATION-REQUIRED`, not “known runnable.”
