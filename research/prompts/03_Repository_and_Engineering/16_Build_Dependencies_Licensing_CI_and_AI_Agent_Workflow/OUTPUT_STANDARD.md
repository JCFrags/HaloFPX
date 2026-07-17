# HaloFPX LLM Wiki — Section Output Standard

Each research assignment must return one self-contained folder at the exact category/section path supplied in the prompt.

## Required files

```text
<category>/<section>/
├── README.md
├── facts_and_constraints.md
├── design_implications.md
├── procedures_and_checks.md
├── open_questions.md
├── sources.md
├── section.yaml
├── diagrams/              # optional
├── scripts/               # optional
├── data/                  # optional; small derived data only
└── evidence/              # optional; logs, command output, checksums, excerpts
```

Create every required Markdown/YAML file even when a file only states that the topic is not applicable.

## Required front matter

Every Markdown file begins with:

```yaml
---
section_id: "NN"
title: "Page title"
status: "draft"            # draft | verified | needs-machine-validation | superseded
last_verified: "YYYY-MM-DD"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: []
related_sections: []
---
```

`section.yaml` must summarize the section ID, title, category, status, last verification date, source count, open-question count, required machine experiments, related sections, and applicability.

## Claim discipline

Label material conclusions using these markers:

- **[VERIFIED]** Directly supported by cited primary evidence.
- **[INFERENCE]** Reasoned conclusion from cited facts; explain the inference.
- **[ASSUMPTION]** A project premise that has not yet been proven.
- **[RECOMMENDATION]** Proposed design or operating choice.
- **[OPEN]** Unresolved question or research dependency.
- **[MEASURED]** Result from a reproducible experiment; link raw data and environment metadata.

Do not convert a repository claim, vendor claim, or one-machine benchmark into a universal fact. Do not invent measurements.

## Sources

Prefer primary sources in this order: exact source code and commit, official documentation, standards, research papers, official issue/PR discussion, then clearly identified secondary material. For every source record:

- stable source ID;
- title and publisher/repository;
- URL or local path;
- commit, tag, release, document revision, or publication date;
- access date;
- claims supported;
- limitations or conflicts.

Summarize rather than copying large copyrighted passages. Preserve short quotations only when exact wording is necessary.

## LLM-oriented writing

- Use stable, descriptive headings and ASCII filenames.
- Keep each page focused and generally below 2,500 words; split large material into linked subpages.
- Put the highest-value facts and design implications near the top.
- Use tables for matrices, interfaces, version compatibility, and open questions.
- Use Mermaid for editable diagrams and retain the source.
- Cross-link by relative path and stable section ID.
- Avoid duplicate explanations; identify the authoritative page.
- Record contradictions instead of silently selecting one source.

## Procedures and scripts

Commands must be reproducible, state prerequisites, mark whether root access is required, and avoid destructive defaults. Scripts should support `--help`, validation, dry-run where relevant, clear errors, and machine-readable output when practical.

## Research split

Every section must explicitly separate:

1. Internet/source-code research that can be completed now.
2. Measurements or inspection required on the actual two Strix Halo machines.
3. Decisions that remain contingent on those measurements.
