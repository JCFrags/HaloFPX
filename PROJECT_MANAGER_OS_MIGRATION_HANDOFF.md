# HaloFPX Project-Manager OS Migration Handoff

Purpose: allow a new project-manager agent on a clean OS to restore, verify,
reorganize if necessary, and continue HaloFPX without relying on chat history.

Status date: 2026-07-25  
Implementation is intentionally paused after terminal L43.

## Read this first

The canonical project manager must begin with these files:

1. `Custom_Inference_Project\AGENTS.md`
2. `Custom_Inference_Project\README.md`
3. `Custom_Inference_Project\PREPARATION_STATUS.md`
4. `Custom_Inference_Project\project-management\lead\OBJECTIVES.md`
5. `Custom_Inference_Project\project-management\lead\CURRENT_STATUS.md`
6. `Custom_Inference_Project\project-management\lead\DECISIONS.md`
7. `Custom_Inference_Project\project-management\lead\MONITORING.md`
8. `Custom_Inference_Project\project-management\lead\monitor-state.json`
9. `Custom_Inference_Project\L20-L43_PROJECT_REPORT.md`
10. `Custom_Inference_Project\references\agent-harness.md`
11. `Agent_Harness` routing/instructions referenced by that file.
12. `HaloFPX\AGENTS.md`, if present.
13. `HaloFPX\docs\halofpx\l40-rpc-authenticated-graph-authority.md`
14. `HaloFPX\docs\halofpx\l42-scheduler-execution-authority.md`
15. `HaloFPX\docs\halofpx\l43-rpc-mutable-update-authority.md`
16. The corresponding L40/L42/L43 evidence receipts and adversarial reviews.

Treat exact source and retained raw evidence as authority. Do not convert
`[MEASURED]`, `[INFERENCE]`, or `[OPEN]` statements into universal facts.

## Backup contents and canonical repositories

The OS-migration backup contains complete copies of:

- `Custom_Inference_Project` — Wiki, research, raw sources, experiments,
  project-management state, plans, and evidence.
- `HaloFPX` — complete Git repository, working tree, accepted implementation,
  rejected-milestone documentation, and raw evidence.
- `Agent_Harness` — reference authority used by the project.

Expected Git state at migration:

| Repository | Branch | Expected HEAD | Expected worktree |
|---|---|---|---|
| HaloFPX | `codex/integration-base-61f2f2d` | `aba0f78d07c824c3bcdbcb5ffbdc26e174cda3bf` | clean |
| Custom_Inference_Project | `codex/pre-fork-preparation` | updated migration-handoff commit recorded in backup manifest | unrelated retained user/experiment files may be untracked; do not delete them |

HaloFPX has no configured remote. Do not invent one or push until the user
chooses repository owner, name, visibility, protection, and credentials.

## New-OS restoration procedure

1. Copy the entire migration folder from the server backup to local storage.
2. Verify the backup manifest and SHA-256 inventory before editing anything.
3. Restore the three canonical folders under a stable workspace root. Original
   Windows paths are not required, but preserve folder names and record any new
   absolute paths.
4. Run `git status`, `git branch --show-current`, `git rev-parse HEAD`, and
   `git fsck --full` in both Git repositories.
5. Confirm HaloFPX is at the exact expected clean commit. Do not reset a dirty
   tree; inventory and preserve unexpected files first.
6. Update absolute Windows paths in project-manager routing documents only
   after recording the old-to-new mapping. Avoid broad blind replacement inside
   raw evidence, receipts, hashes, or frozen manifests.
7. If reorganization is needed, keep one canonical copy, preserve provenance,
   and create a move receipt. Never delete raw sources, evidence, licenses,
   archives, rejected-milestone records, or Git history.
8. Re-establish SSH aliases `nimo-1` and `nimo-2` and test read-only access.
9. Inspect production services read-only before authorizing any implementation
   or runtime work.
10. Update `CURRENT_STATUS.md`, `DECISIONS.md`, and `monitor-state.json` with
    the new paths, verified commits, service state, and management task ID.

## Production preservation boundary

At handoff, the last verified service authority was:

- nimo-2:
  `minimax-m27-rpc-worker.service`, system scope, PID `1535639`, port `50052`,
  `NRestarts=0`.
- nimo-1:
  `minimax-m27-q6-server.service`, system scope, PID `2356329`, port `8081`,
  HTTP `200`, `NRestarts=0`.

These PIDs are historical and will change. The service names, exact system
scope, cgroup, command line, listeners, restart counters, and HTTP health are
the verification authority. Do not accept listeners alone.

The current service is the standard UD-Q6 production baseline. It is not proof
that HaloFPX or its persistent cache is deployed. Preserve it unless a reviewed
milestone explicitly authorizes a controller-managed transition. Recovery order
is always nimo-2 worker first, verify port `50052`, then nimo-1 coordinator,
verify model load and HTTP `200`.

## Project objective and non-negotiable rules

