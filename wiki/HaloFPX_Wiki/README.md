# HaloFPX LLM Wiki

This is the target wiki tree. Merge completed research-agent outputs into the exact numbered category and section paths.

## Authority model

Pages explicitly label verified facts, measurements, inferences, assumptions, recommendations, and open questions. Applicability is versioned. [`manifest.yaml`](manifest.yaml), validated by [`manifest.schema.json`](manifest.schema.json), is authoritative only for canonical section paths and structural artifact state; each section's `section.yaml` remains authoritative for its declared status and applicability. Neither structural completeness nor an applicability entry approves a software baseline or a claim. A page is not authoritative merely because it is newer.

The manifest is generated deterministically from `research/prompts/section_index.yaml` and the present section manifests:

```powershell
python research/prompts/tools/generate_wiki_manifest.py wiki/HaloFPX_Wiki
python research/prompts/tools/generate_wiki_manifest.py wiki/HaloFPX_Wiki --check
```

Validate both required artifacts and the permissive-core `section.yaml` contract with:

```powershell
python research/prompts/tools/validate_wiki.py wiki/HaloFPX_Wiki
python -m unittest research/prompts/tools/test_validate_wiki.py -v
```

The validator enforces registry identity/category, allowed status values, real ISO dates, non-negative and content-matched source/open-question counts, experiment and related-section shapes, and a non-empty applicability mapping. Section-specific extension keys and richer applicability values remain allowed.

## Core ledgers to maintain

- glossary and naming;
- sources;
- assumptions;
- open questions;
- decisions and ADRs;
- experiments and measured results;
- compatibility matrices;
- risks;
- implementation status.
