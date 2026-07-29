# L101 terminal result

Status: **NOT PROMOTED**. The block-aware KV serialization correction and the
primary cache correctness discriminator passed. Final controller evidence
publication did not.

## Correctness result

- Capture and fresh-residency restore both produced token `21549`, suffix
  ` alpha`, and logits SHA256
  `8564aef91899f6d5cc61ad88a8df4c836600a1006f1bc03b6eb6150e8c27c754`.
- Capture and restore both advanced position `1127 -> 1128` with
  `logits_count=200064`.
- The 60 local and 64 RPC-owned occupied KV payloads cover all 124 tensors and
  total `152180736` bytes. Local represented state is `73636408` bytes,
  including bounded metadata; the worker payload is `78544896` bytes.
- Worker capture, stage, apply, and live recapture each report 64 components,
  `78544896` bytes, aggregate descriptor-content SHA256
  `0386b0393dd101d7b96d4da739ce13e2384d096064b625ac02039c1e50ca9d6f`,
  and no mismatch.
- The exact bounded capture and restore windows contain no `GET_TENSOR` or
  `SET_TENSOR`.
- Five per-attempt response streams and five 4200-byte server terminal
  authorities were authenticated and retained.

## Terminal boundary

After the successful restore result had been emitted and retained, the
controller's call to the remote `halofpx_l48_composed_result.py sign` helper
did not return an accepted 64-character lowercase hexadecimal tag. The
controller therefore stopped with `L48 composed result signing failed`.
No successfully signed terminal composed-result envelope exists.

This is an evidence-publication P2 downstream of the authenticated cache
result. It blocks milestone promotion but does not invalidate the exact
capture/restore equality. No retry was performed.

## Cleanup and production

Disposable units, paths, archives, keys, and staging state were removed.
Production recovered worker-first and coordinator-second:

- coordinator: PID `3027112`, InvocationID
  `e6da1fe637144cb394119959c0e88736`, NRestarts `0`, unique port `8081`,
  HTTP `200`;
- worker: PID `2148915`, InvocationID
  `3480c89086e04d5d80060366c5c7ab7f`, NRestarts `0`, unique port `50052`.

The separately retained `l101-attempt` stopped before production mutation on
the staged-source hash mismatch caused by archive newline conversion. It did
not consume the authorized runtime attempt.
