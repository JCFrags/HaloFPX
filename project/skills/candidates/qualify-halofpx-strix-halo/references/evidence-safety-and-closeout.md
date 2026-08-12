# Evidence, safety, and closeout

## Authorization classes

Classify every action before execution:

| Class | Examples | Default |
|---|---|---|
| Planning/review | cards, source inspection, capacity arithmetic | proceed within project scope |
| Read-only target inspection | identity, versions, counters, health endpoints | proceed only if the user requested target inspection and it will not load the service materially |
| Local/off-target mutation | isolated worktree, build, generated fixtures | proceed when part of requested implementation/preparation |
| Target mutation | artifact staging, package/module/config/network/service changes | require explicit authorization and exact targets |
| Disruptive/fault | reboot, restart/stop, cable pull, kernel switch, sustained stress, corruption/fill/OOM | require explicit maintenance/fault authorization and recovery path |

Authorization does not transfer between phases, nodes, services, or runs. A request to inspect is not permission to install, reboot, benchmark, restart, or deploy. Skill invocation is never authorization.

## Experiment evidence bundle

Create a timestamped, stable run root under the project's experiment authority. Keep raw evidence immutable after finalization. Include:

- frozen approved card and its SHA-256;
- authorization record/scope and operator;
- environment manifests for both nodes;
- repository/build/model/tokenizer/workload/config/plan/schema identities;
- exact argv, cwd, environment, target, start/end UTC and monotonic timestamps, exit status, stdout, and stderr;
- topology/routes/socket/subflow state and before/after counters;
- one raw record per request/operation plus telemetry series;
- failure, cancellation, timeout, reject, retry, and cleanup records;
- integrity manifest covering raw artifacts;
- derivation code/runtime/library identities, command, seed, input hashes, output hashes, and reproduction receipt;
- deviations, stop/rollback actions, and final gate result.

Do not include secrets, raw sensitive prompts, unredacted hardware serials, credentials, or private chain-of-thought. Hash restricted identities before redaction when pair tracking requires it and preserve the restricted mapping only in an approved location.

## Claim labels

- `[MEASURED]`: directly observed in the named environment/run with linked raw evidence and applicability.
- `[VERIFIED]`: supported by exact primary source/code/standard evidence; not automatically true on the target.
- `[INFERENCE]`: reasoned from evidence but not directly proved.
- `[ASSUMPTION]`: provisional input that must be made explicit.
- `[RECOMMENDATION]`: proposed action or policy, not a fact or approval.
- `[OPEN]`: unresolved question or gate.

Never change a label merely because a result is favorable. Historical measurements remain historical. Schema validity proves structure, not truth, authorization, correctness, or release readiness.

## Stop rules

Predeclare numeric resource/thermal/time limits in the card. Always stop on:

- missing/expired authorization or changed scope;
- unresolved target, command, output, cleanup, stop, or rollback field;
- loss of SSH plus unavailable console/out-of-band recovery during target-changing work;
- unexpected active workload or inability to isolate the service;
- identity/hash/config drift after freeze;
- free-space reserve breach, material swap-in/PSI pressure, OOM, or runaway artifact growth;
- thermal/power ceiling breach or sustained clock collapse outside the card;
- output/logit/digest mismatch, stale/torn data, accepted corruption, or unexplained fallback;
- ambiguous rail/rank/epoch ownership, missing records, clock uncertainty outside the metric bound, or collector failure;
- new kernel, GPU, USB4, NVMe, filesystem, or integrity error;
- timeout/deadlock/unbounded queue or inability to clean up exact created state.

Do not continue to collect a more favorable result after a stop rule fires.

## Rollback discipline

Before mutation, record current service/process/listener/config/package/kernel/boot/module/network/artifact state and exact known-good identities. Define who may authorize rollback and which rollback steps are themselves disruptive.

Rollback in the smallest approved scope. Preserve failed logs and artifacts before restoring state. After rollback, verify management access, storage/filesystem, GPU/ROCm, both USB4NET rails, MPTCP baseline, expected service/listeners, and absence of test processes/files/modules/ConfigFS state. If any verification fails, mark rollback incomplete and stop.

Never delete the deployed runtime, current RPC cache, model, rollback artifact, workspace, or sole evidence copy as an implicit cleanup step.

## Closeout review

1. Verify the destination and hash manifest for every run artifact.
2. Validate schemas; quarantine invalid records without repair.
3. Recompute summaries from immutable raw inputs and compare hashes.
4. Report `PASS`, `FAIL`, `INCONCLUSIVE`, or `NOT AUTHORIZED` per gate and cell.
5. State exact applicability, controls, failures, uncertainty, residual risk, and current rollback state.
6. Route accepted observations into the relevant Wiki section with source links; route decisions to the decision ledger.
7. Record small safe improvements to candidate guidance. Do not promote this candidate skill until forward-testing and authorized use observations demonstrate that it is safe and sufficient.
