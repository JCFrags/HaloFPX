# Quality-assurance report

**Research cut:** 2026-07-17  
**Artifact version:** 1.0.0

## Executable checks

- 9 standard-library unit tests passed.
- 18 deterministic fault-injection cases passed.
- The bound HaloFPX fixture returned `IMPORT_CANDIDATE_VALID` with `eligible_for_engine_import=true` and `eligible_for_hit=false`.
- The authenticated but request-unbound fixture returned `CATALOG_ENTRY_VALID` with both eligibility flags false.
- No offline path emitted public hit eligibility.
- A same-length CachyLLama payload mutation remained structurally valid but untrusted, demonstrating the observed legacy integrity blind spot.
- The equivalent HaloFPX payload mutation returned `MISS_RECOMPUTE` through digest failure.

Canonical machine-readable result: [`validation/results/validation-summary.json`](validation/results/validation-summary.json).

## Artifact checks

- All Python sources compile syntactically; `validation/run_all.sh` passes `bash -n`.
- Six Graphviz diagrams render successfully to SVG.
- All JSON files parse; all CSV files have consistent row widths.
- Four Kaitai Struct schemas parse as YAML.
- The HaloFPX manifest JSON Schema is valid Draft 2020-12, and the generated sample validates against it.
- Every local Markdown link resolves inside the folder.
- The pre-rendered offline HTML contains 558 anchors and 318 references; all local anchors and files resolve.
- The offline HTML embeds its stylesheet and SVG diagrams; immutable upstream source links remain external by design.

## Limits

This QA is source-static and fixture-based. It does not claim that the audited upstream servers were compiled or exercised, that opaque engine-state semantics were independently decoded, that physical power-cut behavior was tested, or that a particular SSD's firmware/endurance was measured. Those limits are preserved in the Wiki's evidence labels and scope chapter.
