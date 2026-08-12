---
section_id: "86"
title: "Governance Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["HaloFPX integration repository (proposed)"]
  software_versions: ["GitHub repository workflow (proposed)", "MADR 4.0.0", "Semantic Versioning 2.0.0"]
  hardware_revisions: ["dual AMD Strix Halo / gfx1151 (planned)"]
related_sections: ["02", "03", "04", "05", "11", "15", "16", "73", "78", "82", "83", "84", "85"]
---

# Design implications

## Record identity and traceability

**[RECOMMENDATION]** Retain native GitHub identity as `owner/repository#number`, while linking immutable project records from section 03: `HLX-ADR-*`, `HLX-EXP-*`, `HLX-RUN-*`, `HLX-BUILD-*`, source IDs, and claim IDs. Do not mint `HLX-REQ-*`, `HLX-RISK-*`, or `HLX-REL-*` until section 03 and the owning sections approve those namespaces.

Section-local `OQ-86-*` and `EXP-86-*` strings are legacy aliases, never external identities. The open-question ledger now uses canonical `HLX-OQ-8601` through `HLX-OQ-8615`. For experiments, Section 84 remains the registry authority: Section 86 may propose a scoped alias to an existing card only after the card explicitly gains the required observations and controls. An alias does not mean the current card already covers that subprotocol, and one run cannot satisfy two scopes unless every predeclared acceptance rule is present [S86-04, S86-27].

## Cross-section execution contract

**[RECOMMENDATION]** Governance records should consume, by reference:

- Section 82 phase/epic/gate and rollback identifiers for milestone scope and exit;
- Section 83 risk IDs, current score/evidence state, owner role, acceptance expiry, mitigation, and fallback for blocking/waiver decisions;
- Section 84 canonical `HLX-EXP-*` card plus immutable `HLX-RUN-*` evidence for machine claims;
- Section 85 feed/backlog ID, freshness class, candidate revision, impact disposition, and revalidation trigger for upstream changes.

These are authoritative draft inputs within their declared applicability, not accepted policy. Human adoption, role assignment, exception authority, and release acceptance remain open and must be recorded rather than inferred [S86-25, S86-26, S86-27, S86-28].

Every issue should contain:

- one problem/outcome and an accountable human owner;
- requirement, risk, open-question, evidence/source, and affected-section links;
- acceptance criteria with observable pass/fail or an explicit research deliverable;
- dependencies, exclusions, rollback/fallback, and documentation impact;
- for distributed work: rank ownership, failure behavior, and single-node fallback;
- for persistent state: compatibility key and corruption/mismatch behavior.

## Portable label catalog

**[RECOMMENDATION]** Treat labels as orthogonal facets, not prose status. Bootstrap the same catalog in each HaloFPX repository and validate names/descriptions/colors from a versioned manifest.

| Facet | Initial values | Rule |
|---|---|---|
| `type/` | `bug`, `feature`, `research`, `experiment`, `docs`, `upstream-sync`, `release` | Exactly one; reconcile with GitHub organization issue types if adopted |
| `area/` | `formats`, `cpu`, `hip`, `vulkan`, `fabric`, `halokv`, `server`, `build-ci`, `packaging`, `wiki` | One or more |
| `status/` | `needs-triage`, `needs-evidence`, `needs-decision`, `ready`, `blocked`, `in-review` | At most one; closed is native issue state |
| `priority/` | `p0`, `p1`, `p2`, `p3` | One after triage; publish definitions |
| `risk/` | `security`, `data-loss`, `compatibility`, `quality`, `performance`, `license` | Additive; never substitute for risk ledger |
| `release/` | `breaking`, `feature`, `fix`, `docs`, `no-note` | Exactly one on merged material PRs; feeds release-note draft |
| `contrib/` | `good-first-issue`, `help-wanted` | Maintainer invitation, not automatic suitability |

Security-sensitive contents remain private; a public `risk/security` label is only for non-sensitive hardening work.

## Milestones and dependencies

