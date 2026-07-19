# ADR-0024 sticky-quarantine publication adversarial review v01

- Date: 2026-07-18
- Scope: portable fake-only quarantine diagnosis, event-ID authority,
  operation-6 admission, operations 70-76, and restart closure
- Final verdict: **ACCEPT**

## Review history

The initial 69-to-75 proposal was rejected. Independent review identified four
material gaps: operation 5 retained no exact reason/attribution plan; an
untrusted marker could not authorize a signed root-bound quarantine; operation
69 lacked private event-ID authority; and the cross-directory rename omitted
source `staging/` synchronization.

Two further rounds rejected contradictory optional-field shapes, an
unreachable reason-5 mapping, prose-only commitments, inconsistent restart
freshness, and an open-ended namespace projection. ADR-0024 was amended after
each round and implementation remained closed.

## Accepted contract

The accepted decision now requires:

- publication only from an independently authenticated compatible root;
- a closed reason precedence and exact U0/UH/P/S attribution matrix;
- nonpublication for marker/key/scope and residual invariant failures;
- an event-free deterministic-CBOR diagnosis commitment;
- an Ops-only private move-only event witness with an exact 32-byte fake ID;
- authenticated QUARANTINE encoding and self-verification after operation 69;
- a second deterministic-CBOR action commitment binding the diagnosis, event,
  root/path scope, every optional field, full content digest, and length;
- exact state, logical-budget, and reserve revalidation at operation 6;
- no injectable gap between successful operation 6 and operation 70;
- atomic no-replace publication through operations 70-76, including both
  destination-root and source-staging directory synchronization; and
- exactly four restart namespace outcomes before full synchronization:
  neither, staging only, final only, or both.

No positive quarantine disposition exists. Existing quarantine/staging and
unpublishable sticky state short-circuit without event acquisition. Every
ordinary action result remains `quarantined_or_unavailable`; process death has
no ordinary result. Retained final or staging presence blocks all future
mutation.

## Independent algebra check

The eight new operation IDs admit:

| operation | admitted products |
|---:|---:|
| 69 event witness | 12 |
| 70 staging create | 17 |
| 71 exact bounded write | 25 |
| 72 authenticated readback | 9 |
| 73 file synchronization | 25 |
| 74 no-replace rename | 17 |
| 75 root-directory synchronization | 25 |
| 76 staging-directory synchronization | 25 |
| **new total** | **155** |

Across eight IDs, the full code/effect/completion space is
`8 * 5 * 3 * 8 = 960`, so 805 products are forbidden. Adding the accepted
342-product L05q algebra gives exactly 497 cumulative admitted products.

## Final findings

The final review confirmed the commitment chain is acyclic and independently
implementable:

1. diagnosis commitment without an event;
2. Ops-only witness binding invocation and diagnosis while supplying the event;
3. authenticated record encoding producing the full-envelope content digest;
4. action commitment binding diagnosis, event, optional fields, digest and
   length; and
5. operation 6 consuming the witness and independently checking exact state.

The fake ID is collision-free within the shared process-global modeled-restore
issuance domain. Real cross-incarnation uniqueness is correctly unclaimed and
future Linux remains gated on a separately qualified OS CSPRNG and collision
policy.

No blocker remains for implementation inside the existing fake-only,
`STATIC EXCLUDE_FROM_ALL` target. The review does not authorize normal CAS,
Linux credentials or syscalls, filesystem/process-crash evidence, persistent
writes, a durability mode, M63-01 closure, provider/server/cache linkage,
restore, inference, or performance claims.
