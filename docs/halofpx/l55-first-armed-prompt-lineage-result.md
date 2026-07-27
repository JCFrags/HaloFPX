# L55 first armed prompt-chunk lineage discriminator

Status: **PASS (diagnostic localization only)**
Date: 2026-07-27
Base: `0578c9ce3e58ef832af734ab4a9c0e0ddae94f26`

## Result

The sole authorized stories15M execution decisively localized the earliest
retained failure boundary. Common warmup completed through the ordinary RPC
graph. The first and only armed 512-token prompt chunk then used execution
sequence 1 and graph UID 27. The worker independently prepared and executed
that graph with canonical digest
`0717a7531b1dfc48d63cce84b1ec0caba7c3b5351278e389297036298b59f0da`.
Immediately afterward, the coordinator returned `llama_decode=-3`.

The authenticated result was:

`phase=capture-chunk|decode_status=-3|authority=version=1|status=failed|branch=l40_graph_result_reconcile|execution_sequence=1|pending=1|ggml_status=-1`

Its HMAC tag was
`9baa968c97e70b5feb21fd7d718010b196c707af3da93db74bb1180b39357488`,
and the protected-key verifier returned the exact canonical record.

The earliest authenticated non-success boundary is therefore coordinator-side
L40 graph-result receipt reconciliation after successful server graph
execution. The evidence does not distinguish absent receipt availability from
a UID or sequence mismatch, and L55 makes no inference beyond that boundary.

No second prompt chunk, capture, restore, replay, logits, token generation,
state object, or primary-model access occurred.

## Source and build authority

The reviewed diagnostic sources embed:

- provenance schema `halofpx.l55.binary-provenance.v1`;
- source root
  `82334ab4f3f5559d8d926a10d343f154afcbe88c7c983a5ffcbf32b382500803`;
- build ID
  `59aebad3ec8d9843af603632ee49386a5f5b07b5b4947357f5b0420009511377`;
- worker binary SHA-256
  `fe7de2d0904ae6e2008f897fa67590776b83b0f7e8399741b6754fa00056bbda`;
- canary binary SHA-256
  `2d55de5fb5c90ad0be860fe6d021a20e7a0cbf284300d22314ec1d19ee81018a`.

Four strictly mechanical compile corrections were retained and independently
reviewed: the standard `<cstring>` include and `std::strcmp` qualification,
the explicit `struct ggml_backend_sched_authority_result` type qualification,
and the width-safe `std::min<size_t>` conversion at the first-chunk boundary.
They do not alter control flow, event grammar, authority semantics, or result
acceptance. The focused binding, evidence-ordering, and status suite passed
14/14.

## Controller, production, and cleanup

The abbreviated L55 child correctly exited 4 and retained its authenticated
diagnostic result. The outer L48 controller subsequently failed closed when it
looked for the intentionally absent full L48 composed-result record. This is
not an L48 controller/composed-result PASS and is not promoted as one; it does
not invalidate the independently authenticated L55 boundary.

Production remained continuously unchanged:

- nimo-2 system worker: PID 1535639, port 50052, `NRestarts=0`;
- nimo-1 system coordinator: PID 2356329, port 8081, HTTP 200,
  `NRestarts=0`.

The production preflight and final snapshots are semantically identical.
Disposable units, ports 50248/50249, keys, source/build roots, evidence
publication root, coordinator root, and rendezvous root were removed and
verified absent.

Raw evidence is under `docs/halofpx/evidence/l55-raw`. Its 28-file canonical
tree hash (relative path, NUL, file SHA-256, NUL) is
`b2075402ba2cbe0eaf28ead3aaa7cd202ef198c22200347ed2dd008b4d7ecac5`.

## Review boundary

Independent review returned **PASS as a bounded diagnostic/localization
milestone**, not a correctness, cache, or controller execution PASS. A future
milestone would need separate authority to discriminate the two remaining L40
receipt-reconciliation cases. L55 authorizes no correction or further run.
