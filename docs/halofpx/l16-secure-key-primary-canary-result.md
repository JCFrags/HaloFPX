# L16 secure-key primary canary result

Date: 2026-07-21

L15 terminal closeout: `0db5a56111e1ae610e169511ab6f6353e5a1c0ea`

Secure-key prerequisite: `47cd4cb6ecc03b6a24cc0b0083e8d045515003fb`

Binary pin: `7b443ea37ccdab5e044950e3afb98621ab39870b`

Outcome: **NOT PROMOTED — TERMINAL CURRENT-PLACEMENT RPC0 ALLOCATION BLOCKER**

## Decision

[MEASURED] The L16 no-production prerequisite passed before production
mutation. The controller then performed exactly one production transition. The
secure key and exact application readiness gates passed, but the first capture
canary failed while loading the model: the current L16 capture placement
attempted one monolithic RPC0 allocation larger than that worker's total and
free capacity. The controller restored production. No retry was performed.

This is not evidence of insufficient aggregate two-host capacity, unavoidable
model incompatibility, local-state overhead, or a proven optimal split failure.
No inference setting was changed to investigate it. L16 does not admit capture,
cold, restore, fallback, exact-continuation, state-transfer, object, timing,
I/O, throughput, or performance results.

## Secure-key prerequisite

[VERIFIED] L16 removed key creation from the maintenance child. Before
`shutdown()` and the first production mutation, the host-bound controller now:

1. generates a fresh 128-hex-character key as two LF-terminated 64-character
   lines (130 bytes total);
2. transports the bytes over binary stdin to exact argv
   `install -m 600 /dev/stdin /var/tmp/halofpx-l16-primary-control.key` on
   each host;
3. requires exact regular-file type, owner `connorb`, mode `0600`, size 130,
   and equal SHA-256;
4. revalidates both files immediately before shutdown;
5. passes only the digest identity to the maintenance child, which revalidates
   and never creates, copies, or changes the mode of a key; and
6. attempts cleanup on every pre-mutation failure and during recovery, and
   reports/refuses unless exact absence on both hosts is proven.

L15 exposed the original `0644` mode defect. A subsequent disposable L16
exercise exposed Windows text-mode CRLF expansion to 132 bytes and failed
safely. Binary stdin corrected the transport. Exact final real-host preparation
produced identical regular `connorb:0600:130` files, then proved both paths
absent while production remained live. Fifty focused tests covered
success/consume, mode, owner/type/size/digest, missing/mismatch, freshness
transport failure, changed-file revalidation, failed removal, failed absence
verification, receipt failure, recovery, and secret non-exposure. Independent
adversarial review returned PASS with no P1/P2 finding before production
mutation.

## Frozen authority

The exact execution identities were:

| Item | Exact identity |
|---|---|
| C++ source | `47cd4cb6ecc03b6a24cc0b0083e8d045515003fb` |
| binary pin | `7b443ea37ccdab5e044950e3afb98621ab39870b` |
| controller SHA-256 | `0016daf67b56ce3dce0539c7d870e958f2ff36b807d2a36d2ecba91db5cd9acc` |
| runner SHA-256 | `8be61b70996528a88a61179b264e973a20b504d2ad5e5bbe482ff2fa745c16b3` |
| readiness probe SHA-256 | `f2db27e26567b33a4d4e69c5cb248cf61b63dfa3765aa218d09668225905c980` |
| worker SHA-256 | `07b32f27d17edd34a7a979fefef0bcb09fcb29dfaae8052f16813d1270555d3a` |
| canary SHA-256 | `278f61a406bae87b04e2c435f48a1914989c2e992e626800f201b46044d537ec` |

The exact pinned ROCmFPX artifact remained revision
`dba517197f2854f3d362529e13abddcdcad6c10b`, file
`saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`, size
159,873,097,824 bytes, SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.
It was fully re-hashed before mutation and again into the evidence collection.

The request remained 1,129 prompt tokens, saved boundary 1,128, 128 generated
tokens, context 4096, batch/ubatch 512, Q8_0 K/V, balanced `1,1` layer split,
seed 1234, and temperature zero. Prompt SHA-256 was
`f20c7c7a4137de98b991f1bfe6de27e194a93c7257d9496944e312085923143f`.

## One transition and failure

[MEASURED] Preflight matched nimo-1 coordinator PID 2125672 on port 8081,
HTTP 200, and nimo-2 worker PID 1291141 on port 50052, both with
`NRestarts=0` and the exact standard production commands. The controller
stopped nimo-1 at 05:15:35 PDT, proved it stopped at 05:15:36, and then stopped
nimo-2 at 05:15:37.

The production-transition receipt proves both fresh L16 keys were regular
`connorb` files, mode `0600`, size 130, with equal digest. The disposable
nimo-1 worker started as PID 2144665 on isolated port 50180. The application
gate admitted in one attempt after 2.412 ms and bound:

- RPC protocol `4.0.1` and local-state protocol `1.0`;
- rank/world `1/2`, generation `7`, command mask `31`;
- `CAPS`, `CAPTURE`, `STAGE`, `COMMIT_APPLY`, and `ABORT`;
- request/response limits 1,048,576/256 bytes;
- component/object limits 4,096, 1,073,741,824, and 68,719,476,736 bytes; and
- the expected channel-binding and connection-CAPS digests.

The first capture canary then attempted to allocate
159,231,007,232 bytes on RPC0. Worker evidence immediately beforehand reported
133,143,986,176 bytes total and approximately 131,616,526,336 bytes free. The
request exceeded total capacity by 26,087,021,056 bytes and observed free
capacity by 27,614,480,896 bytes. `cudaMalloc` returned out of memory and model
loading exited before prompt decode or capture.

There is no capture result, suffix, post-run disk counter, `[halofpx-state]`
operation, worker object, cold run, restore, missing/mismatch fallback, or
runtime-off control. No zero-state-transfer or equivalence claim is admitted.

## Recovery, evidence, and cleanup

[MEASURED] Recovery started nimo-2 production worker first at 05:17:07 as PID
1305879 on port 50052, then nimo-1 coordinator at 05:17:08 as PID 2144857 on
port 8081 and waited for HTTP 200. Both `NRestarts` values remain zero. The
coordinator command names the standard
`MiniMax-M2.7-UD-Q6_K_XL-00001-of-00006.gguf` production model.

All L16 units are unloaded/inactive, port 50180 is closed, no canary process or
channel key remains, and the empty state/coordinator roots, exact clones/builds,
bundles, and staging evidence were removed after sealing. The model was not
deleted.

The protected raw archive remains on nimo-2 at
`/var/tmp/halofpx-l16-primary-evidence-20260721-v1.tar.zst`, mode `0600`, size
73,948,424 bytes, SHA-256
`7bc366970354d7e0f16f17d217d9c13aa76365b35c73a1faf0472f99fbd75acb`.
Its zstd integrity test and embedded checksum manifest passed.

Independent final review accepts only this terminal **NOT PROMOTED** closeout.
It does not authorize another attempt, changed placement, tuning, production
cache enablement, eviction, shared/prefix reuse, fault expansion, or a new lane.
