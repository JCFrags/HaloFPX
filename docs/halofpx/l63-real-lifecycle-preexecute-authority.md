# L63 real-lifecycle pre-execute authority

**Status:** `[VERIFIED] NOT PROMOTED`

L63 stopped at its source/design gate. No stories15M model was loaded and no
production or disposable service was mutated.

## Result

The source audit accepted ADR-0049 as the minimum honest design, but the
existing accepted foundations cannot produce the required evidence by adding
observational logging:

- L44 derives `connection_epoch` from the mutable CAPS `server_nonce`
  (`ggml/src/ggml-rpc/ggml-rpc.cpp`, mutable begin), not from the RPC
  connection.
- The RPC server has per-connection allocation ordinals/maps, but L44 does not
  expose a negotiated epoch sourced from the live connection and server
  allocation topology. A server nonce and allocation ordinal are not the
  distinct epochs required by ADR-0049.
- A recorder created only after successful L44 begin cannot authenticate a
  begin refusal or pre-session transport. It must exist before entering begin,
  then bind to the accepted session or terminate with the exact refusal.
- L44's begin reads `ggml_backend_sched_authority_admission`, which is
  non-consuming but is not handle-bound and may return the pre-prepare chain
  root. L63 requires a handle-bound snapshot of the actual prepared split.
- `test-halofpx-rpc-mutable-authority` enables one scheduler authority at
  execution sequence 1, reads admission before preparation, and reuses that
  admission for independent attempts, graph UIDs, and sequences. Those cases
  remain useful handler/lifetime tests, but do not prove one composed
  L42-to-L44-to-graph-compute execution identity.
- The client global mutable-session map is protected by a process-global mutex
  that is held across network operations in the compute/finish paths. An
  attempt-scoped recorder inserted there would not prove concurrent isolation
  and could deadlock callbacks.
- Existing response instrumentation begins at authenticated execute and uses a
  process-global event counter. It cannot represent the required L44 begin
  lifecycle or the AUTH_COMPUTE/AUTH_RECOMPUTE transport boundary.

A partial implementation of the prepared snapshot and epochs was inspected
locally and removed before qualification because it did not close the
protocol-wide lifetime, concurrency, and server-validation requirements.
There is no runtime candidate in the terminal tree.
L61 harvesting remains unchanged.

## Smallest remaining implementation

Implement ADR-0049 as one capability-negotiated protocol revision:

1. add a true per-connection epoch and server allocation-topology epoch;
2. expose a non-consuming handle-bound L42 prepared-admission value;
3. create the attempt recorder at execution arm, before L44 begin;
4. move event state and locking into the attempt/session, with no global lock
   held during I/O;
5. instrument AUTH_COMPUTE, AUTH_RECOMPUTE, and EXECUTE with observed partial
   byte/EOF/errno states; and
6. rebuild the real fixture so every graph execution has its own scheduler
   preparation and L44 lifetime.

Only after the real two-host no-model composed fixture reaches authenticated
execute, proves reachable refusals and transport failures, and passes
independent review may a stories discriminator run.

Because these are prerequisites to trustworthy event identity and
cardinality, the real composed qualification could not be honestly built from
the accepted APIs without revising the negotiated protocol/lifecycle
boundaries. The gate therefore failed before stories. No inference,
model-correctness, or cache conclusion follows.

## Production and cleanup

Read-only closeout found exact unchanged production:

- nimo-2 system unit `minimax-m27-rpc-worker.service`, PID 1535639,
  `/system.slice/minimax-m27-rpc-worker.service`, port 50052,
  `NRestarts=0`;
- nimo-1 system unit `minimax-m27-q6-server.service`, PID 2356329,
  `/system.slice/minimax-m27-q6-server.service`, port 8081, HTTP 200,
  `NRestarts=0`.

No L63 disposable unit, port, key, build, model process, or evidence root was
created.
