# Persistent KV-Cache Architecture and Integrity

<span class="badge observed">OBSERVED</span> CachyLLama, llama.cpp, LMCache, SGLang, vLLM  
<span class="badge recommended">HALOFPX RECOMMENDATION</span> Immutable, authenticated, fail-closed persistence

This Wiki answers one operational question: **when is persisted inference state safe to reuse?**

The answer is deliberately strict:

```text
HIT = namespace_authorized
   && format_supported
   && lengths_bounded_and_exact
   && compatibility_fingerprint_exact
   && prompt_identity_exact
   && declared_integrity_profile_verified
   && engine_import_succeeds

otherwise: MISS_RECOMPUTE
```

## Reading path

- [Executive findings](executive-findings.md) gives the decision summary.
- [Observed CachyLLama](observed-cachyllama.md) documents exact keys, paths, native layouts, lifecycle, failure behavior, and concurrency.
- [Observed llama.cpp](observed-llama-cpp.md) separates sessions, sequence snapshots, prompt cache, and manual slot files from a true persistent prefix cache.
- [Comparative designs](comparative-designs.md) evaluates LMCache, SGLang HiCacheFile, and vLLM keying.
- [HaloFPX redesign](halofpx-redesign.md) defines the proposed architecture.
- [Integrity invariants](integrity-invariants.md), [failure recovery](failure-and-recovery.md), and [security](security.md) make the miss/recompute rule executable.
- [Validation](validation.md) describes the included test suite.

Exact commits and source paths are in [Source matrix](source-matrix.md).
