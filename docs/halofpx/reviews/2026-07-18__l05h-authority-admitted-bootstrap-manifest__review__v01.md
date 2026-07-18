# L05h authority-admitted bootstrap-manifest review v01

- Date: 2026-07-18
- Scope: exact manifest digest, authority-owned manifest authentication,
  compatibility/replay binding, derived plan inputs, and offline isolation
- Final verdict: **ACCEPT**

## Independent review

The first adversarial pass returned REVISE for one documentation contradiction:
ADR-0011 called a nonzero attempt ID "wrong" even though the deliberately
stateless planner accepts and binds every nonzero 256-bit value. The ADR now
states that only zero is invalid and that nonzero identities are neither
authenticated, historically replay-checked, nor made one-shot.

The clean full matrix later exposed a static integration failure: L04a's
production-use guard did not yet recognize the new authority source as an
approved offline consumer. The allowlist now names only the exact authority
header/source paths. Independent re-review confirmed the authority library is
still `STATIC EXCLUDE_FROM_ALL`, only its explicit test links it, and separate
contracts continue to forbid server, provider, filesystem, and I/O linkage.

The final review found no correctness, authentication, scope, replay,
compatibility, lifetime/concurrency, secret-cleanup, provenance, or
documentation blocker.

## Verification

| Gate | Result |
|---|---|
| Clean Windows CPU/WebUI-off Release build including `llama-server` | Pass |
| Full HaloFPX-labeled CTests | Pass, 18/18 |
| Focused inherited CTests | Pass, 7/7 |
| Manifest-auth process repetitions | Pass, 100/100 |
| Authority process repetitions | Pass, 100/100 |
| Independent adversarial review | ACCEPT after documentation revision and exact allowlist review |

## Promotion boundary

L05h removes raw digest/count authority but remains an offline planner. External
operator authorization, one-shot token wire, protected registry/high-water and
replay storage, conclusive absence, create-if-absent execution, filesystem
durability, object admission, server integration, and nodes remain closed.
