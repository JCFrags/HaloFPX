# Readiness gates and stop conditions

Use this reference before actions that could change code, trust, remotes, tools, or target machines. Confirm current state in `knowledge/implementation-readiness-gates.md` and the accepted v03 plan review.

## Gate map

| Gate | Required closure evidence | Default while OPEN |
|---|---|---|
| G0A local source identity | Selected candidate; exact commits/trees/gitlinks; clean state; bundles; patch/license/build-input manifest and hashes | Permit only reviewed local read-only freezing and static archaeology. |
| G0B repository governance | Owner/name, visibility, fork form, permissions, protections, force-push/tag/signing rules, evidence location, create/push authority | Forbid remote creation, configuration, tag, and push. |
| G0C test assets | Stage 1 hashes/licenses/static review; Stage 2 approved isolated qualification; Stage 3 exact-manifest promotion | Forbid imported-tool execution outside Stage 2 and all use before promotion. |
| G1 provenance/license | Every selected capability at P3; treatment, dependency closure, attribution/notices/SBOM, distribution and clean-reimplementation decisions | Forbid donor code import, adaptation, cherry-pick, or reimplementation. |
| G2 target baseline | Selected ROCmFPX builds and characterized behavior on both nodes with exact manifests and approved fixtures | Forbid claiming target compatibility or baseline equivalence. |
| G3 feature-off equivalence | Defaults, API, scheduler, current cache, quantization, MTP/speculative, backends, and RPC match the approved baseline | Forbid feature promotion. |
| G4-G7 persistent safety | Transactional state, corruption-as-miss, crash/storage safety, isolation, quota/reserve, diagnostics, and rollback | Forbid persistent write enablement and production reads. |
| G8 target matrix | Both nodes pass exact build/runtime matrix with controlled skew and rollout evidence | Forbid cluster qualification claims. |
| G9 value | Human-approved thresholds from matched variance; correctness and quality preserved | Forbid performance/value claims. |
| G10 release | Immutable manifest/tag/artifacts, deployment receipt, rollback proof, independent review | Forbid release claims and cutover. |

## Provenance states

- P0: behavior claim only; do not import.
- P1: repository license classified; do not import.
- P2: exact commits, parentage, paths/blobs, authors, license/SPDX, prerequisites, and upstream overlap mapped; do not import.
- P3: treatment, attribution, notices, reviewer, tests, and distribution consequence approved; proceed only inside the separately authorized lane.

## Candidate test-asset stages

1. Perform non-executing review of provenance, hashes, licenses, subprocess/network behavior, paths, dependencies, and outputs.
2. Execute only under a separately approved isolated contract using synthetic fixtures, no credentials, no target-node access, no production/model/cache roots, and no unapproved network.
3. Promote only the exact accepted manifest after human/reviewer approval.

## Non-negotiable stops

- Treat corruption, truncation, mismatch, unauthorized scope, or incomplete rank state as miss/recompute before context mutation.
- Keep the deployed RPC tensor cache separate from attention/session persistence.
- Preserve the deployed service and rollback artifacts; never modify both live nodes at once.
- Do not delete the existing RPC cache, run fault injection, reboot, change kernels/packages/services, or share persistent roots across binary generations without explicit reviewed authorization.
- Begin 200-230 GB dual-node optimization only after the Phase 1 release passes correctness, rollback, and single-node recovery gates.

## Current reviewed readiness

The accepted v03 review authorizes `L00A - local source/provenance freeze` only. "Read-only" applies to reference clones, Git objects, imported evidence, remotes, and target systems; it still permits creating new inventories, bundles, manifests, provenance/license records, and static mappings inside approved project evidence paths. It does not authorize source commits, pushes, imported-tool execution, donor import, persistence implementation, deployment, service interruption, or destructive testing.
