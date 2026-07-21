# Independent review: L16 secure-key primary canary result

Date: 2026-07-21

Reviewer: independent adversarial agent

Verdict: **ACCEPT terminal L16 NOT PROMOTED closeout only.**

## Prerequisite review

Three pre-mutation reviews blocked production until the controller treated key
creation, freshness, lifetime, and cleanup as executable fail-closed authority.
The accepted implementation uses binary stdin with exact `install -m 600`,
requires regular type/owner/mode/size/equal digest, accepts only exact missing
return code 1 during freshness and cleanup proof, revalidates immediately before
shutdown and again in the child, attempts cleanup on every pre-mutation failure,
and reports/refuses unless exact absence is proven. It exposes no key bytes.
Exact removal was proven in the accepted live exercises and final transition.
Fifty focused tests and the exact real-host path passed with production
untouched. Final prerequisite verdict: PASS, no P1/P2 findings.

## Canary evidence findings

- Exactly one production transition, capture-worker start, and capture-canary
  start occurred. No retry occurred.
- Both secure keys passed exact metadata/equality validation and the exact
  HELLO plus HFXCAP2 readiness exchange passed in one attempt.
- The current L16 capture placement requested one 159,231,007,232-byte RPC0
  allocation. The worker reported 133,143,986,176 total and approximately
  131,616,526,336 free immediately beforehand. Allocation and model load failed.
- No prompt decode, capture result, suffix, state operation, object, restore,
  fallback, transfer, timing, I/O, or performance result exists.
- This evidence must not be generalized to aggregate two-host capacity,
  unavoidable model incompatibility, local-state overhead, or an optimal-split
  conclusion.
- Recovery restored nimo-2 worker PID 1305879/port 50052 first, then nimo-1
  coordinator PID 2144857/port 8081/HTTP 200. Both `NRestarts` remain zero and
  the exact standard production commands are active.
- Keys, units, port 50180, processes, empty roots, clones/builds, and staging
  were cleaned after the protected evidence archive and manifest were sealed.

No P1/P2 finding remains in the terminal documentation. The review does not
promote the primary-model cache or authorize another placement experiment,
retry, tuning change, or follow-on lane.
