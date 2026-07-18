# L05k bootstrap-authorization consumption transition

Status: accepted offline after independent adversarial review.

L05k freezes the consume-before-create ordering for a future first-store
bootstrap. It adds a target-owned, excluded, memory-only coordinator around an
injected exact-envelope compare-and-advance primitive. No filesystem, durable
registry, anchor creation, bootstrap execution, server path, or node path is
opened by this milestone.

The 1 KiB deterministic-CBOR successor advances an authenticated v1 predecessor
from `H` to `H + 1` and binds the exact predecessor digest plus a complete
receipt: authorization sequence, command identity, token digest, plan
commitment, manifest digest, and proposed anchor digest. A stable, purpose-
separated key-continuity commitment proves that predecessor and successor use
the same registry key tuple and effective secret without reusing their wire-
specific authentication domains. The frozen independent vector is 371 bytes.

Each backend instance owns one nonzero root identity, a bounded terminal-attempt
registry, and sticky quarantine. The wrapper independently recomputes the
operation commitment. Both positive backend outcomes must supply an owned
authenticated current-state witness whose digest and every envelope byte equal
the proposed successor. Definite stale, conflict, replay, and not-applied
outcomes expose no proof; malformed, exceptional, late, or uncertain outcomes
quarantine the root for that backend's lifetime.

Only the coordinator can invoke the backend wrapper and construct the move-only
proof. The proof owns the exact predecessor, successor, proposed anchor, root,
attempt, command, sequence, token, plan, authority, manifest, operation, and
classified-outcome bindings through const accessors. Moving it invalidates the
source. It authorizes no I/O by itself.

Independent review initially found six blockers: unwiped HMAC temporaries on an
authentication-failure path; publicly callable backend execution; positive
outcomes without an exact observed-current witness; an incomplete future-proof
surface; a replay-only race test; and a re-entrant quarantine deadlock. All six
were corrected. The final review returned ACCEPT after focused 5/5 and full
27/27 HaloFPX tests.

The clean Windows CPU/WebUI-off Release control passed 27/27 HaloFPX CTests,
seven focused inherited tests, and 600/600 separate successor, authority/
consumption, and independent-golden processes. Exact hashes and elapsed times
are retained in `evidence/l05k-bootstrap-consumption-repeat-receipt.json`.

Still closed: a real protected registry, restart-surviving quarantine or attempt
history, cross-process CAS, reconciliation, protected key custody, rollback
resistance, conclusive anchor absence, create-if-absent, exactly-once bootstrap,
filesystem durability, cache admission, server integration, persistence, and
node qualification. The v1 authority cannot authorize a different `H + 2`
command from the v2 successor; only exact same-command retry is modeled.
