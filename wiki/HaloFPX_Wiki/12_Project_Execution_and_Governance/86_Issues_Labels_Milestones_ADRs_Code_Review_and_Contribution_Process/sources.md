---
section_id: "86"
title: "Governance Sources"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["Custom_Inference_Project", "HaloFPX integration repository (proposed)"]
  software_versions: ["GitHub Docs observed 2026-07-17", "MADR 4.0.0", "Semantic Versioning 2.0.0"]
  hardware_revisions: []
related_sections: ["01", "02", "03", "04", "05", "11", "12", "13", "14", "15", "16"]
---

# Sources

Access date for all records: **2026-07-17**. GitHub rendered documentation is mutable; `github/docs@df4329a271f3a195338ed6ab8cd493e1a413444f` was the independently observed default-branch HEAD, but the deployed page-to-commit mapping was not exposed.

| ID | Primary source and revision | Claims supported | Limitations/conflicts |
|---|---|---|---|
| S86-01 | Local `Custom_Inference_Project` inspection: `git rev-parse --verify HEAD`, `git remote -v`, and presence checks; 2026-07-17 | Checkout has no commit/remote and lacks proposed governance files | Local state only; can change after this section |
| S86-02 | [Section 01](../../01_Wiki_Governance/01_Wiki_Architecture_Navigation_and_Root_Manifest/README.md), verified 2026-07-16 | Manifest, authority, retrieval, link rules | Recommendations not yet fully implemented |
| S86-03 | [Section 02](../../01_Wiki_Governance/02_Evidence_Citation_and_Source_Policy/README.md), verified 2026-07-16 | Claim/source/provenance discipline | Machine applicability remains separate |
| S86-04 | [Section 03](../../01_Wiki_Governance/03_Glossary_Naming_and_Stable_Identifiers/README.md), verified 2026-07-16 | Stable existing project namespaces | Requirement/risk/release namespaces absent |
| S86-05 | [Section 04](../../01_Wiki_Governance/04_Assumption_Open_Question_and_Decision_Ledgers/README.md), MADR HEAD `835fc94baa37887774b1cddddb2ae874881e703b` observed 2026-07-16 | Ledger types, ADR states, acceptance/supersession | Storage/authority not adopted |
| S86-06 | [Section 05](../../01_Wiki_Governance/05_Research_Data_and_Benchmark_Artifact_Conventions/README.md), verified 2026-07-16 | Immutable run bundles and matched comparisons | No HaloFPX measurements here |
| S86-07 | [Section 11](../../03_Repository_and_Engineering/11_Repository_Lineage_Branches_Commits_and_Frozen_Baselines/README.md), pins refreshed 2026-07-17 | Source pins, baseline compound object, immutable tags | Observed heads are not accepted baselines |
| S86-08 | [Section 12](../../03_Repository_and_Engineering/12_Codebase_Architecture_and_Module_Map/README.md), verified 2026-07-16 | Cross-module and distributed/cache validation surfaces | Machine work open |
| S86-09 | [Section 13](../../03_Repository_and_Engineering/13_ROCmFPX_Feature_Kernel_Format_and_Patch_Inventory/README.md), ROCmFPX `a5605a72768c6562241b248e268e33dc92787394` | Format/backend gates and non-measurement boundary | No target-machine qualification |
| S86-10 | [Section 14](../../03_Repository_and_Engineering/14_llama_ai_and_CachyLLama_Feature_and_Patch_Inventory/README.md), CachyLLama `6be745998f568e379ea197fcf827baec73ff9940`, llama-ai `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | Selective port, persistence/isolation gates | No two-node proof |
| S86-11 | [Section 15](../../03_Repository_and_Engineering/15_Integration_Patch_Stack_and_Upstream_Synchronization_Strategy/README.md), Git docs 2.54.0 | Patch lanes, candidate immutability, closeout | Proposed strategy awaiting machine validation |
| S86-12 | [Section 16](../../03_Repository_and_Engineering/16_Build_Dependencies_Licensing_CI_and_AI_Agent_Workflow/README.md), verified 2026-07-16 | CI/license/AI-log/release engineering gates | Legal and machine decisions open |
| S86-13 | Agent Harness `C:\Users\britt\Documents\Agent_Harness\AGENTS.md`, `guide/architecture.md`, `reviews/AGENTS.md`; local canonical files, inspected 2026-07-17 | Evidence promotion, independent review, reversible improvement | Conceptual/local policy; HaloFPX rules take precedence |
| S86-14 | GitHub Docs, [issue and PR templates](https://docs.github.com/en/communities/using-templates-to-encourage-useful-issues-and-pull-requests/about-issue-and-pull-request-templates), rendered 2026-07-17 | Template paths/default-branch behavior | Mutable rendered documentation |
| S86-15 | GitHub Docs, [managing labels](https://docs.github.com/en/issues/using-labels-and-milestones-to-track-work/managing-labels) and [about milestones](https://docs.github.com/en/issues/using-labels-and-milestones-to-track-work/about-milestones), rendered 2026-07-17 | Repository-local labels; milestone grouping/progress | Does not define HaloFPX taxonomy/readiness |
| S86-16 | GitHub Docs, [issue dependencies](https://docs.github.com/en/issues/tracking-your-work-with-issues/using-issues/creating-issue-dependencies) and [linking PRs to issues](https://docs.github.com/en/issues/tracking-your-work-with-issues/using-issues/linking-a-pull-request-to-an-issue), rendered 2026-07-17 | Blocking edges, permissions, default-branch closing behavior | Platform behavior; manual linkage still needed in some flows |
| S86-17 | GitHub Docs, [available rules for rulesets](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-rulesets/available-rules-for-rulesets), rendered 2026-07-17 | PR/review/status/conversation/latest-push gates | Availability varies by owner/repository/plan; inspect before adoption |
| S86-18 | GitHub Docs, [about code owners](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-code-owners), rendered 2026-07-17 | File location, base-branch behavior, one-of-many approval | Does not encode full responsibility matrix |
| S86-19 | GitHub Docs, [adding a security policy](https://docs.github.com/en/code-security/how-tos/report-and-fix-vulnerabilities/configure-vulnerability-reporting/add-security-policy) and [private vulnerability reporting](https://docs.github.com/en/code-security/how-tos/report-and-fix-vulnerabilities/report-privately), rendered 2026-07-17 | Supported-version/reporting policy and private reporting | Feature/visibility-dependent; no HaloFPX contact exists |
| S86-20 | GitHub Docs, [Secure use reference for Actions](https://docs.github.com/en/actions/reference/security/secure-use), rendered 2026-07-17 | Token least privilege, action SHA pins, workflow ownership, self-hosted runner risk | Guidance does not prove local runner isolation |
| S86-21 | GitHub Docs, [setting contributor guidelines](https://docs.github.com/en/communities/setting-up-your-project-for-healthy-contributions/setting-guidelines-for-repository-contributors) and [contributing to open source](https://docs.github.com/en/get-started/exploring-projects-on-github/contributing-to-open-source), rendered 2026-07-17 | CONTRIBUTING discovery and contributor expectations | Examples are not HaloFPX policy |
| S86-22 | GitHub Docs, [about releases](https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases) and [generated release notes](https://docs.github.com/en/repositories/releasing-projects-on-github/automatically-generated-release-notes), rendered 2026-07-17 | Tag-based releases, generated-note inputs/categories | Generated notes are not completeness or correctness evidence |
| S86-23 | MADR, [Markdown Architectural Decision Records 4.0.0](https://github.com/adr/madr/tree/4.0.0), tag target `61cc5891d9e160c0db8b18d5fd45f9123a7a1c21`, released 2024-09-17 | Released decision template and numbered Markdown records | HaloFPX adds its own metadata/status rules |
| S86-24 | Semantic Versioning, [Specification 2.0.0](https://semver.org/spec/v2.0.0.html), versioned specification; accessed 2026-07-17 | Version semantics require a declared public API | Does not define HaloFPX's API or guarantee compatibility |
| S86-25 | [Section 82](../82_Implementation_Roadmap_Epics_Dependencies_and_Exit_Criteria/README.md), completed 2026-07-17 | Proposed phases, epics, dependency gates, MUP boundaries, rollback points, and machine work M82-01 through M82-12 | `needs-machine-validation`; no schedule commitment, accepted baseline, or release approval |
| S86-26 | [Section 83](../83_Risk_Register_Failure_Modes_Mitigations_and_Contingencies/README.md), completed 2026-07-17 | Living risk register, scoring boundary, blocking rule, update/acceptance/expiry/fallback contract | Owner roles and acceptance authorities are provisional; scores are prioritization, not measured probabilities |
| S86-27 | [Section 84](../84_On_Machine_Research_Plan_Experiment_Cards_and_Lab_Notebook/README.md), completed 2026-07-17 | Canonical `HLX-EXP-*` physical experiment sequence, card/notebook contract, alias deduplication, promotion gates | Cards are proposed and unrun; a candidate alias does not prove protocol coverage or equivalence |
| S86-28 | [Section 85](../85_Internet_Research_Backlog_Upstream_Watch_and_Knowledge_Freshness/README.md), completed 2026-07-17 | Watch backlog, observed/candidate/qualified identity split, freshness classes, impact and revalidation contract | Observations are checked-at snapshots; monitoring does not authorize upgrade or policy adoption |

## Source precedence and conflicts

Exact HaloFPX repository configuration and committed policy will supersede generic GitHub examples once adopted. Official GitHub documentation governs platform behavior; project ADRs govern chosen use. Existing sections govern evidence and technical gates. A conflict is recorded as an open question or ADR input rather than resolved by the newest prose alone.
