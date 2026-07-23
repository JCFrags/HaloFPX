# L32 coordinator live-state completeness diagnosis

Date: 2026-07-23

Base: `60f4272c4a9f0ecb9e365e0c32e697513668d043`

Outcome: **PASS — AUTHENTICATED LIVE-RECAPTURE INSTRUMENTATION; PRIMARY DISCRIMINATOR REQUIRED**

## Scope and finding

[VERIFIED] L32 did not access the primary artifact or mutate production. It
audited the real sequence-state capture/apply paths and added bounded,
default-off diagnostics at the actual post-apply/pre-generation boundary.

[VERIFIED] L31's equal coordinator receipts established input-blob equality,
not equality of the restored live context. L32 now allocates a fresh diagnostic
sequence storage after successful apply, recaptures through the real
`llama_context` memory interface, and independently captures the worker's live
components under a separate diagnostic generation/checkpoint identity. The
original restore object is not reused as recapture evidence.

The authenticated 504-byte coordinator live receipt distinguishes:

1. original capture;
2. authenticated restore input;
3. live post-apply recapture;
4. the immediately adjacent pre-generation boundary.

It binds the token boundary, control/local sizes, worker count/bytes, control,
local, and component-manifest digests for all phases, the recapture object
digest, and an HMAC. Only hashes and bounded metadata are emitted. No
state-changing operation occurs between live recapture and the pre-generation
marker.

[VERIFIED] The worker analyzer now authenticates and compares capture, stage,
apply, and the independent second capture as `recapture`. It reconstructs
component leaves and Merkle roots and rejects missing, duplicate, malformed,
overlapping, content-changing, or unauthenticated records. Coordinator receipt
size, authority, HMAC, and all control/local/manifest phase equalities are
independently verified.

## Source audit

[VERIFIED] The sequence control stream delegates to the active memory
implementation. Transformer KV state includes stream and cell counts,
positions, sequence ownership, extended positions where applicable, value
layout, layer count, types, row geometry, and tensor ranges. Recurrent and
hybrid memories have separate metadata and tensor-state paths. Public extended
get/set boundaries synchronize the context.

L32 does not claim every architecture-specific runtime field is serialized.
State intentionally absent from the memory interfaces remains a primary-model
semantic uncertainty. Changed physical range topology is retained as an
observation, not classified as a defect.

## Focused qualification

[MEASURED] The accepted 19,077,344-byte stories15M fixture used two honest
model residencies, F16 K/V, flash attention off, a 1,129-token prompt, 1,128
saved-token boundary, and maximum prompt chunk 512. This tuple qualifies
lifecycle and state diagnostics only; it is not representative of primary
Q8_0/flash-attention performance.

[MEASURED] Original capture, restore input, live post-apply recapture, and
pre-generation coordinator state were exact:

- control: 13,704 bytes,
  `74994f73a5b5e972d9a57cf897a8869871fe83844b35909d51f57ae07de196e4`;
- local: 2,603,624 bytes,
  `dd773b55782dff7623889a9df752e389b5bee09f76d9b423a716db8aecb3b9ee`;
- component manifest:
  `317230332505f5039715f6ed2219eecab6ffd78d5fc0332030c0f6e997fd30d1`;
- token boundary: 1,128;
- live-recapture receipt HMAC:
  `96342aba14a2364ac7f0838de8099a4936f7c7178620cd782825ca7275fb3dde`.

[MEASURED] Worker capture, stage, apply, and recapture each contain 1,156
components and 5,197,824 bytes. All content aggregates equal
`ed62ec5b04cc53ce870ddd6df1d8eefc10a0e4f44e2a699deb879ebf4462fdbc`.
The analyzer reports no identity/content mismatch. Capture, stage, and
recapture Merkle root is
`b61a834dcb4859ba48196616614bd92b80bed44b33b58518b62b9c2ccabd02e0`;
apply has the expected live-layout root
`d88a74bbc23b8a9d067a44d35215b43602f7a1d0ac0465e91dde61282f0af7b0`.
The range-layout difference is not treated as a defect.

[MEASURED] Capture and restored token are both 4245. Token and decoded hashes
are exactly
`105dcfac89ff7ef4efb5b9253b18b4609ac2a44933a031a2e73020d41d6f6188`
and `6393507a33556ff939295d9a31b1e21b53c391f7c64eefa95226110c39dc56df`.
State windows contain zero legacy `GET_TENSOR`/`SET_TENSOR` operations.

[VERIFIED] Focused synthetic tests cover coordinator control/local/manifest
phase changes, wrong phase authority, truncation, authentication corruption,
worker recapture omission/duplication, and first component divergence. The
focused Python suite passes 96/96. The exact C++ canary and RPC worker compiled
on ROCm-enabled hosts.

The first qualification attempt lacked the coordinator diagnostic environment;
the second exposed an obsolete three-phase agreement parser. Both failed
closed and are retained. The accepted third attempt exercised the final path.
Its analyzer identity was
`aa8eeaba48d80ae09b6c667c2c3cebc5c37ca16e4be43d9645ce6c3b21560dfb`.
Closeout then added an explicit phase-order refusal, independently covered by a
focused test; the final analyzer identity is
`eeb7645ee1d7fe396bf528efbb892d6fdfefd76dc027de7e5b268baa7eb159d0`.

## Production, cleanup, and evidence

[VERIFIED] Production stayed continuously active. Nimo-2's exact system worker
remains PID 1468887 / port 50052 / `NRestarts=0`. Nimo-1's exact system
coordinator remains PID 2304428 / port 8081 / HTTP 200 / `NRestarts=0`.

[VERIFIED] All L32 disposable units, port 50232, keys, state/evidence roots,
source/build roots, remote archives, and the local staging archive are absent.

Raw evidence:
`C:\Users\britt\Documents\Custom_Inference_Project\sources\halofpx\l32-live-recapture-20260723`.
It contains 65 files / 9,387,356 bytes with canonical
relative-path-plus-NUL-plus-content SHA-256
`829c72814a801c2324205fcfdd21f57dadaa310b189d01ef2fea0d7068992839`.

## Boundary and smallest remaining discriminator

No source-backed coordinator/control defect was proven on the accepted
disposable fixture, so L32 makes no semantic correction. The smallest remaining
primary discriminator is one separately authorized two-residency, one-token
restore with this exact live-recapture diagnostic enabled. Acceptance would
require authenticated equality of coordinator control/local/manifest across
all four phases, worker capture/stage/apply/recapture content equality, exact
token boundary, and then exact first-token equality. A mismatch at live
recapture would localize the first unequal live boundary; complete live
equality plus token mismatch would narrow, but not prove, primary-specific
omitted semantics.

L32 does not authorize that experiment, primary access, production mutation,
cache promotion, tuning, or L33.
