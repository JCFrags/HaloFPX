# L25 controller transport and evidence reliability result

Date: 2026-07-23

Base: `46461c888b79e5496c4999c38bae749377dc1966`

Outcome: **PASS**

## Bounded SSH authority

[VERIFIED] Every admitted controller and maintenance-child SSH subprocess now
uses one locally enforced bounded transport. Its closed operation classes are
host-key, connect, authentication, command, service mutation, service
readiness, recovery probe, recovery mutation, cleanup, hash, evidence, and
the single long model session. Normal individual probes are bounded at
10–60 seconds, hashes at 120 seconds, and only the admitted model session is
bounded at 1,800 seconds. Higher-level service/model readiness retains its
separate bounded state-machine window.

[VERIFIED] On POSIX the transport creates a new session and terminates the
process group. On Windows it creates a new process group and immediately
assigns it to a 64-bit-safe Job Object with `KILL_ON_JOB_CLOSE`. Timeout sends
`CTRL_BREAK`, waits a fixed two-second grace period, closes the job to kill
the complete tree, and reaps the SSH process. Job-setup failure uses a bounded
exact-PID tree kill and always terminates/reaps the original process; a
tree-kill timeout is caught, retained, and marked
`descendant_cleanup_unproven`, making the result fatal.

Every operation appends a bounded fsynced JSONL record containing host, exact
argv, operation class, wall and monotonic timing, deadline, PID, return code,
bounded stdout/stderr, timeout class, and actual TERM/kill actions. Host-key,
connect, authentication, remote-command, readiness, mutation, and recovery
failures remain distinguishable. Mutating commands are never silently
retried. Recovery commands use the same primitive.

## Capture evidence durability

[VERIFIED] The canary flushes each result line explicitly. The maintenance
child drains the line to a controller-owned file, flushes and fsyncs it, and
only then publishes it under synchronization for acceptance. Before a worker
restart can occur, the child requires a complete newline-delimited capture
record, positive worker bytes/components, nonzero authenticated coordinator
control/local/manifest digests, the exact reference token, and retained
suffix hashes. Partial or unauthenticated output is not a result.

All synchronous child SSH uses the shared bounded transport. The one admitted
long diagnostic session uses the same Job/process-group setup and cleanup and
records typed bounded evidence. The old multi-case rendezvous path is rejected
before spawn under the closed L25 authority.

## Qualification

[MEASURED] Seventy-nine focused tests passed. They include deterministic
never-returning SSH, output-then-hang, TERM-ignoring escalation, real Windows
parent/descendant no-orphan checks, forced Job setup failure with a real
descendant, taskkill timeout, typed evidence, unknown operation refusal,
ordinary and recovery timeout classification, recovery continuation,
transport failure classes, and authenticated flushed capture parsing.

[VERIFIED] Current-source bounded read-only probes reconciled:

- nimo-2 production worker PID 1415055, exact command, listener 50052,
  `NRestarts=0`;
- nimo-1 production coordinator PID 2236922, exact standard UD-Q6
  command/model, listener 8081, HTTP 200, `NRestarts=0`.

Production was never stopped or mutated. No primary artifact was read or
loaded. No disposable inference fixture was necessary; L25 ran no model or
cache lifecycle.

The immutable evidence root is
`C:\Users\britt\Documents\Custom_Inference_Project\sources\halofpx\l25-controller-reliability-20260723`.
Its 11 files total 31,614 bytes and have canonical
relative-path-plus-content SHA-256
`86818ce3a0219894443590ff510800e8066ad5b9928432c58b2d1f6a88958a7d`.
The final focused-test transcript SHA-256 is
`deca73a9d0e9bcc19e76058a1ecca89d8087f04f8ef91d93dcb8d98cd1f9a3cc`;
the final current-source SSH JSONL SHA-256 is
`b81d4da0dd607ea92727f967ab668633c04486f83904b92fbfc50ee36e311831`.

## Boundary

The independent adversarial review passed process ownership, deadline
coverage, mutation ambiguity, recovery liveness, evidence durability, and
cleanup. L25 changes no cache semantics and authorizes no primary retry,
production cache enablement, performance claim, L26, or other continuation.
