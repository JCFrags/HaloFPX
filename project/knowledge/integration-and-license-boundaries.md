# Integration and license boundaries

## Governing posture

- **[VERIFIED]** ROCmFPX and the preserved CachyLLama engine are MIT-licensed at their captured roots; llama-ai carries GPLv3 text and is a separate orchestration repository. [Repository manifest](../sources/repositories/manifest.yaml)
- **[RECOMMENDATION]** Do not merge or cherry-pick the CachyLLama head, its broad initial SSD commit, or llama-ai wholesale. The useful cache behavior spans the initial change plus many later correctness fixes, and ROCmFPX already has a stronger target-native transactional prompt-cache core. [Donor patch map](../reviews/intake/2026-07-17__donor-file-commit-patch-map__review__v01.md)
- **[RECOMMENDATION]** Preserve ROCmFPX behavior by default and add selected capabilities through small, buildable, bisectable, default-off lanes. The initial exact-cherry-pick roster is empty. [Accepted Phase 0A plan](../reviews/plans/2026-07-17__rocmfpx-llama-ai-fork-plan__draft__v03.md)

## Treatment boundary

| Treatment | Use when | Required evidence |
|---|---|---|
| Retain target-native | ROCmFPX already owns equal or stronger behavior | exact target source, baseline fixture, feature-off regression |
| Manual port or interface adaptation | A narrow MIT donor behavior has exact provenance and fits a stable target seam | introducing commit, dependency closure, file/blob license, attribution, tests, rollback |
| Clean reimplementation | The behavior is useful but copying is incompatible, entangled, or unsafe | approved behavioral specification, separated roles/contexts, prior-exposure record, permissible evidence, comparison review |
| Reject or defer | The feature weakens safety, lacks provenance, expands scope, or has no measured value | documented reason and re-entry gate |

**[OPEN]** Every selected capability remains below the plan's P3 provenance state until its exact commits, prerequisites, authorship, license disposition, treatment, tests, notices, distribution consequence, and reviewer approval are recorded.

## Hard license boundaries

1. **[RECOMMENDATION]** GPL llama-ai runner, service, router, installer, profiles, and documentation code stay outside the intended MIT engine tree unless the owner explicitly changes the distribution decision.
2. **[RECOMMENDATION]** Operational behavior may be used as a requirements source, but a “clean-room” label alone is not legal or provenance proof. A person or agent that inspected GPL implementation must not be silently reassigned as the independent implementer.
3. **[RECOMMENDATION]** Every adapted MIT unit preserves authorship, exact source commit/path/blob, license, and notice obligations. Broad donor commits and later fixes must not be collapsed into a generic attribution.
4. **[VERIFIED]** The implementation-start policy keeps an MIT core, excludes llama-ai GPL code and CC-BY-NC-SA documentation, admits no models/WebUI/tokenizers/templates by placement, and requires per-capability P3 records for CachyLLama material. **[OPEN]** Exact release-tree notices, SBOM, clean-room roles, and legal/distribution approval remain the release portion of `OPEN-LIC-01`. [PF-IR-04 review](../reviews/intake/2026-07-18__pf-ir-04-licensing__review__v01.md)

## Architectural boundaries

- **[RECOMMENDATION]** Keep request normalization, trusted scope resolution, scheduler policy, state codec, match policy, store provider, retention, and telemetry as separate responsibilities.
- **[RECOMMENDATION]** Preserve current ROCmFPX per-run/ephemeral cache semantics and defaults. A new persistent provider must use distinct names, modes, roots, schema, and rollback controls.
- **[VERIFIED]** The deployed RPC cache stores large model tensors, not attention KV, prefix state, or session continuation. It remains a separate namespace, schema, quota, and threat boundary. [RPC cache audit](../sources/measurements/2026-07-17-strix-halo-live-inventory/rpc-cache-audit.md)
- **[RECOMMENDATION]** Server-side direct import of donor `KVRC`, `KVSM`, or `KVPG` records is rejected. Any legacy reader begins as an offline, bounded, read-only inventory/migration tool, and legacy bytes never become trusted hits without independent integrity and compatibility proof.
- **[OPEN]** Exact API compatibility surfaces, admitted state codecs, persistent format, authenticated scope semantics, and storage policy must be approved before a server persistence lane opens.