Build a maintainable ROCmFPX-derived HaloFPX engine for AMD Strix Halo that:

1. Keeps ROCmFPX performance advantages.
2. Adds safe SSD-backed persistent context state without importing GPL
   llama-ai code into the MIT-intended engine.
3. Treats corruption, incompatibility, missing ranks, partial state, or failed
   authentication as cache miss/recompute.
4. Supports exact single-node fallback and controlled dual-node operation.
5. Is not accepted as faster unless matched, repeated evidence proves it.
6. Ultimately improves the pinned 160 GB MiniMax ROCmFPX workload and then the
   200–230 GB dual-USB4 model class.

Never:

- Fabricate benchmark, compatibility, or “fastest” claims.
- Enable new behavior by default before correctness and rollback pass.
- Repeat primary-model transitions merely to gather more smoke evidence.
- Keep rejected runtime candidates in accepted source.
- Mutate production without a frozen manifest, exact controller ownership,
  bounded recovery, one-run authority, and independent review.
- Delete unrelated or untracked user files from either workspace.

## Accepted implementation boundary

HaloFPX currently includes these important accepted layers:

- L21 closed transition/evidence contract.
- L25 bounded SSH transport and recovery.
- L27/L28 RPC epoch and two-fresh-residency lifecycle.
- L30 Q8_0 apply-geometry correction.
- L37 lifetime-safe authenticated result publication.
- L40 `halofpx.rpc-graph-authority.v1`.
- L42 `halofpx.scheduler-execution-authority.v2`.

L43 is **not** accepted implementation. Its candidate was removed. Its evidence
is useful because it demonstrated real SET, SET_HASH, unflagged input,
nested/strided view, server readback, and output sensitivity before review
identified two reusable-layer blockers.

## Exact next milestone: L44

Do not start with another primary run. Open L44 as a no-primary/no-production
correction of exactly two L43 blockers.

### Required design

1. Replace process-global pointer registration with an explicit
   admitted-session handle.
2. Bind each registration and mutation to session, attempt nonce, graph UID,
   execution sequence, RPC connection/allocation epoch, and lifetime.
3. Make concurrent sessions non-interfering; refuse stale, closed, foreign,
   reused, or cross-session handles.
4. Ensure teardown removes all registrations and leaves no global pointer
   authority.
5. Preserve structural/call-site role classification; never use names, sizes,
   or `GGML_TENSOR_FLAG_INPUT` alone.
6. Exercise the real RPC server handlers with malformed/tampered update,
   duplicate/out-of-order sequence, out-of-bounds range, wrong view, and omitted
   reconstructed-leaf cases.
7. Retain real server-applied digest receipts for SET and SET_HASH and bind the
   complete mutable census to accepted L40 graph identity and L42 scheduler
   execution identity.
8. Feature-off must remain behaviorally and wire compatible with no traversal,
   hashing, allocation, synchronization, or logging.

### L44 qualification

Use focused protocol/session/refusal tests plus one disposable real RPC
compute/recompute evidence session. Avoid broad model matrices and unrelated
smoke suites. Obtain independent adversarial review. If rejected, remove the
candidate and retain exact evidence. Do not automatically open the next lane.

### After L44 passes

Authorize exactly one primary two-fresh-residency, one-token discriminator with
L40 + L42 + L44 authority enabled:

- Model bytes: `159873097824`.
- SHA-256:
  `96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.
- Expected cold/reference token: `21549`.
- Historical restored token: `9283`.

Require equality of RPC graph, scheduler transcript, mutable census,
server-applied updates, coordinator state, worker state, synchronized logits,
and final token before any cache promotion. One run only. If it fails, localize
the earliest unequal authenticated boundary instead of adding a fault matrix.

If it passes, proceed in order:

1. Guarded real-server persistent-cache product canary.
2. Corruption/missing/incompatible state as miss/recompute.
3. Bounded quota, reserve, eviction, crash durability, rollback, and lifecycle.
4. Matched feature-off/cold/cache performance gates.
5. Matched engine comparisons for the pinned model.
6. Only then dual-node USB4 performance optimization for 200–230 GB models.

## Project-management operating method

- Use one primary implementation task with a clean milestone boundary.
- Use independent read-only specialists only for bounded domain review.
- Require workers to report directly at terminal/blocker boundaries.
- Do not poll on short intervals. Prefer task events.
- Record each accepted/rejected decision in `DECISIONS.md`, the current state in
  `CURRENT_STATUS.md`, and machine-readable state in `monitor-state.json`.
- At every terminal milestone, verify exact Git state, cleanup, production
  authority when touched, evidence identity, and independent review.
- Accept useful negative results. Do not mistake extensive testing for product
  progress.
- Split foundations when a combined candidate repeatedly produces partial
  guarantees.

## Backup verification receipt

The final Desktop and server paths, file counts, byte totals, and manifest
hashes are recorded in the migration folder's backup manifest. The intended
server destination is nimo-2 because it had sufficient free space; nimo-1 had
only about 31 GB free at migration time.

