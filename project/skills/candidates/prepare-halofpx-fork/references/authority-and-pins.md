# Authority and source roles

Use this reference when freezing or comparing source baselines. Re-read the cited project files before relying on any value; these are retrieval aids, not independent authority.

## Precedence

1. Use `PROJECT_GOAL.md` for the product destination and phase order.
2. Use exact Git objects and `sources/repositories/manifest.yaml` for code identity and observed root-license evidence.
3. Use retained live captures for machine state.
4. Use the canonical Wiki for reviewed research context and OPEN questions.
5. Use accepted reviews for bounded recommendations; never let imported Wikis or draft plans approve themselves.

## Current roles

| Role | Identity | Constraint |
|---|---|---|
| Product lineage | `charlie12345/ROCmFPX` | User-selected canonical base; exact implementation pin remains gated. |
| Frozen research control | `a5605a72768c6562241b248e268e33dc92787394` | Preserve for research applicability and comparison. |
| Implementation candidate | `61f2f2d7bc4955e9bca821095ef69125837133b5` | Recommendation only; qualify against the research control before selection. |
| Operational requirements donor | `fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722` | GPL wrapper snapshot; requirements reference unless distribution policy changes. |
| MIT engine donor | `fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940` | Capability-level provenance and treatment approval required before import. |
| Donor comparison parent | `ggml-org/llama.cpp@92366df30d4eaa4b85139b5fd694360237731b19` | Comparison anchor, not the HaloFPX base. |
| Wiki upstream control | `ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689` | Research applicability control, not an automatic implementation base. |
| Deployed rollback baseline | `charlie12345/rocmfp4-llama@4860505ee322091f0f61eba77d6ad49be88cf4ea` | Measured operational baseline, not HaloFPX source lineage. |

## Controlling project material

- `knowledge/source-baseline-and-authority.md`
- `knowledge/integration-and-license-boundaries.md`
- `sources/repositories/manifest.yaml`
- `reviews/intake/2026-07-17__donor-file-commit-patch-map__review__v01.md`
- `reviews/plans/2026-07-17__rocmfpx-llama-ai-fork-plan__draft__v03.md`
- `reviews/plans/2026-07-17__rocmfpx-llama-ai-fork-plan__review__v03.md`

## Donor treatment baseline

- Preserve ROCmFPX's stronger target-native prompt-cache transaction core.
- Add restart-persistent catalog/lifecycle behavior through a new, default-off provider seam only after the relevant gates close.
- Reject wholesale donor merges, the broad initial SSD commit as a cherry-pick, donor-native v3 formats, FNV identity as a trust gate, anonymous fuzzy cross-directory restore, request user IDs as authorization, and direct GPL wrapper copies into the intended MIT core.
- Treat donor commits primarily as provenance and regression-test seeds; the current approved direct-cherry-pick roster is empty.
