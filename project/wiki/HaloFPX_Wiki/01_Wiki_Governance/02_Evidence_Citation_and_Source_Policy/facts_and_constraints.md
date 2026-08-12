---
section_id: "02"
title: "Evidence Policy Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["CFF 1.2.0", "W3C PROV-O 2013"]
  hardware_revisions: []
related_sections: ["01", "04", "05"]
---

# Facts and constraints

- **[VERIFIED]** Git accepts full object names and unique leading substrings, but a full hash is the least ambiguous record for an exact commit [S02-03].
- **[VERIFIED]** W3C PROV-O distinguishes an `Entity`, an `Activity`, and an `Agent`, and includes relationships for primary source, revision, generation, and invalidation [S02-02].
- **[VERIFIED]** Citation File Format 1.2.0 is YAML 1.2 and can record a software commit, version, repository URL, authors, license, and references [S02-04].
- **[VERIFIED]** BCP 14 reserves uppercase requirement terms for their normative meanings when explicitly invoked [S02-05].
- **[VERIFIED]** The Agent Harness requires evidence-backed promotion and treats memory as scoped experience rather than truth [S02-06].

## Claim record minimum

**[RECOMMENDATION]** Each claim should be representable by:

```yaml
claim_id: "CLM-<section>-<number>"
label: "VERIFIED"
statement: "Atomic, falsifiable statement"
sources: ["S02-01"]
applicability:
  repository_commit: null
  software_versions: []
  hardware_revisions: []
  model_sha256: null
confidence: "high"
verified_at: "2026-07-16"
review_trigger: "upstream commit or environment change"
conflicts_with: []
```

Claim label and confidence are independent: a direct primary source can justify `[VERIFIED]` that a repository documents a behavior while confidence that the behavior works on HaloFPX remains low.

## Quotation and licensing constraints

- **[RECOMMENDATION]** Paraphrase by default. Quote only the minimum exact wording required and preserve attribution, revision, and license.
- **[RECOMMENDATION]** Do not copy large documentation or source fragments into the wiki. Preserve lawful snapshots or paths in `sources/`, including license and checksum.
- **[RECOMMENDATION]** Never store secrets, private prompts, model weights, or private chain-of-thought as evidence.

## Conflict rule

**[RECOMMENDATION]** Do not average or silently choose between conflicting sources. Record both, scope each by version, identify the higher-precedence evidence, and create an open question or experiment when applicability remains unresolved.
