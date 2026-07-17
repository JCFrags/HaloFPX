---
name: qualify-halofpx-strix-halo
description: Prepare, review, and execute evidence-gated qualification of a pinned HaloFPX candidate build on the two AMD Strix Halo targets, from reproducible build and capacity preflight through matched single-node baselines, stable dual-USB4 MPTCP operation, distributed correctness, rollback, and optional USB4STREAM experiments. Use for HaloFPX build qualification, nimo-1/nimo-2 test planning, target admission, single-to-dual-node promotion, transport comparison, large-model fit preflight, or release evidence. This project-local skill is a candidate and remains unvalidated until forward-tested and approved.
---

# Qualify HaloFPX on Strix Halo

## Status and authority

Treat this skill as **candidate/unvalidated**. It organizes the project evidence but does not approve a build, experiment card, deployment, benchmark threshold, target change, or release.

Resolve the active project root before acting. Read its `AGENTS.md`, `README.md`, `PROJECT_GOAL.md`, Wiki README, relevant section manifests, linked decisions, and the current fork plan. Read [authority-and-target-context.md](references/authority-and-target-context.md) before planning or interpreting target state.

Do not run this procedure merely because the skill was invoked. Separate planning, read-only inspection, local/off-target build work, target-changing work, and disruptive work. Obtain explicit user authorization for the exact phase and targets whenever work would install packages or kernels, reboot, restart/stop services, change routes/interfaces, load/unload modules, alter boot/configuration, stage large artifacts, consume material storage, run sustained load, inject faults, or access production model/cache state.

## Core contract

1. Preserve the existing operational runtime as a rollback baseline; never treat a source checkout as deployed state.
2. Freeze exact source, submodule, toolchain, dependency, model, tokenizer, configuration, and artifact identities before comparison.
3. Qualify correctness and resource safety on each node separately before dual-node work.
4. Keep TCP/MPTCP over `thunderbolt-net` as the default dual-rail baseline until a matched experiment proves another carrier.
5. Treat USB4STREAM as optional, reversible, and separately authorized; never make it a prerequisite for initial qualification.
6. Prove realized rank, tensor/layer, KV/state, sampler/output, transport, retry, epoch, and failure ownership. A requested split is not placement evidence.
7. Treat corrupt, partial, stale, incompatible, or unverifiable cache/checkpoint state as a miss or recomputation.
8. Preserve failures and rejected cells. Never repair raw evidence or promote an average without its population, controls, failures, and provenance.
9. Stop at the first failed prerequisite. A later performance result cannot waive identity, correctness, safety, storage, recovery, or rollback failure.

## Qualification workflow

### 1. Classify the request and authority

State the requested outcome, phase, nodes, services, repositories, model artifacts, test-data roots, evidence root, maintenance window, and authorization boundary. If the user requested only planning, review, diagnosis, or read-only inspection, do not cross into target mutation.

Map the work to canonical experiment cards in Wiki Section 84. Copy and version the card; never edit a completed card in place. Keep unresolved commands, targets, authorization, stop rules, or analysis fields explicit. A schema-valid draft is not runnable.

### 2. Admit the candidate build

Use the full checklist in [qualification-checklists.md](references/qualification-checklists.md). Require:

- a clean or fully described source state and pinned base/donor provenance;
- reproducible build inputs and cryptographic hashes for binaries and libraries;
- a separate build/staging area that does not overwrite the deployed runtime;
- static license/provenance disposition for imported or reimplemented features;
- compiler/linker/backend feature evidence and a recorded build receipt;
- a rollback artifact/configuration whose identity and restoration procedure are known.

Do not describe a successful compile as Strix Halo compatibility, runtime correctness, reproducibility, or deployment.

### 3. Freeze capacity and storage envelopes

Run the read-only/precomputed admission checks before model staging or load tests. Use actual artifact bytes and per-node observed capacity rather than nominal `2 x 128 GB` arithmetic. Account separately for weights, non-layer tensors, KV/special state, graph/work buffers, allocator fragmentation, process duplication, pinned transport buffers, page cache, swap policy, temporary conversion, verification, rollback copies, RPC tensor cache, HaloKV, logs, and reserve.

Require an approved free-space floor, peak staging calculation, disposable test root, cleanup owner, and recoverable rollback path. On current evidence, nimo-1 storage headroom is a hard gate; do not solve it by deleting or overwriting the existing RPC cache or other assets without explicit authorization.

