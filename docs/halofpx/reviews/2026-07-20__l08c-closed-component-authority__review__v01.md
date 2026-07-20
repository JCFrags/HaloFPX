# L08c closed compatibility-component authority independent review

## Verdict

**ACCEPT for the default-off generation-one laboratory canary only.**

No P1/P2 correctness, security, provenance, rollback, or feature-off blocker
was found. This verdict does not promote full ADR-0003 semantic construction,
production persistence, L08, distributed recovery, or final zero-regression.

## Finding and resolution

One P3 documentation issue was found. The milestone initially said the seam
“authenticates the component set.” The inputs are trusted operator-supplied
digests, so the accurate claim is that the seam binds the trusted ordered set
into the compatibility root. The milestone language was corrected before
commit.

## Review conclusions

- The exact `halofpx.compat.v1\0` domain, deterministic CBOR map(16), integer
  keys 0 through 15, 32-byte byte-string values, 579-byte wire, and golden
  SHA-256 are correct.
- The compiled labels and order exactly match the frozen JSON/CDDL registry.
  Missing, unknown, duplicate, misordered, zero, malformed, or non-lowercase
  inputs fail closed.
- Protected mode rejects an opaque root and requires the closed component
  list. Direct-rw retains its opaque root and rejects component-list input.
- Configuration input is validated before key or filesystem access.
- Sampler admission is restricted to exact memoryless greedy operation;
  recurrent/hybrid, multimodal, adapters, draft/speculative, control vectors,
  KV overrides, and active placement overrides remain rejected.
- The tensor-override predicate ignores only ROCmFPX's inert null padding and
  rejects actual entries.
- The live canary changes the tokenizer component, proves a miss under the
  changed root, and then proves an authenticated hit after returning to the
  original 16 components.
- Seven focused contracts, the live protected canary, and the all-off checks
  are proportionate for this bounded milestone. Deferred exhaustive matrices
  are explicit.
- The implementation is target-native and changes no dependency, notice,
  SBOM, donor/P3 roster, WebUI, remote, model, or service deployment.

Rollback remains an all-off rebuild or one coherent revert.
