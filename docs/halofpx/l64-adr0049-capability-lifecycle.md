# L64 ADR-0049 capability/lifecycle closeout

Status: `[MEASURED] NOT PROMOTED`.

L64 attempted the full ADR-0049 capability/lifecycle foundation without loading
stories15M, reading the primary artifact, or changing production. A disposable
two-endpoint no-model candidate reached exact output across two execution
sequences and an independent endpoint, but independent adversarial source review
rejected the candidate before acceptance. All candidate source was removed.

The review found these material blockers:

1. Expanded L44 wire structures retained protocol version 1.0 and lacked explicit
   mutual capability negotiation, so old/new peers could disagree under the same
   version and feature-off wire compatibility was not proven.
2. The prepared L42 admission used a consuming claim and L44 checked only
   structural fields/nonzero tag; it did not independently authenticate the
   scheduler-owned value or fully bind its lifetime into server receipts.
3. Several real client and server refusal paths emitted no exact attempt-scoped
   reason, including server-side begin rejection and decode/receipt failures.
4. Terminal cardinality was derived from the number of records already emitted,
   rather than a closed path grammar, so missing events were not detectable.
5. Abort/disarm and server invalidation could discard the recorder after an
   ignored publication failure.
6. The concurrency test did not overlap attempts, and concurrent recorders could
   append multi-write records to a shared sink without a common publication lock.
7. Required transport failure injection, stale/cross-epoch refusal, authenticated
   stream verification, and L61 harvesting coverage were incomplete.

The successful disposable output is retained only as rejected-candidate evidence:

`real_composed=1 recompute=1 concurrent=1 exact=1 uids=1/3/5
connection_epochs=13/13/12 allocation_epochs=2/2/2`

`feature_off_inert=1`

It is not an accepted ADR-0049 foundation and does not make the primary path
preflight-ready.

Cleanup reconciled both disposable units to `LoadState=not-found`,
`ActiveState=inactive`, `MainPID=0`.

Production was checked read-only after cleanup:

- nimo-2 `minimax-m27-rpc-worker.service`: loaded, active/running,
  MainPID 1535639, NRestarts 0, listener 50052.
- nimo-1 `minimax-m27-q6-server.service`: loaded, active/running,
  MainPID 2356329, NRestarts 0, listener 8081, HTTP 200.

No L65 is opened.

