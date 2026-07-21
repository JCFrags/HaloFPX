# P08 exact-model critical-path profile review v01

Verdict: **accept for commit after authority disclosure correction**. No P0,
P1, or P2 blocking finding remains, and no rerun is required.

The review independently verified the response token counts, timing arithmetic,
decoded-content hash, 128/129 filtered receive calls, approximately 29--30 ms
median waits, approximately 48--49% decode GPU duty, balanced low-volume
dual-rail traffic, both manifests, both mode-0600 bundle identities, absent
`HALOFPX_` environment gates, and healthy zero-restart rollback. The
serialized-rank conclusion remains correctly labeled `[INFERENCE]`; the traces
support it strongly but do not prove a particular source-level cause.

The profiled P06h binary is source-semantically authoritative for current
runtime behavior, because P06i changed documentation only. It is not
byte-identical source-archive proof: the node `src/models/minimax-m2.cpp` copy
contains 146 CR bytes from CRLF endings. Normalizing to LF produces the exact
committed-file hash with no textual delta. The milestone and receipt now state
this limitation.

Nonblocking improvement: the reusable shell harness has fixed output names and
can overwrite files in a reused evidence root. A later tooling-only hardening
should require a fresh root. That does not invalidate this preserved run, whose
root was project-scoped, verified, and bundled before synthesis.
