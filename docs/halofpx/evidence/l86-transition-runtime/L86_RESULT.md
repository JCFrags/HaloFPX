# L86 terminal result

Status: **NOT PROMOTED**

The typed scheduler-projection diagnostic implementation passed the exact
feature-on/off compile gates, all eleven focused typed-reason fixtures, the
resolved-storage fixture, the durable L83 formatter fixture, the real
two-process no-model composed fixture, and independent review with no
correctness/security P1/P2.

The single authorized primary warmup discriminator stopped before admission
sealing with:

`version=1|status=failed|branch=l42_resolved_census_refused|typed_reason=4|backend_ordinal=1|candidate_index=487|provenance=1|disposition=1|root_class=1|role=11|role_ordinal=64|stable_tensor_id=5213|copy_slot=4294967295|copy_generation=0|logical_identity=258ede45fec7cd54d49e7437c50c8f7bd1f427a3a3dda2a9bbaa0605488d6a4f|storage_identity=0000000000000000000000000000000000000000000000000000000000000000|rpc_device=0|rpc_connection_epoch=0|execution_sequence=1|pending=1|ggml_status=-1`

Typed reason `4` is
`GGML_BACKEND_SCHED_PROJECTION_WRONG_DESTINATION_BACKEND`. The exact canonical
root candidate is assigned to destination backend ordinal `1`, while the
projection's admitted RPC backend set does not contain that destination. The
refusal occurs before the storage resolver, explaining the zero storage
identity, device, and connection epoch. No L42 admission, L44 session,
authenticated server attempt, workload token, cache capture, stage, commit, or
restore occurred.

Because no authenticated server attempt was opened, no server publication was
possible. The controller therefore retained the expected
`publication_journal_missing` custody classification separately; it is a
consequence of the pre-admission client refusal, not the forward cause.

The controller removed all disposable L48 units, keys, source/build roots, and
remote evidence paths. Production recovered worker-first/coordinator-second:

- coordinator PID `2825097`, InvocationID
  `d77a2401d9954b208bb49b32ffd493b0`, `NRestarts=0`, unique port 8081,
  HTTP 200;
- worker PID `2023878`, InvocationID
  `b1ba9535181d46f8a3446d21b0652b01`, `NRestarts=0`, unique port 50052.

No cache/model correctness or performance conclusion is admitted.
