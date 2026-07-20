# L09 independent milestone review v01

Date: 2026-07-20

Disposition: **ACCEPT**. No P1 or P2 blocker remains.

## Scope reviewed

The review covered the L09 implementation and evidence against ADR-0036 and
the canonical Wiki requirements for correctness, freshness, clarity,
provenance, security, rollback, performance isolation, and reusable
improvements. It inspected the dual-lock authority, layout and byte accounting,
quota and reserve checks, private scope, server failure disclosure, redacted
observation, default-off behavior, and the no-eviction boundary.

## Finding and correction

The initial review found one P2 issue: some failures after durable pending-record
publication could close writes without fully reconciling publication state.
The implementation introduced one total fail-closed path for every
post-pending exit, including root drift, materialization, anchor, terminal,
cleanup, and exception paths. Publication uncertainty now reconciles through
the same path. A focused `after_pending_storage_failure` regression preserves
that behavior. The earlier Btrfs portability defect in directory link-count
validation also remains correctly repaired without relaxing the type, owner,
mode, device, mount, or `openat2` containment checks.

## Qualification reviewed

- selected focused and inherited controls: 12/12 passed;
- process canary: miss, durable write, restart, authenticated exact hit;
- reserve canary: coarse storage rejection, write closure, no publication
  mutation;
- gated server SHA-256:
  `7ef354e70ff8a69bf33b9eb4d57c6ef4bc17e06720f99647e074624f8d043851`;
- focused test SHA-256:
  `3294c711bdad148bc0bb1626b62f2e05792087c3ec6f2be0c23b04b27e6964dc`;
- exact component tuple SHA-256:
  `cb0fde361fd2b902cdd9bac8892788313cb556024363039b5b6016fb724f4877`;
- sanitized evidence manifest SHA-256:
  `1458ba47400a12fe35062253917d40ab4e69ad7c6c5fe04be9a3849f8223a052`;
- compressed evidence bundle SHA-256:
  `c65d2bbbf2420a702484d16eb54eaea20fbcee981b2edfdee3fa0e70fa2da3ed`.

The known-good nimo-1 server and nimo-2 RPC worker remained active and enabled.
All five immutable reference repositories remained clean at their locked
commit and tree.

## Boundary

L09 remains a Linux-only, compile-gated, runtime-opt-in, private,
single-generation canary. It does not admit production persistence, shared
scope, online deletion, eviction, generation advancement, or an administrator
HTTP route. No donor implementation, GPL llama-ai code, CachyLLama transplant,
new dependency, WebUI, remote, release, model mutation, or deployed-service
change entered the milestone.

No additional test expansion is warranted for this milestone. The deferred
fault, filesystem, concurrency, retention, distributed-recovery, and soak
matrices should be reopened only for a concrete risk hypothesis or the later
product-admission boundary.
