# L05g offline bootstrap-authority planner

Status: accepted offline after independent adversarial review.

L05g freezes only the first-store administrative planning boundary from
ADR-0010. A noncopyable memory authority owns fixed scope and separate
bootstrap-admin and anchor-authentication key purposes, copies caller material,
and wipes the owned secret bytes at destruction. It exposes no raw key view.

The planner can produce an opaque `authorized_unexecuted` plan for a nonzero
attempt, `1..128` objects, and one selected manifest digest. It synthesizes the
exact authenticated generation-one anchor with a null predecessor and binds
the plan to the immutable authority snapshot. Failed/default results expose no
plan, and plans retain no key material.

This milestone intentionally has no execution primitive. It does not prove
operator identity, token issuance, durable registry/high-water protection,
historical attempt replay prevention, conclusive anchor absence, create-if-
absent behavior, asynchronous reconciliation, filesystem durability, or an
authenticated manifest-admission path. No backend, server, provider, cache-hit,
persistent-write, or node path is opened.

The clean Windows CPU/WebUI-off Release control passed all 18 HaloFPX CTests
and seven focused inherited regressions. The exact anchor and authority test
binaries each passed 100 independent processes. The first independent review
found two incomplete key-derived temporary cleanup exits in the reused anchor
encoder; those exits and the static cleanup-coverage contract were corrected,
and re-review returned ACCEPT. Exact executable hashes and counts are retained
in `evidence/l05g-bootstrap-authority-repeat-receipt.json`.