### 4. Qualify each node independently

Execute the matched single-node card only after identity and measurement-system cards pass. Test nimo-1 and nimo-2 as separate subjects with the same pinned build/model/workload and record differences rather than normalizing them away.

Correctness precedes performance. Detect unsupported operations, undeclared fallback, tokenizer/template mismatch, state mismatch, cache mismatch, non-finite values, output divergence, OOM/pressure, thermal instability, and dirty teardown. Begin with the smallest safe model/context/load cell and advance through preregistered steps. Keep one correctness-passing, resource-bounded single-node configuration as the development/recovery baseline.

### 5. Qualify stable dual-rail MPTCP

Use the existing `thunderbolt-net`/MPTCP design as the control. Prove each rail's identity by address plus sysfs ancestry; never infer physical path from `tb0`/`tb1` numbering. Prove endpoint binding and counter attribution for A only, B only, A+B same direction, A+B opposite directions, and bidirectional cells.

Record MPTCP subflow creation, per-subflow bytes, fallback, retransmit/reorder/error behavior, CPU/IRQ cost, latency distributions, and integrity. A negotiated 40 Gb/s interface or two 20 Gb/s lanes is not achieved goodput, and two active subflows do not prove additive capacity or failure independence.

### 6. Qualify distributed inference

Advance only from passing per-host and fabric controls. Freeze a machine-readable placement/ownership plan and its hash. Begin with a correctness-oriented reduced workload, then progress to representative and capacity-extension cells.

Compare against matched single-node or two-replica controls where those controls can run. When the target model cannot fit one node, label the result capacity extension and use a smaller same-family control; do not publish an invalid speedup ratio. Include client-visible correctness, TTFT, ITL, throughput/goodput, memory, power/thermal, tails, and every failure in the evidence bundle.

Treat rank/link/process loss as a new epoch. Stop visible output commits, reject late completions, and recover only from a mutually verified checkpoint plus input/output ledger. Otherwise restart from the original prompt. Single-node fallback means a separately qualified fitting configuration in a new epoch, not partial-state continuation.

### 7. Consider USB4STREAM only as an optional branch

Read the USB4STREAM checklist in [qualification-checklists.md](references/qualification-checklists.md). First preserve and re-prove same-kernel USB4NET behavior. Candidate kernels must be reviewed, installed alongside known-good entries, and introduced one node at a time only under explicit authorization with console/out-of-band recovery.

Compare one carrier-neutral framed protocol over TCP and USB4STREAM with identical payloads, queueing, integrity, and failure semantics. Require bounded short-I/O handling, credits, authentication/peer identity, epoch fencing, cleanup, and post-test USB4NET smoke. Do not claim GPU-direct or zero-copy from a device node or allocation type; prove the end-to-end producer-to-consumer path.

Retain MPTCP if USB4STREAM fails correctness, rollback, cleanup, security, stability, or matched benefit gates. Do not invent numeric advancement thresholds; use approved Section 55/81 decisions or record the policy gap.

### 8. Close out and promote evidence

Follow [evidence-safety-and-closeout.md](references/evidence-safety-and-closeout.md). Preserve immutable raw outputs first, validate their schemas and hashes, derive summaries from hashed inputs, and independently reproduce material results. Route artifacts through `sources/experiments -> Wiki claims -> decisions`; keep this skill candidate until forward-testing and authorized use observations support promotion.

Report one of: `PASS`, `FAIL`, `INCONCLUSIVE`, or `NOT AUTHORIZED`. List the exact passing envelope, failed/rejected cells, residual risk, rollback state, evidence paths, and next gate. Never broaden a local or historical observation into a universal claim.

## Stop and escalation rules

Stop immediately when authorization is absent or ambiguous; exact targets or rollback are unresolved; the active service cannot be isolated; management/console recovery is unavailable for disruptive work; artifact identity changes; storage reserve or memory/PSI/thermal limits are crossed; outputs diverge; corruption or stale state is accepted; a rank/link identity is ambiguous; the kernel/GPU/storage/filesystem reports a new fault; cleanup fails; or evidence capture loses integrity.

Preserve the failed run and return the system to the approved baseline if rollback is authorized. If rollback itself would change target state beyond current authorization, stop and request direction rather than improvising.
