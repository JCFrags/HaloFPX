# Artifact manifest

| Class | Location | Purpose |
|---|---|---|
| Prebuilt wiki | `site/` | Browser-ready HTML, CSS, search index, and copied SVG diagrams |
| Wiki sources | `docs/` | Markdown pages suitable for review and version control |
| Standard project files | repository root | README, summary, license, contribution, security, changelog, citation metadata |
| Cost-model code | `tools/cost_model.py` | Reproducible formula calculator; no benchmark values embedded |
| Site builder | `tools/build_site.py` | Rebuilds the static wiki from Markdown |
| Validators and tests | `tools/validate_wiki.py`, `tests/` | Checks links, placements, arithmetic, and evidence-label coverage |
| Placement schema | `schemas/placement.schema.json` | JSON Schema for role, model, expert, KV, tokenizer, and sampler ownership |
| Placements | `placements/*.yaml` | Machine-readable layouts for each execution mode |
| Structured data | `data/*.csv`, `data/*.json` | Model fields, formula summaries, calculated worked examples, measurement templates |
| Execution diagrams | `diagrams/svg`, `diagrams/dot`, `diagrams/mermaid` | Rendered and editable topology diagrams |
| Benchmark helpers | `tools/probe_*`, `tools/benchmark_*` | Non-destructive inventory and link-test templates |
| Third-party runtime asset | `vendor/mathjax/` | Apache-2.0 MathJax bundle for offline equation rendering |
