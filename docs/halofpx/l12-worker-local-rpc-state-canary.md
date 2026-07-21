# L12 worker-local RPC state protocol and two-rank canary

Date: 2026-07-21

Implementation commit: `6444d1e173aa7c0d5bea6c2b3539d1fd936f3f65`

Outcome: **ACCEPT FOR THE DISPOSABLE SMALL-MODEL CANARY ONLY**

## Decision and ownership boundary

[VERIFIED] ADR-0040 was frozen before implementation. The accepted design adds
a Linux-only, compile-default-off and runtime-default-off RPC command family:
CAPS, CAPTURE, STAGE, COMMIT_APPLY, and ABORT. The worker owns its configured
cache root, logical-rank identity, tensor-range persistence, immutable local
objects, staging context, validation, and live worker-buffer apply. The
coordinator retains the llama context, request/token boundary, coordinator-local
control and local state, greedy sampler, all-rank decision, and cold-recompute
authority.

[VERIFIED] State operations authenticate the full bounded request transcript
with HMAC-SHA-256 and return only bounded version, status, identity digest,
object digest, nonce, counts, and byte totals. No state bytes are returned by
the new commands. Stable object selection binds model, compatibility root,
plan, topology, world/rank/placement, generation/checkpoint, token boundary and
prefix, and the canonical component/tensor manifest. A fresh attempt nonce and
channel binding fence each stage/commit attempt.

[VERIFIED] CAPTURE writes worker-owned component ranges to a temporary file,
validates them, and publishes one immutable object by no-replace rename. STAGE
opens and validates the local object and loads only disposable staging buffers.
COMMIT_APPLY requires the matching READY nonce and identity. Legacy mutation,
missing/corrupt/mismatched objects, replay, timeout, partial validation, or
apply uncertainty discard the attempt. The canary destroys its disposable
context and recomputes from a clean tokenization on any restore failure. The
implementation does not claim crash-atomic live mutation.

## Exact protocol boundary

| Item | Accepted value |
|---|---:|
| wire version | 1.0 |
| maximum request | 1 MiB |
| maximum response | 256 bytes |
| fixed identity header | 480 bytes |
| component descriptor | 112 bytes |
| maximum components | 4,096 |
| maximum bytes per component | 1 GiB |
| maximum bytes per rank object | 64 GiB |
| READY-to-COMMIT timeout | 5 seconds |
| process-lifetime accepted-stage nonce ledger | 4,096 entries, then fail closed |

The compile gate accepts only little-endian Linux. `GGML_RPC_HALOFPX_LOCAL_STATE`
defaults OFF. Even in an enabled build the worker requires every runtime switch:
explicit enable, absolute protected root, protected key/channel file, rank,
world size, and key generation.

## Exact-commit qualification

[MEASURED] Both nodes built detached exact clones of implementation commit
`6444d1e173aa7c0d5bea6c2b3539d1fd936f3f65`. The feature-on build supplied the
protocol probe, coordinator canary, and disposable worker. A separate
`GGML_RPC=ON` / local-state-OFF build exposed no `--halofpx` flags, and a true
`GGML_RPC=OFF` / local-state-OFF build produced `libllama` successfully.

The small fixture was the 15M Q4_0 model with SHA-256
`66967fbece6dbe97886593fdbb73589584927e29119ec31f08090732d1861739`.
The coordinator ran on nimo-2 and the rank-1 worker on nimo-1 at disposable
endpoint `10.44.0.1:50174`, with layer split, tensor split 1, three offloaded
layers, context 256, parallel 1, and no mmap.

| Path | Result |
|---|---|
| uninterrupted capture suffix | `471,22049,297,278,25013,29889,940,471` |
| worker-local restart restore suffix | exact match |
| clean cold recompute suffix | exact match |
| all three suffix file SHA-256 values | `06a0b15c33af689aae89417257e012bc86745d57eeb4b26a4e323f7e60af998c` |
| worker object | 4 components, 11,520 bytes |
| immutable object content SHA-256 | `94fe9f268e8ee86604d0a0d33bb5f1e2ed07a08c624f1696f6cf0cd5a5f0e2fd` |
| worker object mode/owner | `0600`, service owner |
| capture state-operation GET/SET count | 0 / 0 |
| restore READY-to-APPLY GET/SET count | 0 / 0 |

[MEASURED] Worker debug logs show worker-local buffer allocation and four
server-side `COPY_TENSOR` operations followed by `stored`. Restore shows a
worker-local staging allocation followed by adjacent `ready` and `apply`.
Neither state-operation window contains legacy `GET_TENSOR` or `SET_TENSOR`.
Those server-side copies do not carry tensor bytes on the RPC control plane.

[MEASURED] The success path created and read the immutable rank-1 object and
accepted READY before COMMIT_APPLY. A missing worker object and a truncated
worker object each returned a worker-stage failure and exact cold suffix. Plan
and topology mismatches each failed authenticated coordinator receipt checking
and exact cold-recomputed. Focused probes also rejected stage replay, commit
replay, referenced-buffer destruction before commit, READY timeout, invalid
range, invalid shape, and invalid authentication without terminating the
worker.

## Review, evidence, and cleanup

[VERIFIED] The independent adversarial review is ACCEPT with no remaining
blocking finding. It specifically reconciles feature-off linking, pre-allocation
framing limits, untrusted descriptor validation, fresh key/nonce and replay
fencing, response transcript/status binding, coordinator receipt integrity,
actual component-manifest identity, and pending-state invalidation on legacy
mutation.

The raw evidence bundle is retained on nimo-2 at
`/var/tmp/halofpx-worker-local-state-evidence-20260721-v1.tar.zst`, SHA-256
`d4fe0c86f3b87248cc1e06e6ddfd666940ea204c584ea045e6ba401b73f41196`.
It contains the exact source bundle and patch, build configurations, binary and
artifact hashes, protocol and canary logs, worker journals, feature-off help,
service definitions/status, production snapshots, and cleanup receipts.

[MEASURED] Both disposable services were stopped; disposable ports, roots,
keys, exact clones, and debugging builds were removed. The known-good nimo-1
coordinator remained PID 2053029 on port 8081 and the nimo-2 RPC worker remained
PID 1186396 on port 50052, with their original start times.

## Bounded next gate

This milestone does not admit the 160 GB primary model. Project Lead may next
authorize one disposable primary-model canary using the same exact commit and
identity tuple, isolated roots/ports/services, preserved production endpoints,
and these stop gates:

1. preflight exact model and binary hashes, free-space and root permissions,
   rank/world/plan/topology/placement identity, and protected key/channel;
2. one uninterrupted/cold/worker-local-restore exact-suffix comparison;
3. journal proof of worker-local objects, all-rank READY/commit, and zero state
   payload GET/SET during the persistence operations;
4. one missing/corrupt worker object and one plan or topology mismatch, each
   requiring clean cold recomputation;
5. independent review before any production enablement or performance claim.

Eviction, shared/prefix reuse, production enablement, broad fault matrices,
cable faults, and final performance claims remain explicitly outside this
gate.
