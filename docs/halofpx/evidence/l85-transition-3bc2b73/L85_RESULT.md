# L85 terminal result

Status: **NOT PROMOTED**

The resolved-storage census implementation passed the bounded offline gates and
independent source review. The single authorized primary warmup attempt then
stopped before admission sealing. The exact durable client result was:

`version=1|status=failed|branch=l42_resolved_census_refused|typed_reason=0|execution_sequence=1|pending=1|ggml_status=-1`

The terminal grammar recorded `begin=1`, `abort=1`, and zero L42 admission,
L44 begin, register, exclude, prepare, commit, transport, or successful terminal
events. No authenticated server attempt was opened, so no server authority was
expected or published. The warmup kill gate held: no workload token, cache
capture, stage, commit, restore, or state comparison ran.

The retained journal does not identify a particular canonical entry or a
specific projection-refusal reason. Therefore L85 proves that the exact primary
graph is rejected by the new resolved projection before sealing, but it does
not prove which of the projection invariants rejected it. No retry or semantic
correction was performed.

The controller removed its disposable units, keys, source/build paths, and
evidence paths. Production recovered worker-first/coordinator-second. Read-only
closeout found the exact named units active/running, unique listeners on ports
50052 and 8081, the expected production argv, and coordinator HTTP 200.
`NRestarts=0` on both newly started units; no automatic restart occurred during
this recovery.

No cache correctness, model correctness, or performance conclusion is admitted.
