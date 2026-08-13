# ADR-0064: offline closed-world Strix HMM admission snapshot

Status: proposed for offline domain qualification only. Target collection,
sudo/elevation, maintenance admission, issue-#41 closure, target execution, and
performance measurement are not accepted.

Date: 2026-08-13

## Context

ADR-0057 models the complete maintenance transaction but deliberately consumes
fake production, GPU-census, and kernel observations. Its promotion gates call
for fresh live closed-world HMM/KFD/render, systemd, PID, cgroup, listener,
kernel/OOM, and capacity admission. PR #51's adapter evidence also needs one
reusable machine-authority reference instead of inventing its own weaker
fields.

The 2026-08-12 incident is the falsifier. Its retained nimo-2 kernel record
reports 114,041,696 KiB of `gpu_active` HMM at global OOM even though ordinary
memory reporting still appeared usable. The later authority files retain
108,245,408 KiB and 114,067,524 KiB `GPUActive` on the two nodes. They do not
close elevated device ownership, exact per-owner HMM allocation, or clean
cursor-bounded kernel baselines and therefore cannot admit a transition.

## Decision

Add a separate offline-only Python validator with three closed byte contracts:

- `halofpx.strix-hmm-admission-snapshot.v1` for a two-role observation;
- `halofpx.strix-hmm-admission-policy.v1` for trusted identities, owner sets,
  collection requirements, capacity, planned allocation, reserve, and time
  bounds; and
- `halofpx.strix-hmm-admission-result.v1` for a digest-bound, typed
  `ADMIT|REFUSE` decision.

Require explicit `complete`, `unreadable`, or `refused` state per role. A
complete node closes systemd, PID/start identity, executable/argv digests,
cgroup membership, boot-bound monotonic service and capture times, listener
ownership and coordinator health digest, all KFD/render devices and FD owners,
exact per-owner HMM bytes, same-source host aggregate, `GPUActive`, physical
capacity, an enclosing same-boot monotonic kernel interval, nonempty journal
cursor bounds, and OOM/fault/reset counters. The required owner set includes
the protected production identity and its complete device paths. Any missing
or extra JSON field is invalid; any unsafe or uncertain well-formed observation
is a refusal.

Keep capacity policy separate from telemetry. Admission requires checked
unsigned-64 arithmetic, exact physical capacity, aggregate plus planned
increment within admitted HMM capacity, and the required residual reserve.
The planned increment counts only bytes added before a separately verified
release; anticipated service stops or reclaim never create admission headroom.
On each host, the exact sum of all per-owner HMM allocations must equal both
the same-source HMM aggregate and `GPUActive`. If the pinned target kernel
cannot expose an exact per-owner source, v1 refuses. Process RSS, ordinary free
memory, proportional estimates, and an aggregate-only observation cannot
substitute.

Treat HMM accounting semantics as code-reviewed authority, not a self-asserted
string. V1 uses a closed source-profile registry binding each name to one
capture domain, whether it supplies exact per-owner bytes, and whether it is
admissible. `approximate-rss-v1` and all unknown names are invalid. The retained
incident aggregate profile and `retained-incident-evidence` capture kind are
categorically non-admissible even when a document is coherently rewritten.
The frozen v1 registry admits only the synthetic exact profile. Its future
collector placeholder is also non-admissible; a live exact profile requires a
separately reviewed schema revision.

Require a canonical caller-supplied trusted timestamp and record it in the
result. Do not read the control host's wall clock as authority. Validate the
capture interval, policy validity, maximum age, and source kind.

Keep `target_execution_authority=false` and `performance_result=false` literal
in every result. The module imports no SSH, subprocess, socket, sudo, systemd,
KFD, or render collector, defines no target Runner, and can write only a new
local result file. An offline `ADMIT` is a semantic fixture result, not a
maintenance authorization.

Define the ADR-0062 integration seam as an external exact-byte digest. Its
outer receipt may carry `hmm_admission_result_sha256`; its evidence tree must
retain the exact raw result, snapshot, and policy bytes under the frozen root
names `hmm-admission-result.raw.json`, `hmm-admission-snapshot.raw.json`, and
`hmm-admission-policy.raw.json`. Positive validation
must use the bound-result API, which checks both exact input digests and
canonical recomputation before returning overall and per-role `ADMIT`. The
result carries no self-digest. The result-only API is deliberately
non-authorizing and rejects overall or per-role `ADMIT`, because a standalone
envelope can be coherently forged.
The result never replaces signed owner authorization, atomic two-node
nonce consumption, trusted node time, watchdogs, or terminal reconciliation.
The per-role `production_identity_sha256` reuses the exact
`halofpx_strix_maintenance.ProductionIdentity.digest` field and canonical-byte
domain through one shared module. It binds the active pre-maintenance protected
production identity. ADR-0062's later inactive service baseline is a distinct
post-stop observation, while disposable A/B identities remain cycle-local.

## Qualification

The focused tests cover synthetic success; exact input and result field sets;
duplicate keys; non-finite and invalid UTF-8 input; overflow; explicit
unreadable/refused states; capture errors; elevation and closed-world flags;
systemd/PID/start/executable/argv/cgroup/listener drift; missing and foreign
device owners; device-census errors; exact HMM reconciliation and source;
capacity, planned increment, and reserve; every kernel counter; stale, reversed,
and out-of-window time; same-boot service/capture/kernel chronology; every
shared production-identity field, cross-module golden digests, bool-as-integer
confusion; coherently rehashed result laundering, historical promotion, and
approximate-source attacks; new-file output; and the absence of a live
execution surface.

The retained incident test reads the immutable authority logs, performs the
only allowed KiB-to-byte conversion with checked multiplication by 1024, and
proves that aggregate historical measurements plus their known fact gaps
remain `REFUSE`.

## Consequences and remaining gates

The project gains one reusable machine-admission evidence schema and digest
boundary without weakening issue #41. ADR-0057 and PR #51 can consume the same
future result instead of duplicating partial HMM fields.

Still open are the exact pinned-kernel per-owner HMM interface, an authorized
elevated two-node collector with raw evidence custody and race closure,
calibrated capacity/reserve policy, cryptographic owner authorization, atomic
two-node nonce consumption, independent worker-first watchdogs, full PR-#51
evidence-tree validation, and paired terminal reconciliation. This ADR does not
authorize any of them.

Rollback removes the offline module, shared production-identity digest helper,
synthetic examples, focused tests, documentation, CI entry, and this ADR, then
restores the maintenance digest property to its byte-identical inline
calculation. No target, production unit, model, cache, or executable is changed.
