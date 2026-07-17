# HaloFPX Project

Treat this repository as a living, evidence-backed engineering wiki for the custom dual-Strix-Halo inference project.

## Read before changing project material

1. Read `README.md`, `wiki/HaloFPX_Wiki/README.md`, the relevant category manifest, and linked decisions.
2. Treat claim labels literally: `[MEASURED]` is environment-specific; `[VERIFIED]` requires primary evidence; `[INFERENCE]`, `[ASSUMPTION]`, and `[RECOMMENDATION]` are not facts; `[OPEN]` remains unresolved.
3. Prefer exact commits, model hashes, hardware revisions, and software versions over names such as "latest."
4. Do not fabricate benchmarks, compatibility, hardware capability, API behavior, or provenance.
5. Preserve raw sources and licenses. Keep research prompts in `research/prompts/`, researched pages in `wiki/HaloFPX_Wiki/`, experiments in `experiments/`, and import evidence in `sources/`.

## Evidence and improvement discipline

- Route evidence through `sources -> wiki -> implementation decisions`; do not promote unsupported claims.
- Treat memory and prior runs as scoped experience, not global truth.
- Link substantial work to a requirement, issue, decision, experiment, or wiki section.
- Retain raw data and environment metadata for performance claims and compare matched configurations.
- For distributed behavior, state rank ownership, failure behavior, and single-node fallback.
- For cache behavior, corruption must cause a miss or recomputation, never accepted invalid state.
- Before closing a task, review every material artifact for correctness, freshness, clarity, provenance, and reusable improvement. Apply small safe evidence-backed corrections; otherwise record a proposal under `reviews/`.
- Keep changes reversible and never silently replace verified material.

## Source precedence

Exact source code/commit -> official documentation or standard -> research paper -> official issue or PR -> clearly labeled secondary evidence.

The Agent Harness reference authority is `C:\Users\britt\Documents\Agent_Harness`; use `references/agent-harness.md` for routing.
