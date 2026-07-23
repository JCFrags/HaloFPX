# L27 RPC worker epoch and model-residency diagnosis

Date: 2026-07-23

Base: `9b45bb9c844ec224fbd6fc3b39bdfe23eec11ee3`

Outcome: **PASS — SOURCE-BACKED AUTHORITY AND FAIL-CLOSED GUARD**

## Finding

[VERIFIED] A worker restart destroys the RPC server's model buffers, tensor
maps, graph authority, and process-local remote identifiers. A coordinator
model residency retains the socket and `remote_ptr` values that created those
allocations. A fresh HELLO/HFXCAP2 probe is independent readiness evidence; it
does not update or validate the old residency's connection or allocations.

[VERIFIED] L20/L22/L23 post-restart restore ran in a new coordinator process and
loaded the model again. L24/L26 uniquely used the `diagnostic` sequence. Its
static `resident_init` retained the model across capture and restore
`invoke_mode` calls, so its claim of one material load was literal but its
restart lifecycle was inadmissible.

## Disposable discriminator

[MEASURED] The accepted 19,077,344-byte stories15M Q4_0 fixture, SHA-256
`66967fbece6dbe97886593fdbb73589584927e29119ec31f08090732d1861739`,
ran on isolated port 50187 with F16 K/V and flash attention off. This tuple
tests RPC lifetime/controller semantics, not primary KV performance.

Worker A was PID 2257439, InvocationID
`8654d450c95b43dba81f7474405d8ce1`. Capture completed with token 4245,
1,156 components, 5,197,824 worker bytes, and aggregate
`ed62ec5b04cc53ce870ddd6df1d8eefc10a0e4f44e2a699deb879ebf4462fdbc`.
After worker A stopped, worker B started as PID 2257496, InvocationID
`a09e1eafe62143e1b3a9489105cbb6af`.

The still-resident coordinator reproduced L26 exactly: its attempt to create a
new context after the worker restart aborted in RPC buffer allocation with
`Remote RPC server crashed or returned malformed response`.

A new coordinator process then loaded the same model against worker B and
restored successfully. It reproduced token 4245 and the exact capture
control/local/manifest digests, 1,156 components, and 5,197,824 worker bytes.
This proves the lifetime dependency without the primary artifact.

## Guard and qualification

The old same-residency diagnostic entry point now refuses before starting a
worker or loading a model. A focused standalone lifecycle validator rejects an
unchanged/malformed worker InvocationID, unchanged/invalid coordinator PID, or
a restore model load not proven after worker restart. It admits the exact
changed-worker/changed-coordinator/ordered-load tuple in unit tests, but no
runnable fresh-residency primary path consumes it yet. Any future admitted
runner must bind this authority before staging.

Focused controller and guard tests pass 71/71. Production was never stopped or
mutated and was HTTP 200 at closeout. Exact closeout evidence verifies both
disposable units inactive, port 50187 absent, and the explicitly listed source,
state, and key paths absent. No broader rendezvous-path claim is made.

The immutable evidence root is
`C:\Users\britt\Documents\Custom_Inference_Project\sources\halofpx\l27-rpc-epoch-20260723`.
Its ten files total 375,439 bytes and have canonical
relative-path-plus-NUL-plus-content SHA-256
`f1bdacfb0ad216eb4bfdc05e9c8e15f170127bbf07b47d615c39642b0e168588`.

## Boundary

L27 does not retry L26, access the primary artifact, implement transparent RPC
recovery, alter cache semantics, tune performance, enable caching, or authorize
L28.