**[RECOMMENDATION]** A milestone represents one outcome-bound Section 82 phase/gate, evidence campaign, or release candidate, with exact epic/gate scope, linked Section 83 risks, required Section 84 cards, applicable Section 85 freshness checks, target baseline, due date/condition, and owner. Do not use milestones for workflow status or permanent components. An item belongs to at most one active milestone; use native dependency edges for blockers and sub-issues for decomposition [S86-15, S86-16, S86-25, S86-26, S86-27, S86-28].

## ADR boundary

**[RECOMMENDATION]** Require `HLX-ADR-*` before accepting changes to:

- public API/configuration, model/GGUF/cache ABI or compatibility rules;
- distributed mode, rank/control ownership, transport, failure/fallback policy;
- security/trust boundaries, identity/isolation, secret handling;
- baseline/upstream strategy, license/distribution unit, dependency authority;
- release/version policy, irreversible migration, or governance exception.

Use MADR 4.0.0 structure plus section 04 metadata, evidence, decision authority, rollback, and reconsideration trigger. A PR may propose an ADR; the ADR becomes `accepted` only through its defined human authority. Implementation must not merge before required acceptance. Supersede; never rewrite accepted history [S86-05, S86-23].

## Review responsibility matrix

| Responsibility | Minimum duty | Independence rule |
|---|---|---|
| Issue owner | scope, acceptance, dependency upkeep | May author code; cannot waive mandatory evidence alone |
| Path/code owner | correctness, maintainability, tests | Not the latest material pusher for final approval |
| Decision authority | accept/reject ADR and exceptions | Named human, not agent or originating artifact |
| Experiment reviewer | protocol, comparison keys, raw bundle, exclusions | Different from run operator for release-blocking claims |
| Security owner | private intake, threat review, disclosure | Restricted membership and records |
| Release owner | baseline, notes, artifacts, rollback | Cannot infer readiness from milestone percentage |

**[RECOMMENDATION]** Protect `.github/`, `CODEOWNERS`, workflows, security policy, ADR schemas, release scripts, patch manifests, and AI-log validators with explicit owners. Because GitHub may accept one of several matching code owners, use separate required-review rules or a documented review check when multiple disciplines must approve [S86-18]. Identities remain **[OPEN]**.

## Pull request contract and definition of done

**[RECOMMENDATION]** A material PR declares: closing issue; requirements/open questions/ADRs; source and donor commits; patch lane; files and behavior changed; test IDs and raw evidence; performance-claim status; security/license/privacy review; docs/changelog/release label; AI-change record; rollback; unresolved risks.

A change is done only when:

1. scope and acceptance criteria are satisfied and no undisclosed material work remains;
2. exact source/dependency/build identities and provenance are recorded;
3. required fast, CPU, HIP, Vulkan, cache, distributed, quality, and performance gates pass for the affected surface—or a human-approved, time-bounded exception links a follow-up issue;
4. invalid/corrupt cache state misses or recomputes; rank ownership/failure/fallback is documented;
5. results are not promoted beyond their evidence label or applicability;
6. security, license, and data handling review is complete;
7. code, tests, docs, wiki claims, manifests, changelog entry, and migration/rollback notes agree;
8. required independent approvals and conversation resolution are present after the latest material push;
9. AI assistance is recorded under section 16 and a human understands every line;
10. merge is performed by an authorized human, the issue closure is verified, and release/post-merge observation is scheduled where needed.

No exception may permit secrets in logs, acceptance of corrupt persistent state, fabricated results, missing provenance for copied code, or an undisclosed security vulnerability.

## Release and contribution posture

**[RECOMMENDATION]** Keep a curated `CHANGELOG.md` for user-visible behavior and use GitHub-generated notes only as a reviewed draft. A release attaches exact tag/commit, baseline/build manifests, checksums, SBOM/notices/provenance, supported hardware/software tuple, known limitations, migrations, rollback/fallback, security notes, and linked experiments [S86-12, S86-22]. Adopt SemVer only after an ADR defines HaloFPX's public API [S86-24].

**[RECOMMENDATION]** `CONTRIBUTING.md` should specify setup, exact toolchains, topic branches, issue-first expectations, local checks, evidence handling, license/sign-off policy, AI-assistance policy, review behavior, security route, and upstream-target policy. When contributing upstream, the target repository's stricter contribution rules supersede HaloFPX convenience [S86-12, S86-21].
