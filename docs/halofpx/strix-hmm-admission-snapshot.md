# Offline Strix HMM admission snapshot

Status: **offline contract only**. This document, its parser, its examples, and
its tests do not authorize or perform target access. Issue
[#41](https://github.com/JCFrags/HaloFPX/issues/41) remains the production gate.

The executable closed-world validator is
[`scripts/halofpx_strix_hmm_admission.py`](../../scripts/halofpx_strix_hmm_admission.py).
It consumes already-collected JSON bytes and a caller-supplied trusted UTC
timestamp. It imports no SSH, subprocess, socket, sudo, systemd, KFD, or render
collector and has no target `Runner`.

## Why this is separate

The 2026-08-12 incident proved that ordinary memory availability and process
RSS did not describe the machine's real allocation risk. At the first nimo-2
global OOM, the retained journal reported `gpu_active:114041696kB`; later
post-recovery records reported `GPUActive: 108245408 kB` on nimo-1 and
`GPUActive: 114067524 kB` on nimo-2. The validator converts those historical
Linux `kB` values to bytes only with checked multiplication by 1024.

Those records do **not** contain a complete elevated device-FD census, an exact
per-owner HMM accounting source, or two clean cursor-bounded kernel baselines.
The `retained-incident-evidence` capture kind and its aggregate-only accounting
profile are categorically non-admissible even if every mutable JSON field is
made internally consistent. They therefore cannot be transformed into a v1
`ADMIT`. Tests retain the exact aggregate measurements while classifying the
missing authority as `REFUSE`.
The incident manifest remains exact SHA-256
`331634016681b57183aedbea3550f95d86486ce21d1baf8e7e3e3e5c6f35d815`.

## Three versioned byte contracts

All three formats are UTF-8 JSON objects with duplicate keys, non-finite
numbers, extra fields, omitted fields, wrong types, and values outside unsigned
64-bit range rejected. The result's `snapshot_sha256` and `policy_sha256`, and
ADR-0062's outer `hmm_admission_result_sha256`, hash exact raw file bytes. They
never hash reformatted JSON.

Embedded structured digests use explicit canonical domains. A
`node_snapshot_sha256` hashes the parsed node object with UTF-8, sorted keys,
compact separators, no ASCII escaping, and no trailing newline. Device-owner
identity digests use that same grammar over exactly host, PID/start ticks,
unit/cgroup, executable/argv digests, and device paths. The protected production
identity deliberately uses the already-established maintenance grammar described
below, including its trailing newline. These domains are not interchangeable.

### Snapshot v1

Schema: `halofpx.strix-hmm-admission-snapshot.v1`.

The top-level field set is exactly:

```text
schema, issue, capture, roles, errors
```

`issue` is 41 and `roles` is exactly `coordinator` and `worker`, fixed to
`nimo-1` and `nimo-2`. `capture` closes one start/completion interval, its
source kind, elevation state, closed-world state, and collection errors. Each
role has one explicit collection state:

- `complete`: the full observation is present and the node error list is empty;
- `unreadable`: the observation is null and one or more typed errors explain
  why it could not be read; or
- `refused`: the observation is null and one or more typed errors explain the
  refusal.

A complete observation closes all of the following in one node record:

- systemd unit, `ActiveState`, `SubState`, `InvocationID`, `MainPID`, restart
  count, monotonic service-start timestamp, and control group;
- the node boot ID plus monotonic start/completion timestamps for capture;
- PID start ticks, executable and argv SHA-256, process cgroup, and the exact
  sorted cgroup membership;
- exact TCP listener port and sorted owner PIDs, plus the coordinator health-body
  SHA-256 (the worker health digest is null);
- the complete sorted `/dev/kfd` plus render-node set;
- every device-FD owner, bound to PID/start ticks, unit/cgroup,
  executable/argv digests, owned device paths, and exact `hmm_allocated_bytes`;
- a named exact HMM accounting source, its host aggregate, `GPUActive`,
  `GPUReclaim`, and physical capacity, all in bytes; and
- the same boot ID, a monotonic interval enclosing protected service start and
  the complete capture interval, start/end journal cursors, and counts for global
  OOM, OOM kill, amdgpu fault, KFD fault, and GPU reset, plus collection errors.

The snapshot reports observations only. It does not choose safe capacity,
reserve, planned allocation, expected service identity, or allowed owners.

### Policy v1

Schema: `halofpx.strix-hmm-admission-policy.v1`.

The top-level field set is exactly:

```text
schema, issue, validity, capture_requirements, roles, errors
```

The policy supplies the facts that telemetry cannot self-authorize:

- validity window and maximum snapshot age;
- mandatory elevated, closed-world, zero-kernel-event collection;
- exact source kind;
- the expected boot ID for each role;
- exact protected production identity for each role;
- exact KFD/render node inventory;
- allowed and required device-owner identity digests;
- the exact per-owner HMM accounting source name; and
- physical capacity, separately admitted HMM capacity, planned incremental
  allocation, and required post-plan reserve, all in bytes.

`planned_increment_bytes` means bytes that may be added before any separately
verified release. V1 never credits an expected service stop, reclaim, or future
cleanup. For the ADR-0062 stop-then-start transaction, this pre-mutation result
can set the increment to zero to qualify the current protected baseline; the
later stopped-service record and cycle-local GPU censuses must independently
admit the disposable allocation. An active-baseline result cannot pre-authorize
that later allocation by assuming protected HMM will disappear.

The example's capacities and identities are synthetic fixtures. They are not
target measurements or approved operating thresholds.

The required owner set must include the protected production identity bound to
the complete policy device set. Device-owner identities canonically cover host,
PID/start ticks, unit/cgroup, executable/argv digests, and owned device paths;
they deliberately exclude the changing HMM byte count.

The protected `production_identity_sha256` has one shared digest domain with
`halofpx_strix_maintenance.ProductionIdentity.digest`. Its exact fields are:

```text
role, host, unit, pid, invocation_id, nrestarts, process_start_ticks,
start_monotonic_us, executable_sha256, argv_sha256, control_group,
listener_port, listener_pid, health_sha256
```

[`scripts/halofpx_strix_production_identity.py`](../../scripts/halofpx_strix_production_identity.py)
owns that field set and the existing maintenance canonical-byte grammar. The
HMM validator and maintenance authorization both call its
`production_identity_digest` function. This makes each HMM role digest directly
comparable to `Authorization.production[role].digest`; it is not a digest of a
similar but narrower HMM-only identity.

### Result v1

Schema: `halofpx.strix-hmm-admission-result.v1`.

The top-level field set is exactly:

```text
schema, issue, snapshot_sha256, policy_sha256, trusted_now_utc, roles,
decision, reason_codes, target_execution_authority, performance_result
```

Each exact role record contains:

```text
role, host, node_snapshot_sha256, production_identity_sha256,
classification, hmm_headroom_bytes, reason_codes
```

`classification` and the overall `decision` use only `ADMIT` or `REFUSE`.
Reason codes are a closed, sorted, duplicate-free enum enforced by the module.
For a role to classify `ADMIT`, its node and production-identity digests and
nonnegative residual headroom must be present and its reason list must be
empty. Overall `ADMIT` requires both roles to classify `ADMIT` and every reason
list to be empty.

This result describes the earlier, pre-maintenance instant with protected
production active. ADR-0062 binds these exact result bytes by digest, then
separately validates its later post-stop/pre-adapter service baseline. The two
records are different time boundaries; disposable A/B identities do not enter
`production_identity_sha256`.

The result always has `target_execution_authority=false` and
`performance_result=false`. It carries no self-digest. A downstream evidence
tree, including ADR-0062's PR-#51 validator, must retain the exact snapshot,
policy, and result bytes as root `hmm-admission-snapshot.raw.json`,
`hmm-admission-policy.raw.json`, and `hmm-admission-result.raw.json`. Its outer
receipt may bind the result as
`hmm_admission_result_sha256`, because successful bound validation first checks
the result's exact snapshot and policy digests and then canonically recomputes
all fields. Positive consumers must call
`validate_bound_admission_result_bytes(result, snapshot, policy)`. It returns
the parsed canonical result or raises exported `AdmissionError`.

`validate_admission_result_bytes(content)` is deliberately non-authorizing: it
may validate a wholly negative envelope, but rejects overall or per-role
`ADMIT`. Result bytes alone can be coherently forged, so the API never
reclassifies them or establishes a positive decision. Consumers must still
obtain owner authorization, window/nonce consumption, watchdog, and terminal
reconciliation independently.

## Fail-closed admission rules

`ADMIT` requires all of these conditions at once:

1. The supplied trusted time and capture interval are canonical whole-second
   UTC, inside the half-open policy interval `[not_before_utc, expires_utc)`,
   ordered, and fresh. There is no system-clock fallback.
2. Capture is elevated, closed-world, complete, and from the policy-named
   admissible source. Retained incident evidence is always non-admissible.
3. Both service/process/cgroup/listener records exactly match policy.
4. The device census exactly matches policy, every owner is allowed, and every
   required owner is present.
5. On each host, the checked sum of every owner's exact
   `hmm_allocated_bytes` equals both the same-source HMM aggregate and
   `gpu_active_bytes` exactly. The accounting source must appear in the closed,
   code-reviewed v1 registry, match the capture domain, promise exact per-owner
   bytes, and be marked admissible. An arbitrary matching string such as
   `approximate-rss-v1` is structurally invalid. If the pinned kernel cannot
   expose exact per-owner HMM accounting, v1 must remain `REFUSE`; RSS or
   proportional attribution may not substitute.
6. Aggregate plus planned increment fits the admitted HMM capacity and leaves
   at least the policy reserve. Physical capacity also matches policy.
7. The capture clock, kernel window, and policy identify the same boot. The
   kernel interval starts no later than protected service start and ends no
   earlier than capture completion; the service start precedes capture start.
   Every cursor-bounded OOM, kill, amdgpu/KFD fault, and reset count is zero and
   every collection error list is empty.

The validator never converts a malformed JSON document into a signed-looking
refusal result. Structural errors exit separately. Well-formed but unsafe or
incomplete snapshots produce typed `REFUSE` results.

The frozen v1 registry admits only `synthetic-exact-hmm-accounting-v1`, and
only in the synthetic capture domain. The retained incident aggregate profile
and `future-elevated-collector-unqualified-v1` are explicit non-admissible
profiles. A future live exact-accounting profile requires a separately reviewed
schema revision; changing a policy string cannot create it.

## Future collector data contract — still open

No collector is implemented here. A later separately reviewed, owner-approved
collector would have to normalize, retain, and cryptographically bind exact
raw sources for at least:

- systemd show properties for the two protected units, including exact
  `ExecMainStartTimestampMonotonic` microseconds;
- `/proc/<pid>/stat`, `exe`, `cmdline`, `cgroup`, and exact `cgroup.procs`;
- privileged listener ownership and retained coordinator health response bytes;
- a privileged enumeration of every `/dev/kfd` and `/dev/dri/renderD*` FD
  owner, including PID reuse protection;
- one pinned-kernel exact per-owner HMM allocation interface that reconciles to
  the same host aggregate;
- `GPUActive`, `GPUReclaim`, and physical capacity in exact bytes;
- boot ID, separately collected monotonic capture bounds, an enclosing
  cursor-bounded kernel journal interval and counts; and
- trusted time supplied by the separately approved transaction authority.

Missing, partial, permission-refused, racing, duplicated, foreign, or
unreconcilable data must produce `unreadable`, `refused`, or a typed admission
refusal. This document does not approve commands, sudo rights, target access,
or that future collector.

`GPUReclaim` is retained as diagnostic evidence only. V1 never subtracts it
from active HMM ownership or uses it to manufacture additional capacity.

## Offline use

The snapshot, policy, and resulting checked-in example are deliberately
synthetic. The result is retained so downstream offline validators can consume
the exact byte contract without inventing fields:

- snapshot SHA-256:
  `e942b3d9dcd10e9bddc5484d1e8eb19bf804cf29dae28851a420c13f8846837a`;
- policy SHA-256:
  `e75f18b1c564643bed5882afd377a70d9e6e78ca10572de6bd87a97c81984fab`;
- result SHA-256:
  `3dff6309c8ae51a3eda0d340e0228d41284897865329be993bee82e1b65c351a`;
- coordinator production-identity SHA-256:
  `3131533450592b2c6fe152095c4e15becdb4532e346d1747495d59370468a13a`;
  and
- worker production-identity SHA-256:
  `c622e76a01b08c180d63c008e08b1272e3e73f073cb2e3fa02d590b48054883b`.

```powershell
python -B scripts/halofpx_strix_hmm_admission.py `
  --snapshot scripts/halofpx-strix-hmm-admission-snapshot.example.json `
  --policy scripts/halofpx-strix-hmm-admission-policy.example.json `
  --trusted-now-utc 2026-08-13T07:01:00Z
```

Exit 0 means the exact offline inputs evaluate to `ADMIT`; exit 3 means a
well-formed typed `REFUSE`; exit 2 means malformed input or a local output
error. `--output` accepts only a new local file. The CLI cannot collect data or
execute work on a target.

Run the focused qualification with:

```powershell
python -X utf8 -B -m unittest tests.test_halofpx_strix_hmm_admission -v
```
