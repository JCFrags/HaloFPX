# ADR-0045: RPC worker epoch bounds distributed model residency

- Status: accepted for the default-off diagnostic harness
- Date: 2026-07-23
- Base: `9b45bb9c844ec224fbd6fc3b39bdfe23eec11ee3`

## Decision

An RPC worker process/InvocationID change ends every coordinator model residency
which owns allocations on that worker. Restore may proceed only after a fresh
coordinator process loads the complete distributed model against the restarted
worker. The admitted order is:

1. capture and authenticate state;
2. terminate the capture coordinator residency;
3. restart the disposable worker and bind its new InvocationID;
4. start a different coordinator PID and load the complete model after that
   worker is ready;
5. only then validate, stage, and apply state.

HELLO and HFXCAP2 prove protocol/channel readiness of their own new connection.
They do not re-create model tensors, remote buffers, or validate handles held
by another still-resident coordinator. Transparent weight rehydration and RPC
reconnection recovery are outside this decision.

The legacy one-process `diagnostic` path is fail-closed before worker or model
startup. A standalone validator specifies and tests changed worker
InvocationID, changed coordinator PID, and proof that the restore model load
followed worker readiness. It is not yet wired to a runnable fresh-residency
primary path, which must consume that authority before state staging.

## Source authority

The client stores each RPC buffer as a server-returned `remote_ptr` plus the
socket that created it. The server constructs `remote_ptr` by casting its
process-local `ggml_backend_buffer_t`; buffer, tensor, and graph requests later
send that value back. The server owns its buffer and tensor maps only for the
life of that process.

The client socket cache is endpoint-keyed and weak, but live model buffers hold
strong socket references. A worker restart therefore leaves an old residency
holding the dead connection and process-local remote identifiers. The separate
readiness probe opens its own connection and does not replace those references.

## Lifecycle correction

L20, L22, and L23 used separate coordinator processes for their numbered
residencies: post-restart restore loaded a fresh model. L24 and L26 used the
special `diagnostic` sequence, whose static `resident_init` deliberately kept
one model alive across `invoke_mode(capture)` and `invoke_mode(restore)`.
Descriptions of that sequence as a valid one-load restore contract were
incorrect. A true worker-restart restore requires at least two material model
loads; the broader accepted lifecycle remains three residencies when its
feature-off control is required.

## Boundary

This is a fail-closed harness/lifecycle rule, not transparent RPC recovery or a
cache semantic change. The feature remains default-off.
