# L28 fresh RPC residency diagnostic runner

Date: 2026-07-23

Base: `5616abb2c19c1611c3852575270ad41b43085921`

Outcome: **PASS**

## Executable contract

[VERIFIED] The controller now has a no-production `disposable` execution mode.
It validates the closed L28 manifest and exact Popen argv, snapshots production,
provisions protected disposable keys, runs the child without stopping
production, cleans every admitted path, and requires the final production
snapshot to equal the initial snapshot.

[VERIFIED] The diagnostic child uses two coordinator processes/model loads.
Capture coordinator A must be gone before worker A is stopped. Worker B must
have a distinct PID and InvocationID and exact HFXCAP2 readiness. Restore
coordinator B loads its model against B, publishes `model-ready`, and blocks
before artifact validation/staging. The runner then re-reads B's identity,
executes the L27 validator, and publishes `restore-authorized` only on success.

## Disposable qualification

[MEASURED] The accepted stories15M Q4_0 fixture was used at 19,077,344 bytes
and SHA-256
`66967fbece6dbe97886593fdbb73589584927e29119ec31f08090732d1861739`.
F16 K/V and flash attention off are non-representative of primary kernel
performance and were used only for RPC lifetime semantics. The 1,129-token
prompt decoded in three bounded chunks with maximum chunk 512.

Capture used worker PID 2278153 / InvocationID
`d8b852f1d7394276b5ed5ab0c18743b8` and coordinator PID 1447731. Restore used
worker PID 2278269 / InvocationID `74acffcb6f1949449f74103bf11957e2`
and coordinator PID 1447819 / InvocationID
`e47f95e2bb524d3c80f8d25392373832`.

Before A stopped, an HMAC-authenticated sidecar bound worker A's PID and
InvocationID, coordinator A's PID, and captured worker-object digest
`f3ad268fc87a6c90d06a517e82a948dbbbd312bcb32300babb842483f885e1b0`.
The runner verified that exact binding after model B became ready and before
publishing restore authorization.

Capture and restore both produced token 4245. Token and decoded-text hashes
were exactly equal. Capture/stage/apply agreed on 1,156 components, 5,197,824
worker bytes, and aggregate
`ed62ec5b04cc53ce870ddd6df1d8eefc10a0e4f44e2a699deb879ebf4462fdbc`.
Coordinator control, local, and component-manifest digests also matched.
Legacy state-window GET_TENSOR/SET_TENSOR count was zero.

## Corrections during qualification

The first preserved attempt found a fixture configuration bug: the derived
artifact directory was not refreshed after selecting the L28 root. The second
attempt completed the full exact lifecycle but exposed a cleanup attribution
bug when checking an already-absent worker unit while the other admitted epoch
owned the shared port. Both were corrected narrowly. The final third run
passed the lifecycle, controller snapshot reconciliation, and complete cleanup,
but adversarial review rejected its unauthenticated epoch audit. The fourth run
added and exercised the authenticated object/epoch sidecar. All earlier
evidence was preserved and was not treated as the final PASS.

Production remained continuously active: nimo-1 coordinator PID 2248156,
nimo-2 worker PID 1422619, HTTP 200, and both NRestarts zero. No primary
artifact was accessed or loaded.

## Evidence

Final immutable evidence:
`C:\Users\britt\Documents\Custom_Inference_Project\sources\halofpx\l28-fresh-residency-20260723-r4`

- files: 26
- bytes: 850,482
- canonical relative-path-plus-NUL-plus-content SHA-256:
  `b10c09d6b8c7e9b182ae9205c35d6ab92b3a87434cb5beb1af1d721573301749`

Preserved non-PASS attempts:

- `l28-fresh-residency-20260723`
- `l28-fresh-residency-20260723-r2`
- `l28-fresh-residency-20260723-r3`

## Boundary

L28 does not authorize a primary run, transparent RPC recovery, cache
promotion, performance claims, production mutation, or L29.
