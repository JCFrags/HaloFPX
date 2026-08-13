# ADR-0062: offline complete Strix adapter-evidence validator

**Status:** proposed; offline implementation and hosted synthetic qualification
only

**Date:** 2026-08-13

## Context

ADR-0057 requires a future maintenance controller to validate the complete
immutable PR-#51 adapter evidence tree. Its first offline controller slice
retained and replayed only one sparse adapter handoff receipt. That receipt can
bind ordering and a selected schedule row, but it cannot establish that every
frozen A/B schedule entry ran successfully, that all nested raw evidence still
matches its summaries, or that no unaccounted file was added after analysis.

The PR-#51 evidence core and CachyOS adapter already define the relevant plan,
preflight, schedule, process, request, cleanup, raw-sample, analysis, and hash
records. This decision adds a strict reader for that existing baseline profile.
It does not reinterpret a partial or failed tree as complete, repair evidence,
or create a new execution path.

## Decision

Add one offline validator that accepts only a complete, full-success PR-#51
run tree. The validator is closed-world and read-only. It derives the complete
expected inventory from the frozen plan, schedule, and warmup count; requires
every schedule entry exactly once; and rejects missing, duplicate, failed,
reserved, temporary, or extra content.

### Two-pass observed-change boundary

Validation has two non-mutating passes:

1. The first pass resolves the evidence root, walks without following links,
   rejects unsafe containment, and captures the exact path, file identity,
   byte length, bytes, and SHA-256 of every admitted file. Directories must be
   real contained directories. Files must be regular, non-link, non-reparse,
   single-link files. Symlinks, junctions, other reparse points, hard links,
   path aliases, traversal, case-colliding names, reserved names, staging
   names, and temporary files refuse.
2. The second pass repeats the tree census and file identity, length, bytes,
   and SHA-256 checks. Any *observed* change between passes, including an added
   or removed path, refuses. Only after the two captures agree does semantic
   validation reconstruct every relationship from the captured bytes without
   reopening a pathname.

This portable capture is valid only under trusted single-operator custody with
no hostile synchronized writer. It detects ordinary observed drift, but a
synchronized nested directory A-to-B-to-A replacement can evade both passes;
Windows/Python does not provide the held `openat`-style directory traversal
needed to close that ABA attack. This decision therefore makes no hostile-
concurrency, permanent-immutability, or multi-user storage-security claim.

### Exact inventory and core reconstruction

The admitted baseline inventory contains exactly the files implied by the
plan and schedule:

- frozen `plan.json`, `schedule.json`, `commands.json`, and both role
  preflights;
- the retained coordinator request and both retained machine-authority
  receipts, plus the immutable issue-#41 `incident.raw` and root-level
  `hmm-admission-snapshot.raw.json`, `hmm-admission-policy.raw.json`, and
  `hmm-admission-result.raw.json`;
- one adapter execution directory for every schedule index, each with its
  exact policy copy, intent, execution receipt, declared warmup cycles, and one
  measured cycle;
- every cycle's declared response, client timing, raw HTTP, journal,
  telemetry, GPU-census, terminal, and cleanup artifacts applicable to that
  cycle;
- one evidence-core raw sample directory for every schedule entry, including
  its byte-identical retained adapter receipt and declared raw artifacts; and
- the final `analysis.json`, `samples.jsonl`, `status.json`, and
  `SHA256SUMS` inventory.

The validator re-loads the closed plan and policy schemas, regenerates the
schedule and commands, validates both preflights, and checks the exact plan,
policy, preflight, input, model, command, condition, pair, order, and schedule
hashes throughout the tree. `SHA256SUMS` must be canonical, sorted, unique,
and complete for every other admitted file and no other path. It is checked
against freshly calculated digests, not trusted as an inventory source.

Resource admission is part of that closed contract, before schedule
expansion. The adapter tree allows at most 16,384 files/directories, depth 8,
16 MiB per file, and 256 MiB total. Plan maxima are coupled rather than
independently attainable: with `E=2*pairs`, the retained workload must satisfy
`E * (warmups_per_condition + 1) * output_tokens <= 262144`, in addition to
`pairs<=64`, `warmups_per_condition<=16`, `output_tokens<=65536`,
`prompt_tokens<=1048576`, and `context<=1114112`.

`analysis.json` must report every scheduled sample retained with no missing or
failed entry, `evidence_core_complete=true`, and
`execution_qualified=false`, `measurement_ready=false`, and
`performance_claim=false`. `status.json`, `samples.jsonl`, the raw sample
documents, and the re-derived analysis must agree exactly. A partial tree or a
correctly retained failure remains valuable evidence, but this full-success
validator refuses it.

### Cross-field process and request custody

