# ADR-0045: closed disposable execution and evidence contract

- Status: accepted by L21 qualification
- Date: 2026-07-22
- Base: `e2edc4b3277f5385118e759ed9f89c1ea0a7445a`
- Scope: no-production contract correction; small disposable model only

## Decision

The accepted lifecycle remains exactly three model residencies as recorded by
ADR-0044. L21 does not repeat or alter it.

One closed manifest is the sole disposable authority. Its schema has no
extension fields and binds exact hosts, ports, units, executable paths and
hashes, child argv, input fixtures, protected keys, source/build/state/evidence
roots, retained archive, and cleanup targets. Values must be absolute,
private, unique, host-bound, milestone-prefixed, and members of the frozen L21
allowlist. Validation completes before any unit, file, or process mutation.

Every transient unit starts without `--collect`. Immediately after start, the
collector obtains a nonzero `ExecMainPID`, a 32-hex `InvocationID`, and the
unit's monotonic start timestamp. Only then may the controller wait for exit.
The journal is selected by exact unit and InvocationID, bounded below by the
pre-start journal cursor and wall/monotonic timestamps, and reconciled to the
captured PID. A unit cannot be reset or collected until its identity, exit
status, journal, allocation/refusal lines, and closing disk statistics are
durably written.

Every evidence, archive, hash, permission, and cleanup command is mandatory.
Failure makes the result non-promotable but enters the same best-effort cleanup
finally path. Cleanup covers all manifest units, ports, keys, source/build/state
roots, temporary evidence roots, and child processes. Only the manifest-bound
mode-0600 hashed archive may remain.

Fresh production-before and production-after snapshots bind exact host, unit,
command, PID, listener, start timestamp, restart counter, model role, and HTTP
health. Any difference fails L21 even though production mutation is prohibited.

## Boundary

L21 exercises only one real early allocation refusal plus focused synthetic
abnormal-exit, timeout, evidence-command-failure, and cleanup-failure cases. It
does not access the primary artifact, provision production keys, stop or
restart production, repeat the three-residency proof, claim performance, enable
cache behavior, or open L22.

## Qualification

[VERIFIED] Thirteen focused tests cover closed-manifest refusal, fish-safe remote
argv, exact/ambiguous cursor handling, PID/InvocationID mismatch, abnormal exit,
timeout, evidence/path-probe failure, archive-failure retention, and cleanup
failure.

[MEASURED] A real isolated nimo-1 RPC worker refused one 133,143,990,272-byte
request against reported total 133,143,986,176 bytes. The passing mode-0600
archive is `/var/tmp/halofpx-l21-small-evidence-v9.tar.zst`, SHA-256
`b807e10a24813b9cc5178962aa4dfefbc513737482e86843e67d4b0273ab7f13`.
It contains PID/InvocationID/cursor-bound journals, remote-host clock bounds,
disk evidence, production equality, runtime cleanup, and finalization records.

[MEASURED] Production closed unchanged at nimo-1 PID 2144857/8081/HTTP 200 and
nimo-2 PID 1305879/50052, both `NRestarts=0`. All disposable units, port 50198,
source/build/state/key/evidence paths, and packaging intermediates were removed.
