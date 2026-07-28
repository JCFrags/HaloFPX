# L62 pre-execute admission discriminator

**Status:** `[MEASURED] NOT PROMOTED`

L62 did not consume the authorized stories15M run. The prerequisite failed
before model launch, so no claim is made about the L61 first-chunk failure.

The candidate enumerated the coordinator RPC graph-compute decisions preceding
`RPC_CMD_HALOFPX_GRAPH_AUTH_EXECUTE` and added a bounded HMAC verifier. Its
closed verifier tests passed, and exact ROCm Linux binaries compiled. The
candidate then attempted the required real two-host no-model qualification
using two isolated user units:

- nimo-1 `halofpx-l62-rpc1.service`, port 50262;
- nimo-2 `halofpx-l62-rpc2.service`, loopback port 50263.

Both exact RPC binaries became active with ROCm0. A fresh identical protected
130-byte key was installed mode 0600 on both hosts and its equal SHA-256 was
verified without retaining key bytes. The real
`test-halofpx-rpc-mutable-authority` fixture could arm the RPC execution but
failed at `ggml_backend_rpc_halofpx_mutable_begin` before any mutable CAPS
request or pre-execute record. Removing the fixture's redundant one-shot
scheduler-admission read did not change that boundary. Because the required
real emission/admission path was not qualified, independent pre-runtime review
did not authorize the stories run.

Independent review also rejected the implementation candidate on source
grounds:

- focused tests constructed verifier records rather than exercising the real
  refusal seams;
- a failed `send_rpc_cmd` record claimed `request_sent=1`, which is stronger
  than the observable authority;
- `connection_epoch` was populated from the graph-auth server nonce rather
  than an explicitly named connection/allocation epoch;
- graph-compute instrumentation exposed aggregate mutable-session state but
  did not cover L44 begin/register/exclude/commit/abort refusal reasons.

The unqualified runtime candidate was removed. L61's accepted host-bound
harvesting foundation is unchanged. The smallest remaining work is a new,
source-reviewed design that exposes L44 lifecycle refusal authority and an
honest transport-send state, then proves it through the real no-model
composition before any stories execution.

Production was read-only and remained exact: nimo-2 system worker PID 1535639,
port 50052, `NRestarts=0`; nimo-1 system coordinator PID 2356329, port 8081,
HTTP 200, `NRestarts=0`. Both L62 disposable units ended
`not-found/inactive/dead/MainPID=0`; keys, streams, source trees, archives,
ports, and local packaging were removed.