For every warmup and measured cycle, both rank roles are mandatory. The
validator cross-binds role, host, unit, PID, InvocationID, process-start and
boot identity, systemd start identity, executable SHA-256, argv, allowlisted
environment, cgroup, listener ownership, host-local readiness/live monotonic
proofs, terminal records, journals, and cleanup-completion proofs. Both worker
and coordinator readiness records bind the exact process identity, boot, and
host-local observation time; replay across cycles refuses. Warmup and measured
identities are fresh, each next role-local process follows its predecessor's
proved cleanup, and cleanup proves
the exact captured PID, cgroup, unit, and port absent. The protected before
and after snapshots must be structurally equal.

Both machine-authority receipts must bind the separately retained root-level
`hmm-admission-result.raw.json` bytes through their exact
`hmm_admission_result_sha256`. The snapshot, policy, and result files are all
mandatory. The validator calls ADR-0064's exact sibling
`validate_bound_admission_result_bytes(result, snapshot, policy)` API, which
reparses the closed schemas, checks the result's exact snapshot and policy
digests, and requires byte-for-byte canonical recomputation at the retained
trusted timestamp. The result-only validator explicitly refuses every overall
or per-role `ADMIT`; a result envelope alone cannot satisfy this gate.

The recomputed `halofpx.strix-hmm-admission-result.v1` document must bind issue
#41, contain exactly the coordinator and worker roles, classify each role
`ADMIT`, and retain empty per-role and overall reason-code lists. Each role
must bind its exact host, non-null node-snapshot SHA-256, and a
production-identity SHA-256 equal to the role's expected digest supplied from
the maintenance authorization. Each role also carries a nonnegative
HMM-headroom byte count. The overall decision must be `ADMIT`, with
`target_execution_authority=false` and `performance_result=false`.
Those production-identity digests bind the active protected identities from
the maintenance authorization before its first mutation; they are not the
disposable A/B identities. Each machine-authority receipt separately records
the later post-stop/pre-adapter protected service baseline as inactive, PID
zero, no cgroup or listeners, and no process. The two time boundaries must not
be conflated.

The hosted fixture supplies only ADR-0064's checked-in synthetic snapshot,
policy, and result bytes. Bound canonical recomputation proves their internal
relationship; it does not prove who collected a future snapshot, authenticate
the retained trusted timestamp, freshen observations, authorize an owner or
window, consume a nonce, or grant target execution. Those remain separate
promotion gates.

The validator binds every disposable identity and role-local observation to
the exact ADR-0064 boot and to the half-open monotonic freshness window after
that role's captured snapshot. It also applies the policy `RuntimeMaxSec`,
half-open UTC expiry and snapshot-age limits, requires host-local cleanup before
the next cycle, and never compares monotonic values across the two PCs. These
are retained-clock consistency checks; they do not authenticate wall time.
ADR-0064's bound evaluator treats equality at its configured maximum snapshot
age as admissible. ADR-0062 deliberately narrows its complete-tree profile:
the retained trusted time, intent, and every role-local event must be strictly
before that maximum-age deadline; equality refuses.
ADR-0064 `planned_increment_bytes` is reported but is not derived from or
digest-bound to this adapter plan/model/allocation, so it cannot authorize the
workload allocation.

The initial and measured GPU censuses must be complete, error-free, and admit
only the exact disposable PIDs for the applicable host and interval. The
measured pre/request/post census records require one common two-role witness
wholly contained in the request interval. Each role's telemetry likewise needs
a wholly contained controller-clock witness; coordinator telemetry also binds
the same-host request clock. All samples remain inside their role-local
freshness and process-lifetime windows.

The validator rechecks the exact retained request bytes and sent-body hash;
the response JSON, client timing, and raw streamed HTTP hashes; HTTP and token
counts; `cache_n=0`; output-token and prompt-token counts; final timing record;
and deterministic output-content SHA-256. Those values must agree across the
cycle receipt, raw artifacts, evidence-core sample, and final analysis. A
different request serialization, token count, response body, output hash, or
cache reuse refuses. This remains a cold-cache-off evidence profile.
For the admitted native `/completion` performance profile, each predicted
token must produce one observable `stop=false` SSE partial and one retained
remote-host monotonic event stamp; the validator derives TTFT and all ITLs from
those stamps. Valid server streams that defer an incomplete UTF-8 partial are
not accepted as complete performance evidence. One exact source-shaped
`stop=true` terminal event is required, and `[DONE]` or error events refuse.
Every warmup and measured cycle must retain the same deterministic output hash,
even when the plan omits an optional golden digest.
The retained stamps are collector evidence: raw SSE binds their count and
event order but carries no timestamps from which their numeric values could be
independently reconstructed. Derived summaries inherit that collector-clock
limitation and are not target performance authority.

