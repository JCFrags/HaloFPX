# Independent review: L15 primary-model canary result

Date: 2026-07-21

Reviewer: independent adversarial agent

Verdict: **ACCEPT terminal L15 NOT PROMOTED closeout; do not accept primary-model canary qualification.**

## Review sequence

The first pre-mutation review rejected the runner/controller because recovery
could overlap an orphaned disposable worker and a positive restore could
silently cold-fallback. A second review rejected recovery because a descendant
SSH/canary process could survive on nimo-2. The final committed correction at
`09fe45f82dc91be87142d47a27348788a1ac7c03` made every canary an exact named
transient unit, made controller recovery prove both hosts clean before any
production start, required exact restore/fallback results, and bound journal
evidence to InvocationID/PID. The final pre-mutation verdict was PASS with no
remaining P1/P2 finding; 38 focused tests passed.

## Evidence findings

- Exactly one production transition and one capture-worker start occurred. No
  retry occurred.
- The retained nimo-2 key metadata is mode `0644`, owner `connorb`, size 130.
  The readiness probe rejected this unprotected expected-channel key locally
  before opening the RPC connection. This is not a worker CAPS rejection.
- No readiness JSON, canary result, suffix, object, state operation, accepted
  connection, or canary unit exists. The coordinator and worker roots were
  empty. No zero-transfer or correctness result is admissible.
- Recovery restored nimo-2 worker first as PID 1291141 on port 50052, then
  nimo-1 coordinator as PID 2125672 on port 8081 with HTTP 200. Both exact
  standard production commands are active and both `NRestarts` values remain
  zero.
- All disposable units, processes, ports, roots, keys, clones, and builds were
  removed after sealing. The model was retained.
- The protected evidence archive and its embedded checksum manifest verify.

No P1/P2 finding remains in the terminal closeout. The review does not promote
worker-local primary-model state, performance, equivalence, fallback, or
control-plane transfer claims and does not authorize another attempt or lane.
