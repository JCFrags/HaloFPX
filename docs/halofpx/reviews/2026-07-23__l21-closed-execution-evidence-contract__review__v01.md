# Independent review: L21 closed execution/evidence contract

Date: 2026-07-23

Verdict: **ACCEPT**

## Scope

The review independently inspected the closed manifest, collector/orchestrator,
focused tests, every preserved archive identity, final v9 evidence, and live
production/cleanup state. It did not modify source or evidence.

## Findings and resolution

The initial review rejected v6 because cleanup command failures could be
ignored, evidence-root probes could silently skip archiving, final archives did
not contain cleanup proof, and controller-local monotonic values were not valid
remote bounds. The re-review rejected v8 because `stop`/`reset-failed` exit code
5 was accepted even for a loaded unit.

The accepted source:

- quotes every remote argv as one `shlex.join` command for fish;
- parses exactly one opaque cursor and rejects missing/malformed/ambiguous
  output;
- captures clocks on the remote host;
- treats every path probe, evidence command, archive/hash/integrity action, and
  cleanup action as mandatory;
- records runtime cleanup inside the archive and verifies evidence-root and
  packaging-intermediate removal;
- accepts nonzero stop/reset only when exact `LoadState=not-found`; and
- has a focused injected `rc=5` plus `LoadState=loaded` rejection test.

## Independent evidence

All 13 focused tests passed. Final archive
`/var/tmp/halofpx-l21-small-evidence-v9.tar.zst` is a 3,753-byte regular file,
mode 0600, SHA-256
`b807e10a24813b9cc5178962aa4dfefbc513737482e86843e67d4b0273ab7f13`.
It contains the status-23 allocation refusal, exact PID/InvocationID and cursor
binding, remote wall/monotonic bounds, disk snapshots, byte-identical
production-before/after records, and a PASS finalization receipt with no
cleanup errors.

The reviewer independently confirmed nimo-1 production PID 2144857,
port 8081, HTTP 200, `NRestarts=0`; nimo-2 production PID 1305879,
port 50052, `NRestarts=0`; disposable units not-found/inactive/PID0; and all
mutable roots, port 50198, archive stage, and cleanup receipt absent. v6 and v8
remain preserved under review-rejected identities.

No material finding remains within the no-production, no-primary,
three-residency-preservation boundary.
