# L48 — closed runner/controller binding result

Result: **NOT PROMOTED — DISPOSABLE READINESS TRANSPORT-DEADLINE BLOCKER**

Base: `d9aabb66822660b393cc8f14501ea5552471c6d9`

L48 reconstructed the frozen ADR-0048 composition only far enough to build a
closed operational binding. The candidate bound exact feature versions,
source and binary identities, controller and child paths and hashes, protected
key paths, exact child argv, result path/schema, execution structure, and
cleanup ownership. The controller provisioned a fresh identical 130-byte
two-line key to both hosts through bounded stdin, verified regular-file type,
owner `connorb`, mode `0600`, size, and equal SHA-256 without exposing key
bytes, and removed both copies after each exercise.

The runner accepted only the exact key-file option and propagated the
controller-prepared key digest to the endpoints. The candidate result
contract required bounded authenticated records for L40 graph identity, L42
prepared/final scheduler authority, L44 mutable-session census and
SET/SET_HASH receipts, exact prompt chunk order `512,512,104`, replay count,
logits/token authority, phase ordering, and feature-off state. Focused local
tests passed 50 cases, including wrong-mode key and incomplete, duplicate,
reordered, wrong-attempt, and tampered result refusal. Both remote builds
completed. A final independent pre-runtime review returned GO after the
controller itself was added to the exact local path/hash authority.

## Single disposable runtime result

The sole authorized stories15M controller session failed closed before model
qualification. The worker unit started on the isolated endpoint, but the real
HFXCAP2 readiness probe was invoked with an internal 120-second readiness
budget through the runner's generic `command` transport class, whose local
process deadline is 30 seconds. Transport record 24 shows:

- host: `nimo-2`
- operation: `command`
- command: `halofpx_rpc_readiness.py` against `10.44.0.1:50248`
- local deadline: 30 seconds
- duration: 30.010296 seconds
- `timed_out=true`, `term_sent=true`, `kill_sent=true`
- no stdout or stderr

The runner raised a typed transport-timeout failure and no signed composed
result was admitted.
Consequently L48 produced no token, state, scheduler, mutable-session, or
graph-authority correctness evidence. The timeout is an operational
controller/runner binding defect: a readiness operation with a frozen
120-second application budget was placed under an incompatible 30-second
transport class. L48 did not infer authority to change deadlines or repeat
the run.

The controller's finally-protected cleanup reported no failures. Both
disposable units are inactive, port 50248 is closed, all admitted L48
source/build/state/evidence/key paths are absent on both hosts, and no
disposable process remains. Production was continuously unchanged:

- nimo-2 system worker: PID 1535639, exact command, port 50052, NRestarts 0.
- nimo-1 system coordinator: PID 2356329, exact command, port 8081,
  HTTP 200, NRestarts 0.

No primary artifact was accessed and production was not mutated. The rejected
runtime candidate was removed before closeout; only this closeout and immutable
evidence remain. L48 does not make the primary controller preflight-ready.

Raw evidence is retained under `docs/halofpx/evidence/l48-raw/`.
