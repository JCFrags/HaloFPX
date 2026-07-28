# L66 ADR-0049 foundation result

**Status:** `[MEASURED] NOT PROMOTED`

L66 implemented and exercised the Project Lead's Design-B split between a
strictly non-mutating authenticated preflight and authenticated activation.
During focused qualification, exact request/handler evidence showed the first
preflight refusal was caused by a malformed ephemeral key fixture: 65 bytes
were supplied where the guarded loader requires two 64-hex lines, mode 0600,
matching owner, and a pinned file SHA-256. After correcting only that fixture,
two stale protocol-version checks were corrected mechanically: activation
still expected scheduler-admission major 2 and callers still sent mutable
attempt version 2 after the wire advanced to major 3.

The focused real composed fixture then observed:

`real_composed=1 recompute=1 concurrent=1 exact=1 uids=1/5/2 connection_epochs=9648305431948195/9648305431948195/9648314021650075 allocation_epochs=2/4/2`

This materially validates the non-mutating preflight, logical-versus-physical
census reconciliation, execute/recompute direction, exact output, overlapping
attempts, UID variation, connection epochs, and allocation rollover. It is not
a promotion claim.

Independent adversarial review rejected the candidate because all four L66
retention blockers remained open: independent complete expected binding,
non-tautological finite branch grammar, interprocess durable no-replace
publication, and a complete real-refusal provenance manifest. The candidate
source and candidate-only verifier were therefore removed.

No stories milestone, primary model run, production mutation, cache matrix, or
performance claim occurred. All L66 transient services and paths were removed.
Production was observed unchanged at closeout:

- nimo-1 `minimax-m27-q6-server.service`: active/running,
  `MainPID=2356329`, `NRestarts=0`, port 8081, HTTP 200.
- nimo-2 `minimax-m27-rpc-worker.service`: active/running,
  `MainPID=1535639`, `NRestarts=0`, port 50052.
