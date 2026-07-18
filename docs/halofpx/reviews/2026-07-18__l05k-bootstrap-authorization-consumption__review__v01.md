# L05k bootstrap-authorization consumption review v01

- Date: 2026-07-18
- Scope: successor wire, key continuity, operation identity, backend fencing,
  ambiguity classification, proof ownership, concurrency, default-off isolation,
  evidence, and non-capability claims
- Final verdict: **ACCEPT**

## Independent adversarial review

The first review returned REVISE with six promotion blockers. The successor
verifier failed to wipe a computed HMAC after authentication failure. Backend
`execute` was public, permitting an arbitrary holder to quarantine a root. The
positive backend result was a bare enum without an exact observed-current
successor witness. The proof omitted fields required to recompute the operation.
The race test duplicated one attempt rather than racing distinct commands. The
backend also held its mutex while a virtual callback could re-enter the locked
quarantine accessor.

The final implementation wipes derived authentication temporaries on every
failure path; makes execution private and coordinator-only; requires an owned,
authenticated, exact-byte/digest successor witness for both positive outcomes;
exposes every recomputation input through a const, move-only proof; races fresh
attempts for distinct commands and qualifies exact same-command retry; and uses
an atomic quarantine observation safe during the serialized callback. A missing
or mismatched positive witness is malformed and sticky-quarantines the root.
Final independent re-review returned ACCEPT.

## Verification

| Gate | Result |
|---|---|
| Clean Windows CPU/WebUI-off Release build including `llama-server` | Pass |
| Full HaloFPX-labeled CTests | Pass, 27/27 |
| Focused inherited CTests | Pass, 7/7 |
| Successor process repetitions | Pass, 200/200 |
| Authority/consumption process repetitions | Pass, 200/200 |
| Independent golden-vector repetitions | Pass, 200/200 |
| Independent adversarial review | ACCEPT after six corrections |

The independent vector fixes a 371-byte successor envelope, authentication tag
`1f3f79689d0b2dd37988aac8c92bf90be6689b0079bd5fc4a7066973fdebd202`,
envelope digest
`fd1a86a341a6b90639e7fa31730d1c77fcbc898ce11b527d241b9d55325069e2`,
and cross-version continuity commitment
`3390a19271852403712dac091321cebfac9abb640422d81ae4136cc7a7cc7386`.
Exact file and executable hashes are retained in the repeat receipt.

## Promotion boundary

L05k qualifies only an excluded, synchronous, memory-only transition contract.
It does not prove real durability, protected origin, restart recovery,
cross-process coordination, rollback resistance, reconciliation, protected
anchor absence, create-if-absent, bootstrap execution, cache persistence,
server/provider behavior, or node behavior. Quarantine and terminal-attempt
history deliberately do not survive construction of a new backend instance.
