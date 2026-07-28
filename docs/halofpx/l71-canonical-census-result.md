# L71 canonical scheduler census result

Status: **PASS — safe default-off source retained**

Base: `09fa1f4313c81ca9e629af6772f2108fb7ab8bf7`

L71 replaces the scheduler/runtime census double-walk with one immutable,
per-RPC-backend canonical list built from scheduler source authority before
admission sealing. Each entry binds the destination backend, pointer-independent
source identity, root/copy provenance, class, exact wire role and ordinal,
registration/exclusion disposition, and the runtime tensor used by L44.

Only entries with identical stable identity, destination/runtime tensor, and
semantics collapse. A stable-identity or runtime-tensor collision with different
semantics fails plan construction. Runtime duplicate refusal was not weakened.
Prepared admission counts and the authenticated census projection are derived
only from the canonical list; `process_ubatch` exports and consumes that same
list once, without re-walking roots or copies.

Focused qualification:

- Windows compile-off `rpc-server` target: PASS.
- Changed `llama-context.cpp` translation unit: PASS; the final `llama.dll`
  link then reaches the pre-existing unresolved feature-off
  `ggml_backend_rpc_halofpx_mutable_negotiate_preflight` dependency.
- Scheduler authority self-test: PASS, exact mask `0x7ffff`, including repeated
  admitted-copy collapse and conflicting duplicate refusal.
- `git diff --check`: PASS.
- No model, Stories, RPC transport, production, cache, or performance operation
  was performed.

Independent review initially found and rejected two cross-module projection
defects (full-entry root versus retained RPC projection, then exclusion source
role versus wire exclusion role). Both were corrected before retention. Final
review found no correctness/security P1/P2 and accepted ownership/lifetime,
pointer-independent ordering, exact collision/refusal behavior, sole-source
sealing/iteration, and feature-off gating.

