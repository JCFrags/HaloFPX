# L20 no-production execution-contract result

Date: 2026-07-21

Base: `7cb42be0ba3f45863c418fb9befd5d306f5ce893`

Outcome: **NOT PROMOTED — EVIDENCE/OWNERSHIP CONTRACT INCOMPLETE**

## Verified lifecycle result

[MEASURED] A disposable 15M Q4_0 model on nimo-2 with an isolated nimo-1 RPC
worker proved the minimum current lifecycle uses three material-model
residencies, not six:

1. one load held capture/uninterrupted generation and a separate clean-cold
   context;
2. after a true worker restart, one load held exact restore plus separate
   corrupt-object and plan-mismatch cold-fallback contexts; and
3. after changing to a feature-off worker, one load held the matched cold
   control.

The worker PIDs were 2165752, 2165831, and 2165909. The 1,129-token prompt and
1,128-token boundary decoded as three bounded chunks with maximum 512. Every
uninterrupted, cold, restored, fallback, and mode-off continuation had token
SHA-256 `8833fa5b4ce8735bd0520e7eb2e969b26ae302b688c5f4f80c2fb58858af703f`.
Capture/restore state windows contained zero legacy GET_TENSOR/SET_TENSOR
operations. This validates the three-residency derivation but not the rejected
controller candidate.

## Terminal blocker

Independent review rejected the execution contract because it did not retain a
real early allocation-refusal case, did not bind every source/build/state path
into manifest-owned cleanup, could lose PID/InvocationID authority after
`systemd-run --collect`, and did not make every evidence-collection failure
fatal. The successful raw run also lacked its own production-before snapshot.
These are contract defects, not model/cache correctness failures.

The unaccepted controller and lifecycle source changes were removed. All L20
units, port 50197, keys, state roots, source roots, and build roots were removed.
The protected evidence archive remains at
`/var/tmp/halofpx-l20-evidence-20260721-v1.tar.zst`, mode 0600, SHA-256
`c9d50ecc1b92cedbe369812650149b5cf6328f46829a8a5c05e64c4869302ad4`.

Production was never stopped or restarted and closed healthy at nimo-1 PID
2144857/8081/HTTP 200 and nimo-2 PID 1305879/50052, both `NRestarts=0`.
The primary artifact was not read or loaded. L21 was not opened.
