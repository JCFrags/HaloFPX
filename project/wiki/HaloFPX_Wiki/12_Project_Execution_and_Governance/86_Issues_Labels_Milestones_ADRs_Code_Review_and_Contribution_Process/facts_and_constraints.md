---
section_id: "86"
title: "Governance Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["Custom_Inference_Project", "HaloFPX integration repository (proposed)"]
  software_versions: ["GitHub Docs 2026-07-17 snapshot context", "MADR 4.0.0"]
  hardware_revisions: ["dual AMD Strix Halo / gfx1151 (planned)"]
related_sections: ["01", "02", "03", "04", "05", "11", "12", "13", "14", "15", "16"]
---

# Facts and constraints

## Verified platform behavior

| ID | **[VERIFIED]** behavior | HaloFPX constraint |
|---|---|---|
| F86-01 | Issue forms live on the default branch under `.github/ISSUE_TEMPLATE/*.yml`; a PR template may live in root, `docs`, or `.github` [S86-14]. | Templates are versioned input aids, not enforcement by themselves. |
| F86-02 | GitHub labels classify repository issues, PRs, and discussions; label changes in one repository do not affect another [S86-15]. | A label catalog must be bootstrapped and audited per repository. |
| F86-03 | Milestones group issues and PRs and expose due date and completion derived from open/closed membership [S86-15]. | A milestone percentage is closure state, not test coverage or readiness proof. |
| F86-04 | Native issue dependencies express blocking relationships; creating them requires at least triage permission [S86-16]. | `status/blocked` is a visible hint; the dependency edge is authoritative. |
| F86-05 | Closing-keyword links auto-close only when the PR targets and merges into the default branch [S86-16]. | Candidate/release-branch PRs require explicit/manual linkage and closure handling. |
| F86-06 | Rulesets can require a PR, approvals, code-owner review, successful status checks, conversation resolution, and an approval from someone other than the latest pusher [S86-17]. | Branch rules enforce minimum mechanics; project evidence gates still require CI design. |
| F86-07 | CODEOWNERS is read from `.github/`, root, or `docs/` in that order. If several owners match one rule, one approval can satisfy required code-owner review [S86-18]. | Cross-lane review cannot be encoded merely by listing multiple owners on one pattern. |
| F86-08 | A `SECURITY.md` can publish supported versions and reporting instructions; public repositories may enable private vulnerability reporting [S86-19]. | Public issues must not be the vulnerability intake route. |
| F86-09 | GitHub recommends minimum `GITHUB_TOKEN` permissions, immutable full-SHA action pins, and avoiding public-repository workloads on persistent self-hosted runners because untrusted code can compromise them [S86-20]. | The two valuable Strix hosts must not execute untrusted fork code with secrets or persistent project state. |
| F86-10 | GitHub surfaces `CONTRIBUTING` guidance to issue and PR authors [S86-21]. | Setup, testing, review, license, AI-assistance, and security expectations should be discoverable before work begins. |
| F86-11 | GitHub releases are tag-based; generated notes summarize merged PRs and contributors and can categorize by labels [S86-22]. | Generated notes are a draft; they do not prove compatibility, completeness, or safe rollback. |

GitHub documentation is mutable. The observed `github/docs` HEAD was `df4329a271f3a195338ed6ab8cd493e1a413444f` on 2026-07-17, but rendered pages do not expose a page-specific deployment revision; refresh before implementation [S86-14, S86-15, S86-16, S86-17, S86-18, S86-19, S86-20, S86-21, S86-22].

## Existing HaloFPX authority

