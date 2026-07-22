# ADR-0044: no-production primary-canary execution contract

- Status: rejected at independent review; L20 terminal NOT PROMOTED
- Date: 2026-07-21
- Base: `7cb42be0ba3f45863c418fb9befd5d306f5ce893`
- Scope: default-off harness and disposable small-model proof only

## Residency decision

A model residency is one successful `common_init_from_params()` model load and
the set of contexts created from that model while every selected backend and
its allocations remain alive. Multiple clean or restored contexts may share a
residency. A real RPC worker process restart destroys the worker-owned model
allocations and transport connection, so a post-restart restore cannot honestly
reuse the pre-restart distributed residency.

The candidate minimum complete lifecycle is three material residencies:

1. feature-on worker: uninterrupted capture and a separate clean-cold context;
2. restarted feature-on worker: exact restore plus separate corrupt/missing and
   plan-mismatch cold-fallback contexts; and
3. restarted feature-off worker: one matched clean-cold control.

L20 must prove the worker-restart dependency with the disposable two-host small
model. If a safe runtime mechanism can preserve the distributed residency
across a true worker restart, the count may be reduced only with direct proof.
Six isolated per-mode loads are not admissible.

## Milestone manifest

Every maintenance mutation is authorized by one closed JSON manifest. It binds
the milestone name, maintenance child executable, exact worker and canary host,
port, unit allowlists, protected key paths, and evidence root. The controller
must reject unknown fields, missing fields, duplicate or swapped hosts,
relative or non-private paths, unexpected ports, incomplete unit sets, or a
maintenance argv whose executable differs from the manifest before preparing a
key or stopping production. Cleanup and rollback use only the admitted manifest
and cover every admitted unit, port, process, key, and evidence identity.

## Evidence contract

Before launching the maintenance child, the controller records a monotonic and
wall-clock start marker plus host disk statistics and journal cursors. It then
captures the child PID/exit disposition and collects the exact admitted units'
MainPID and InvocationID-bound journals, allocation/refusal output, and closing
disk statistics in a `finally` path for success, failure, signal, and timeout.
Evidence collection failure is itself a failed maintenance result and cannot
permit promotion. No secret bytes or environment values are recorded.

## Boundary

L20 may use only a disposable small model on isolated ports, roots, units, and
keys while production remains continuously active. It may not read metadata
from or load the primary artifact, provision production keys, stop or restart
production, claim performance, enable cache behavior by default, or open L21.

## Review disposition

The disposable experiment verified that three residencies are sufficient and
necessary for the current honest lifecycle: the worker PID changed between
capture and restore, and the mode-off control required a third feature-off
worker. The candidate controller contract was nevertheless rejected. It did
not exercise an early allocation refusal through the real evidence collector;
its manifest did not own or clean every source, build, state, and evidence
path; post-`systemd-run --collect` InvocationID loss could defeat PID binding;
and evidence collection errors were not uniformly fatal. The candidate source
was removed rather than committed. L20 therefore closes NOT PROMOTED without
weakening any gate.
