# Custom ROCm engine fork preparation status

## Verdict

**READY TO BEGIN THE LOCAL CUSTOM FORK.**

The implementation base is frozen at
`charlie12345/ROCmFPX@61f2f2d7bc4955e9bca821095ef69125837133b5` (tree
`0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`). The offline source lock, two-node
build/runtime/RPC qualification, complete PF-IR research intake, safe licensing
boundary, feature-off API contract, and static conformance fixtures are ready.

## Authorized first implementation lane

1. Restore the locked ROCmFPX bundle into a new writable local repository.
2. Create `codex/integration-base-61f2f2d` at the exact selected commit.
3. Record the clean base manifest and reproduce the feature-off baseline.
4. Add contracts and target-native seams before importing donor code.
5. For each capability, produce a P3 provenance/license record, choose retain,
   manual MIT port, clean reimplementation, or defer, then implement behind a
   default-off flag with tests and rollback.

The initial direct-cherry-pick roster is empty. llama-ai GPL code and its
separately licensed documentation stay outside the MIT engine. Models, WebUI,
tokenizers, templates, and corpora are not implicitly admitted.

## Deliberately later gates

- Remote GitHub fork creation still needs owner, repository name, visibility,
  permissions, protection, signing, and push authority (`OPEN-GOV-01`). This does
  not block local implementation.
- The seven executable/state PF-IR-10 assets require isolated qualification.
- Persistent writes require format, state completeness, scope, corruption,
  crash, storage-reserve, quota, rollback, and acceptance gates.
- Deployment, dual-node large-model optimization, performance claims, and release
  require their matched evidence and human approvals.

Canonical plan: `reviews/plans/2026-07-17__rocmfpx-llama-ai-fork-plan__draft__v03.md`.
Readiness gates: `knowledge/implementation-readiness-gates.md`.
