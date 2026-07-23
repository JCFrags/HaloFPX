# L24 primary restored-state discriminator result

Date: 2026-07-23

Base: `cb1913ca233acf8661530622720b411bc0e5d5aa`

Outcome: **NOT PROMOTED — CONTROLLER SSH HANDOFF FAILURE**

## Result

[VERIFIED] The one authorized controller-managed transition used the pinned
159,873,097,824-byte primary artifact with SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.
The frozen command retained 1,129 prompt tokens, the 1,128-token boundary,
one generated token, Q8_0 K/V, flash attention on, context 4096, batch and
ubatch 512, seed 1234, temperature zero, explicit `RPC0,ROCm0`, layer split,
and tensor split `1,1`.

[MEASURED] The single model residency loaded successfully and capture
completed. RPC material allocation included the expected
80,950,550,528-byte worker request. Capture stored 64 worker components and
2,454,528 bytes. Its descriptor/content aggregate was
`014a1024f13225a3f7bd7bba6be43dce1106a0354d68b5043f284263cce19bc9`.

[MEASURED] The coordinator created the capture artifacts and
`capture-ready`, but the local maintenance child remained blocked in its
controller-owned SSH `test -f` subprocess. Direct read-only observation found
the marker. Because the controller could not authoritatively observe its own
handoff, the run was aborted under the stop-on-ambiguity gate before worker
restart and restore. The buffered canary result was not retained, so L24 does
not claim an authenticated reference token even though capture completed.
There are no stage or post-apply aggregates and no restored token to
interpret. L24 was not retried.

## Controller reliability finding

[VERIFIED] The same failure mode affected the first emergency-recovery
hostname probe: a controller-owned SSH subprocess hung despite successful
concurrent read-only SSH checks. After Project Lead explicitly authorized
terminating only the identified stuck subprocess, the registered exit
recovery advanced.

[INFERENCE] The closed controller lacks a sufficiently bounded
transport-level deadline for individual SSH subprocesses. An otherwise
successful remote operation can therefore block the child handoff or recovery
before the controller's higher-level state machine observes a result. This is
a controller reliability defect and independently makes L24 non-promotable.
No controller correction or second primary attempt is authorized here.

## Production and cleanup

[VERIFIED] Recovery restored nimo-2 worker first at 12:37:49 PDT: PID
1415055, exact production RPC command, port 50052, `NRestarts=0`. Only after
that readiness was verified did nimo-1 coordinator start at 12:37:50 PDT:
PID 2236922, exact standard UD-Q6 command/model, port 8081, `NRestarts=0`.
The model completed loading at 12:41:17 PDT and HTTP returned 200.

[VERIFIED] All disposable units were inactive/collected. Port 50184 and every
manifest-owned L24 key, state, evidence, rendezvous, source, and build path
were absent. The primary artifact was not deleted.

The immutable evidence root is
`C:\Users\britt\Documents\Custom_Inference_Project\sources\halofpx\l24-primary-20260723\primary-diagnostic-20260723T123200`.
Its 17 files total 503,091 bytes and have canonical
relative-path-plus-content tree SHA-256
`c333749cb92066ec671bd0f3d6b783250616d13cbf93a74dc6db4cb40af97940`.

## Boundary

The cache remains default-off. L24 makes no restored-state root-cause,
correctness, promotion, performance, or production-cache claim. It does not
authorize a retry, a semantic fix, or L25.
