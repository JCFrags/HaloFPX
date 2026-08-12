---
type: intake-review
status: accepted-with-restrictions
date: 2026-07-18
scope: PF-IR-04 licensing dossier
---

# PF-IR-04 licensing review

## Verdict

**[VERIFIED]** The archive `PF-IR-04-licensing-dossier.zip` is preserved at SHA-256
`4E8F437F53D9E24F3A2E7D160F6949B488592F6CF8C787B6CAA89603A400572C` and
extracts to 120 files. Import executed no bundled script.

**[RECOMMENDATION]** Accept the dossier as external licensing evidence, not legal
approval. It supports this implementation-start policy:

- keep the engine core MIT;
- exclude llama-ai GPL-3.0-or-later implementation and CC-BY-NC-SA-4.0
  documentation from the engine tree;
- use llama-ai behavior only as a requirements source under the approved
  provenance and clean-reimplementation process;
- admit CachyLLama material only after a per-capability P3 source, dependency,
  license, attribution, notice, test, and reviewer record;
- do not import models, tokenizers, templates, WebUI output, corpora, or release
  bundles merely because they appear in a repository;
- retain an empty direct-cherry-pick roster until an individual capability is
  approved.

This closes the external-evidence portion of `OPEN-LIC-01` and fixes the safe
scope for beginning target-native implementation. Human review remains required
for an exact distribution tree, SBOM/notices, clean-room roles, and release.

## Evidence routing

- Preserved package: `sources/imports/2026-07-18-pre-fork-internet-research/extracted/PF-IR-04-licensing-dossier/PF-IR-04-licensing-dossier/`
- Local exact-tree authority: `sources/repositories/source-locks/2026-07-17-pre-fork/`
- Governing boundary: `knowledge/integration-and-license-boundaries.md`