- **[RECOMMENDATION]** Sections [01](../../01_Wiki_Governance/01_Wiki_Architecture_Navigation_and_Root_Manifest/README.md)–[05](../../01_Wiki_Governance/05_Research_Data_and_Benchmark_Artifact_Conventions/README.md) remain authoritative for navigation, evidence, IDs, ledgers, and experiment bundles [S86-02, S86-03, S86-04, S86-05, S86-06].
- **[VERIFIED]** Current source observations remain pinned to llama.cpp `788e07dc91d266ad3162a1ce9037665656269689`, ROCmFPX `a5605a72768c6562241b248e268e33dc92787394`, CachyLLama `6be745998f568e379ea197fcf827baec73ff9940`, and llama-ai `1017f3dfdce3ca2b06aa9007b23295db3bb35722` as of 2026-07-17 [S86-07, S86-08, S86-09, S86-10, S86-11, S86-12]. These observations are not approved product baselines.
- **[RECOMMENDATION]** Section [15](../../03_Repository_and_Engineering/15_Integration_Patch_Stack_and_Upstream_Synchronization_Strategy/README.md) owns patch lanes and immutable candidates; section [16](../../03_Repository_and_Engineering/16_Build_Dependencies_Licensing_CI_and_AI_Agent_Workflow/README.md) owns build, license, CI, and AI-change evidence [S86-11, S86-12].
- **[VERIFIED]** MADR 4.0.0 provides a released Markdown decision template; section 04 already defines HaloFPX-specific ADR statuses and fields [S86-05, S86-23].
- **[VERIFIED]** Section 82 now owns the proposed dependency-aware roadmap phases, epics, gates, rollback points, and MUP boundaries; it explicitly makes no schedule commitment or readiness claim [S86-25].
- **[VERIFIED]** Section 83 now owns the living risk register and its update, acceptance, expiry, fallback, and blocking contracts. Its role names are provisional, and residual-risk acceptance still requires named human authority [S86-26].
- **[VERIFIED]** Section 84 now owns the canonical `HLX-EXP-*` physical experiment sequence, card completeness, notebook structure, and promotion gates. Section-local experiment names are aliases only and do not establish equivalent coverage [S86-27].
- **[VERIFIED]** Section 85 now owns upstream-watch inputs, observed/candidate/qualified identity separation, freshness classes, impact propagation, and revalidation triggers. Feed observation is not baseline promotion [S86-28].

## Constraints not solved by GitHub

**[INFERENCE]** GitHub closed state cannot establish that an issue met its acceptance criteria; a reviewer and deterministic checks must verify closure evidence.

**[INFERENCE]** CODEOWNERS maps paths to accounts, not responsibilities such as experiment validity, security disclosure, license approval, or release authority. HaloFPX needs a named responsibility matrix in addition to path ownership.

**[INFERENCE]** SemVer is useful only after HaloFPX defines its public API and compatibility surface. Until that ADR exists, version labels are release-planning hints, not verified compatibility statements [S86-24].

**[ASSUMPTION]** The future implementation repository may be hosted on GitHub. If another forge or local-only workflow is selected, preserve the semantic contract while replacing platform-specific enforcement.

## Current cross-project gaps

| Gap | Consequence | Required resolution |
|---|---|---|
| No commit or remote in this checkout | No reviewable baseline or hosted issue authority | Create/select canonical implementation repository through a recorded decision |
| No approved requirement ID namespace | Issue-to-requirement links cannot be validated | Section 03 plus requirements authority must approve a namespace |
| No owner identities or decision authorities | CODEOWNERS and approval gates cannot be configured honestly | Human project owner names roles and accounts |
| No issue/PR forms, rules, or CI | Proposed DoD is not enforceable | Implement and test governance configuration in the future repository |
| No contributor, conduct, or security policy | External intake and private reporting are undefined | Approve `CONTRIBUTING.md`, conduct policy, and `SECURITY.md` |
| No ADR, changelog, release, or AI-log storage | Decisions and releases lack durable traceability | Approve canonical paths and schemas |
| Sections 82-85 are draft authorities but have no approved governance adoption/owner mapping | Their contracts can be linked consistently, but cannot yet be enforced or accepted by implication | Human decision authority adopts or revises the contracts, names owners, and records exceptions without rewriting source sections |
