# L40 RPC authenticated graph authority

Status: `[MEASURED] PASS` for the narrow RPC foundation. L40 did not access the
primary artifact or mutate production. It does not authorize scheduler-copy
authority, mutable-input or `SET_TENSOR` role authority, a primary run, cache
promotion, tuning, or L41.

## Accepted contract

The runtime-default-off `halofpx.rpc-graph-authority.v1` path adds four closed
RPC commands after the existing state family: CAPS, graph prepare, recompute
prepare, and execute. The ordinary graph-compute/recompute wire remains
unchanged unless `HALOFPX_RPC_GRAPH_AUTH=1` is present on the client and server,
both possess the same 32-byte channel key, and the authenticated CAPS exchange
agrees on exact version and bounds.

Authenticated records use explicit canonical little-endian codecs, exact sizes,
hard graph/tensor/node limits, a distinct HMAC domain, client attempt nonce,
server nonce, nonzero graph UID, and a monotonic nonzero execution sequence.
Malformed, trailing, duplicate, unknown, out-of-order, oversized, stale, or
tampered authority fails closed.

Immediately after the real client `serialize_graph`, the client derives a
canonical graph digest. The server reconstructs the graph first, independently
walks its actual nodes and recursive leaf postorder, and rejects extra or
out-of-order wire tensors. Both digests cover ordered node/tensor indices,
operation and parameters, type, dimensions, strides, flags, complete source
null bitmap and edges, every view edge/offset, plus connection-local allocation
ordinal and buffer-relative offset.

The allocation ordinal is assigned at each successful real `ALLOC_BUFFER` on
both ends; it is not inferred from graph encounter order. Client socket reuse
cannot inherit a stale ordinal sequence.

Graph execution is two phase:

1. the server reconstructs and compares authority without computing, then
   returns an authenticated preparation receipt;
2. only after the client validates that receipt does it send a separately
   authenticated EXECUTE command.

Compute and recompute retain the same graph digest and empty L40 transcript root
under one attempt, while advancing execution sequence. CAPS reset clears old
lineage. Allocation or legacy graph replacement discards lineage; an
intervening tensor mutation invalidates a prepared receipt. Execution consumes
preparation before invoking the backend, so an ambiguous backend failure cannot
replay the same authority.

## Qualification

Both Release Linux configurations compiled:

- ROCm + RPC + HaloFPX local-state support enabled;
- RPC with HaloFPX local-state support compiled out.

The focused C++ self-test passed 18 checks covering canonical HMAC validation,
tamper, wrong sequence, zero sequence/UID, duplicate and unknown graph IDs,
node bounds, receipt tamper, exact codec round trip, trailing-byte rejection,
wrong version, and allocation-ordinal sensitivity.

One isolated nimo-1 ROCm RPC worker on port `50240` executed a graph prepare,
authenticated receipt, execute, recompute prepare, and second execute from
nimo-2. Client and server independently reported digest
`ef2fecddf9713b2b63e02391de1fbc351f6538e46eacb3e268bc08fbfe0a041d`,
UID `2`, and sequences `1` then `2`. Both executions returned the same expected
output (`first0=second0=0.938476562`; exact byte equality).

The same forced-RPC test with the runtime feature unset returned identical
output and added zero graph-authority records (`14` before and `14` after),
demonstrating ordinary runtime admission. A feature-on client against a
feature-off disposable endpoint and a wrong-key client both failed closed.

Source and binary identities:

- `ggml-rpc.cpp` SHA-256:
  `aacd2f2cc7aa4e348e834169120084af1a5eccd1faf663e844a725d7409374b9`;
- focused test source SHA-256:
  `98e56fe98b3bd77016cebe49145c4c493812bb60035583267aa1e6b52addcf83`;
- final RPC server SHA-256:
  `c7a1677dfd88b976eb6fe1582c1ddb2d556a23c410f22657448368cdcc8fcb66`;
- focused test binary SHA-256:
  `0a83f86c37e1c3031b354c2d88b385a9e09704e090c1f8c8caa2f73489f2197b`.

## Review and boundary

Independent adversarial review initially rejected native record encoding,
graph-encounter allocation identity, client-inherited server ordering, and a
post-compute receipt. It accepted the final corrected implementation after
explicit LE codecs, real allocation ordinals, server-derived ordering,
two-phase preparation/execution, and stale-lineage invalidation were verified.

Scheduler copies, mutable-input discovery, tensor-update role transcripts,
expert/Q8/flash-attention coverage, and state semantics remain deliberately
outside L40. No claim about them follows from this PASS.

## Production and cleanup

Production remained continuously unchanged. At closeout the nimo-2 system
worker was active/running under
`/system.slice/minimax-m27-rpc-worker.service`, PID `1535639`, port `50052`,
`NRestarts=0`. The nimo-1 system coordinator was active/running under
`/system.slice/minimax-m27-q6-server.service`, PID `2356329`, port `8081`,
HTTP `200`, `NRestarts=0`.

Disposable ports `50240` and `50241`, user units, the nimo-1 binary root, and
both nimo-2 L40 build roots were removed.