### Versioned profiles and integration

The validator admits the baseline PR-#51 tree and one explicit optional
profile: PR-#67's `sampling_output_sync_prometheus_v1`. Detection of either
reserved root document or any per-sample sidecar directory selects that
profile. Both root documents and one exact four-file directory for every
scheduled sample then become mandatory. The validator invokes the exact
sibling's authoritative `validate_frozen_run` against a private replay of the
captured bytes, compares the deterministically reconstructed analysis with the
retained analysis, and checks that every sidecar PID, InvocationID,
process-start tick, request interval, request hash, response hash, and client
hash agrees with its measured adapter cycle. Core or sidecar
`evidence_complete` alone is never admission. Partial, disabled, orphaned,
unknown, or future observability layouts refuse; none is silently ignored.

The offline maintenance controller invokes this validator on the complete
adapter tree returned by its deterministic fake. Its later closed maintenance-
bundle verifier performs the same cold, from-disk validation rather than
trusting an earlier in-memory result or the copied selected receipt. A
validation failure prevents adapter-success admission. It does not prevent
the controller's already-required cleanup and worker-first recovery behavior.

On rejection, recovery completes before failure finalization. The controller's
`halofpx.strix-maintenance-failure-custody.v1` report performs a bounded,
no-follow census (15,766 entries, depth 8, 16 MiB per file, 304 MiB admitted
pre-finalization bytes). Observed links/reparse points, hard links, special files, unsafe names,
oversize/deep/count/byte cases, read races, and directory drift become typed
metadata exclusions; observed directory drift removes all admitted descendants
from that traversal. `complete=false` means rejected bytes may intentionally be
absent from `SHA256SUMS`. The distinct `FAILED.json` marker binds only safely
admitted bytes plus the exclusion report and is never authorization or a
complete adapter tree.
The canonical exclusion report is capped at 4 MiB and retains at most 2,048
rows; each displayed path is capped at 512 UTF-8 bytes and binds the full path
by SHA-256. Omitted rows retain a count, reason counts, and aggregate digest.
`SHA256SUMS` is capped at 10 MiB. A 16 MiB finalization reserve gives a distinct
320 MiB cold-verification ceiling for the manifest, report, and final marker.

## Explicit non-authority

This decision does not add or construct a Runner, SSH client, target command,
service action, build, quantization, inference, or benchmark. It does not
change either literal target-execution gate. It establishes no CachyOS, ROCm,
`gfx1151`, model-correctness, latency, throughput, or performance result.

It also does not provide owner authentication, reviewed-source/executable
binding, trusted time, replay-proof nonce consumption, atomic two-node
authorization or terminalization, an independent recovery watchdog, or real
production-recovery authority. A structurally valid tree cannot supply those
missing capabilities. GitHub issue #41 remains open P0 and target work remains
`REFUSE`.

## Hosted qualification

Qualification uses one deterministic, hosted-only fixture that materializes a
complete PR-#51 tree through the real evidence-core and CachyOS adapter code
with a side-effect-free fake Runner. It executes every synthetic schedule row,
performs only fake requests, finalizes the ordinary analysis, then validates
the result from disk. No Strix Halo target, production service, model payload,
network endpoint, or benchmark is used.

Focused negative cases must cover missing and extra paths, temporary/reserved
content, symlink/reparse and hard-link aliases, between-pass replacement,
manifest drift, schedule omission/duplication, policy or preflight changes,
role/process/cgroup/listener/GPU mismatches, before/after or cleanup drift,
request/response/token/output tampering, non-cold samples, failed cycles,
false qualification claims, and unsupported observability sidecars.
HMM cases additionally cover a missing snapshot, policy, or result; tampering
of each input; coherent triple rebinding against stale machine-authority
receipts; result-only positive refusal; caller production-identity drift; and
overall or per-role refusal.
The supported PR-#67 profile additionally covers missing, partial, orphaned,
cross-swapped, counter-tampered, summary-tampered, analysis-tampered, and
adapter-identity/request-rebound sidecars.

Run the hosted validator qualification with:

```powershell
python -X utf8 -B -m unittest tests.test_halofpx_strix_adapter_evidence -v
```

## Relationship and rollback

ADR-0056 remains the versioned A/B comparison authority. ADR-0057 remains the
offline maintenance transaction and real-promotion-gate authority. ADR-0064
defines the mandatory bound HMM snapshot/policy/result recomputation consumed
here. This proposal replaces only ADR-0057's deliberately sparse adapter
handoff check with a complete-tree prerequisite; it does not supersede any
production gate.

Rollback removes the complete-tree reader, hosted fixture/tests, controller
calls, documentation, and this proposed ADR. It changes no target state,
stored model or cache format, existing evidence bytes, or feature-off runtime
behavior.
