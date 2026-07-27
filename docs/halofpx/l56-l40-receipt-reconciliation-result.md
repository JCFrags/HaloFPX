# L56 L40 client receipt-reconciliation discriminator

Status: **PASS (source-backed diagnostic classification)**
Date: 2026-07-27
Base: `51e87b0c011eb3c7dc5b170bd8f64048bccd0853`

## Exact finding

L56 required no additional model runtime. Retained L55 evidence and the exact
current source deterministically identify:

- subreason: `graph_uid_mismatch`;
- expected scheduler graph UID: 26;
- actual RPC split graph UID: 27;
- expected and actual execution sequence: 1;
- RPC backend ordinal: 0;
- local receipt inspection reason: 0 (`OK`).

The scheduler assigns the overall graph a fresh UID before assigning a fresh
UID to each ordered split. Under the frozen RPC0-first placement, the first RPC
split therefore receives the immediately following UID. L55 authenticated that
the worker prepared and executed RPC graph UID 27, sequence 1. L40 then compared
that split receipt UID with the overall scheduler graph UID 26. This comparison
is false even though the executed receipt itself is valid.

This is a diagnosis only. L56 does not change the comparison, receipt
publication, synchronization, consumption, retry, status, or wire behavior.
It does not implement the correction.

## Refusal census

The exact postcompute `l40_graph_result_reconcile` branch has these ordered,
mutually exclusive conditions:

1. scheduler-recorded RPC backend ordinal is outside `backend_ptrs`;
2. local result inspection refuses:
   invalid argument, non-RPC backend, missing RPC context, status not executed,
   zero graph UID, or zero execution sequence;
3. receipt graph UID differs from the prepared overall scheduler graph UID;
4. receipt execution sequence differs from the admitted sequence.

L56 exposes each local inspection condition as a closed enum and maps it to a
closed authenticated subreason. It records bounded expected/actual UIDs,
expected/actual sequences, backend ordinal, and inspection reason.

Malformed receipt framing/size/version/mode/status, attempt/server nonce,
canonical digest, transcript root, and HMAC failures occur earlier in
`hfx_graph_receipt_valid`; they return backend compute failure and cannot reach
this postcompute reconciliation branch. Stale/replay lineage is likewise
handled in the earlier authenticated compute/recompute path. L55 reached the
postcompute reconciliation branch after successful client/backend completion,
so those earlier refusals are excluded for the observed execution.

## Qualification and provenance

The status grammar requires a complete closed L40 subreason tuple, rejects
unknown/incomplete/wrong-branch/tampered records, and retains the existing
HMAC/private-key authority. Feature-off behavior remains inert because the new
inspection is reached only inside the existing explicitly armed composed
authority path.

Focused status, binding, and evidence-ordering tests passed 16/16. Exact ROCm
worker and canary builds completed without a model:

- schema: `halofpx.l56.binary-provenance.v1`;
- source root:
  `6678087c0f10b67f04d10d4d2cd8cbc6a94b033163a0cc26229cd65a00b9dd66`;
- build ID:
  `3f8a7005ad338413eb65c39b7524c5339db8e0279d40e1dfa662c1977ce25cc0`;
- worker SHA-256:
  `5ff5150f51860a031b405b97d4a25ab4506d4309b4f7184f0baeac48bb31cff4`;
- canary SHA-256:
  `a2b287c1d9cbe32ab72e84578ce21aba9fafaa70550d7afeeb6bd261a0922c8c`;
- source archive SHA-256:
  `914f5d4323dbd6c33529f36b0cbb17ae8fd7f2b578f111bb985a9c84f21fd464`.

Raw build evidence is under `docs/halofpx/evidence/l56-raw`; its four-file
canonical tree SHA-256 is
`fac6e14e5684d0dc064c0a4fa608de2544b53716afa3aff111c0051e0fa33383`.

## Safety boundary

No stories runtime, prompt chunk, model load, primary artifact access, cache
operation, or production mutation occurred. Disposable source/build paths were
removed. Production remained nimo-2 worker PID 1535639/50052/NRestarts0 and
nimo-1 coordinator PID 2356329/8081/HTTP200/NRestarts0.

L56 opens no correction or L57.
