# llama-ai × CachyLlama feature inventory

Commit-pinned LLM Wiki and ROCmFPX portability assessment.

## Assessed revisions

- `fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722`
- pinned `fewtarius/CachyLlama@6be745998f568e379ea197fcf827baec73ff9940`
- target `charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394`

## Open the Wiki

Open [`index.html`](index.html) for the searchable dark-mode inventory, or start with [`Home.md`](Home.md) for a plain Markdown Wiki.

## Primary deliverables

- [`data/feature-inventory.csv`](data/feature-inventory.csv) and [`data/feature-inventory.json`](data/feature-inventory.json): 120 feature rows.
- [`data/retain-redesign-reject.csv`](data/retain-redesign-reject.csv): feature-by-feature porting decision table.
- [`20-Evidence-Index.md`](20-Evidence-Index.md), [`data/evidence.json`](data/evidence.json), and [`data/source-files.csv`](data/source-files.csv): exact commits, paths, locators, and URLs.
- [`schemas/`](schemas): normalized current configuration and proposed ROCmFPX persistent-cache schema.
- [`examples/`](examples): API and launch examples.
- [`22-ROCmFPX-Porting-Plan.md`](22-ROCmFPX-Porting-Plan.md): staged implementation plan and acceptance criteria.

## Result in one paragraph

CachyLlama demonstrates valuable restart-persistent target/draft/spec checkpoints, system-prefix reuse, continuation matching, slot affinity, router lifecycle, and Strix-focused tuning. ROCmFPX already has a materially safer run-scoped disk cache: private ownership, temporary-file publication, atomic rename, pair validation, failure circuit breaking, and focused tests. The recommended port is therefore an extension of ROCmFPX's current cache engine—not a transplant of CachyLlama's storage path—with authenticated tenant scoping, a versioned persistent manifest, structural system-prefix boundaries, unified cache lifecycle accounting, and target-native Strix/MTP kernels.

## Method boundary

This is static source analysis. No repository was built or executed and no hardware benchmark was reproduced. Maturity and portability are assessments, not release guarantees.
