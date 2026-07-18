---
type: decision
status: accepted-for-local-implementation
date: 2026-07-18
decision: OPEN-PIN-01
---

# ROCmFPX implementation-pin decision

## Decision

Select `charlie12345/ROCmFPX@61f2f2d7bc4955e9bca821095ef69125837133b5`
(tree `0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`) as the immutable local
implementation base. Preserve `a5605a72768c6562241b248e268e33dc92787394` as
the research/control revision. This closes `OPEN-PIN-01` for starting the local
fork; it is not release qualification.

## Basis

- Both revisions built from the locked offline bundle on nimo-1 and nimo-2.
- Cross-node hashes matched for each revision.
- Candidate CPU reference, TurboQuant 7/7, ROCmFP4 quant regression, and ROCm0
  `FLASH_ATTN_EXT` 2899/2899 cases passed.
- F16 and Turbo4 small-model request/load/teardown smoke passed on both nodes for
  both revisions, with same-mode outputs equal across nodes and revisions.
- Candidate private one-rail RPC smoke passed with remote-allocation evidence.
- The candidate's bounded TurboQuant attention staging is relevant to the target
  memory objective and is a direct child of the retained control.

Evidence: `experiments/2026-07-17-open-pin-01-build-qualification/RESULTS.md`,
`experiments/2026-07-17-open-pin-01-runtime-smoke/RESULTS.md`, and
`experiments/2026-07-17-open-pin-01-rpc-smoke/RESULTS.md`.

## Limits

Turbo4-versus-F16 quality, broader RPC/security qualification, full feature-off
equivalence, performance, and release gates remain open. The implementation must
not silently advance this pin.
