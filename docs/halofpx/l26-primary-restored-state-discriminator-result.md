# L26 primary restored-state discriminator result

Date: 2026-07-23

Base: `36b026d29454adc9cdd61baf387303c3e8d9f200`

Outcome: **NOT PROMOTED — POST-RESTART RPC CONTEXT-CREATION FAILURE**

## Result

[VERIFIED] The one authorized controller-managed transition used the pinned
159,873,097,824-byte primary artifact with SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.
The command retained the frozen 1,129-token prompt, 1,128-token boundary, one
generated token, Q8_0 K/V, flash attention on, context 4096, batch and ubatch
512, seed 1234, temperature zero, explicit `RPC0,ROCm0`, layer split, tensor
split `1,1`, and `HALOFPX_STATE_DIAGNOSTICS=1`.

[MEASURED] Capture completed and its output was durably flushed and
authenticated before restart. The reference token was 21549. Capture recorded
64 worker components and 2,454,528 bytes with descriptor/content aggregate
`014a1024f13225a3f7bd7bba6be43dce1106a0354d68b5043f284263cce19bc9`.
Coordinator control and local state were 15,048 and 2,301,688 bytes. The
authenticated control, local, and component-manifest digests were respectively
`2d614e8634f7f9defc4ed59f59b900490e021e382045c566109588bd288a0cbb`,
`7117319f7dc2b848d3ce3b35469aee3c62bc93a8262f88cb53949e7bbc5ceaca`,
and `7ad364fb0a047ca8db745c439bf8ad3f6fe6b92ae56f7b9f632caa88e47c69b3`.

[MEASURED] The capture worker was PID 2247768. It was stopped and replaced by
the true restart worker PID 2247899. Both exact HELLO/HFXCAP2 probes admitted
RPC protocol 4.0.1, state protocol 1.0, rank 1/world 2, endpoint
`10.44.0.1:50184`, and the same authenticated channel binding.

[MEASURED] Before staging or live application, the coordinator aborted while
creating the post-restart context. The retained failure is
`Remote RPC server crashed or returned malformed response`, originating at RPC
buffer allocation during KV-cache/context construction. The disposable
coordinator exited by SIGABRT/core dump. No stage aggregate, apply aggregate,
restored token, or restored suffix exists. This localizes the observation only
to the post-restart context-creation boundary; it does not establish why the
RPC response failed.

The observed capture timing was 5,461.353 ms prompt, 115.710 ms state capture,
and 73.454 ms for the one reference token. These are diagnostic evidence only,
not a performance result.

## Transport, state, and allocation evidence

[VERIFIED] The controller and child retained 849 bounded SSH operation records.
No SSH operation timed out. The failing model session returned a typed command
failure after 202.633 seconds, inside its frozen 1,800-second deadline. The
failure therefore differs from the L24 transport hang.

[MEASURED] The capture worker journal retained the accepted
80,950,550,528-byte primary material allocation and the 64-component
local-state publication. The capture state operation itself used the
worker-local protocol. Model loading and ordinary graph execution used normal
RPC tensor operations outside that state window; they are not misreported as
state-page transfers. Restore state operations were never reached, so L26
makes no restore-window GET/SET claim.

## Recovery and cleanup

[VERIFIED] The controller cleaned the disposable worker/canary units, port,
keys, state, evidence, rendezvous, source archives, and build roots. All named
disposable units are inactive/dead with MainPID zero and every manifest-owned
path is absent.

[VERIFIED] Production recovery was worker-first. Nimo-2's exact
`minimax-m27-rpc-worker.service` started at 13:31:03 PDT and is active/running
in `/system.slice/minimax-m27-rpc-worker.service`, PID 1422619, listener 50052,
`NRestarts=0`. Nimo-1's exact `minimax-m27-q6-server.service` started second at
13:31:04, is active/running in its named system cgroup with PID 2248156, serves
the standard UD-Q6 model on 8081, returns HTTP 200, and has `NRestarts=0`.

During coordinator loading an independent read observed transient
inactive/dead properties and HTTP 503 while listeners were visible. Listener
visibility was not accepted as recovery, no duplicate process was started, and
the final exact unit/cgroup/PID/command reconciliation above supersedes that
transient observation.

The immutable evidence root is
`C:\Users\britt\Documents\Custom_Inference_Project\sources\halofpx\l26-primary-20260723\primary-diagnostic-20260723T_current`.
Its 25 files total 1,162,756 bytes and have canonical
relative-path-plus-NUL-plus-content SHA-256
`8a60129fbd69934bfaf989bf64076d7abf0c555dd742a1680ea077e8a3c14d43`.

## Boundary

L26 is terminal NOT PROMOTED. It was not retried. The result does not establish
a cache semantic root cause, change cache behavior, enable production caching,
make a performance claim, implement a correction, or authorize L27.

