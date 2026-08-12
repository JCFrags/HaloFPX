---
section_id: "86"
title: "Open Governance Questions"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["Custom_Inference_Project", "HaloFPX integration repository (proposed)"]
  software_versions: []
  hardware_revisions: ["dual AMD Strix Halo / gfx1151 (planned)"]
related_sections: ["03", "04", "05", "15", "16", "73", "78", "82", "83", "84", "85"]
---

# Open questions

| ID | **[OPEN]** question | Resolution evidence | Authority/dependency |
|---|---|---|---|
| HLX-OQ-8601 | What is the canonical implementation repository, forge, visibility, and default branch? | Approved repository/hosting ADR and initialized baseline | Project owner; sections 11/15 |
| HLX-OQ-8602 | Which human accounts fill issue, code, decision, experiment, security, and release roles? | Approved responsibility matrix and tested review routing | Project owner |
| HLX-OQ-8603 | Which stable namespaces identify requirements, risks, releases, and governance exceptions? | Section-03 schema update with collision/migration tests | Sections 03, 82, 83 |
| HLX-OQ-8604 | Which forge plan/features are available, and are rulesets or branch protection the enforcement authority? | Hosting inventory plus fixture-repository test | Repository administrator |
| HLX-OQ-8605 | What public API and compatibility surface permits SemVer, and is CalVer needed for baselines? | Release/version ADR with examples and upgrade tests | Release authority; sections 66-72 |
| HLX-OQ-8606 | What exact test and matched-performance thresholds block PRs and releases? | Baseline variance, quality, reliability, and cost evidence | Sections 73-81 |
| HLX-OQ-8607 | How are the two Strix self-hosted runners isolated, scheduled, cleaned, and prevented from running untrusted PR code? | Threat model and amended `HLX-EXP-20260717-849` CI-isolation subprotocol (legacy alias `EXP-86-02`) | Security/CI; section 16 H16-O07 |
| HLX-OQ-8608 | What changes require two human approvals or multiple disciplines beyond ordinary CODEOWNERS? | Trial review matrix and risk analysis | Security/decision authority |
| HLX-OQ-8609 | What security contact, acknowledgement target, disclosure timeline, severity scheme, and supported-version policy apply? | Approved `SECURITY.md` and private reporting drill | Security authority |
| HLX-OQ-8610 | What contribution license/sign-off, code-of-conduct, authorship, and AI-disclosure rules apply? | Legal/owner review and contributor walkthrough | Section 16 H16-O10/O11/O14 |
| HLX-OQ-8611 | Where do ADRs, requirement/risk ledgers, governance exceptions, and AI records canonically live? | Storage ADR, schemas, deterministic index validation | Sections 01, 03, 04, 16 |
| HLX-OQ-8612 | What maximum duration and approval level apply to a gate exception, and which gates are never waivable? | Exception-policy ADR and failure exercises | Security/release authority |
| HLX-OQ-8613 | How will labels/milestones/issues be synchronized or audited across a future multi-repository split? | Versioned manifest plus reconciliation dry run | Project governance |
| HLX-OQ-8614 | How are generated release notes reconciled with curated changelog, security advisories, migration notes, and unpublished internal changes? | Release rehearsal and editorial checklist | Release authority |
| HLX-OQ-8615 | Which human authority adopts the completed Sections 82-85 draft contracts as enforceable governance inputs, assigns their owner roles, and approves any deviations? | Accepted governance/roadmap ADR or equivalent owner decision, responsibility map, exception path, and cross-link audit | Project owner and named decision authority; Sections 82-85 |

Legacy aliases `OQ-86-001` through `OQ-86-015` map positionally to `HLX-OQ-8601` through `HLX-OQ-8615`. New external links must use the canonical IDs.

## Internet/source follow-up

1. Refresh official GitHub ruleset, issue-form, Actions-security, advisory, and release documentation immediately before configuration; record the selected plan and page revisions.
2. Inspect the current contribution and security policies at the exact intended upstream commits before any upstream submission.
3. Compare MADR releases after 4.0.0 and adopt a newer template only through an explicit schema/decision migration.
4. Monitor SemVer and release-platform changes, but do not adopt version semantics until HaloFPX's public API is defined.
5. Review relevant upstream issue/PR templates and label taxonomies as interoperability input, not as inherited authority.

## Repository and machine follow-up

1. Amend `HLX-EXP-20260717-850` before running and independently reproducing the legacy `EXP-86-01` scope in a disposable fixture repository.
2. Amend `HLX-EXP-20260717-849` before running the legacy `EXP-86-02` scope without valuable credentials or data; include an attempted fork-PR escape and cleanup audit.
3. Amend `HLX-EXP-20260717-850` before executing the legacy `EXP-86-03` scope using a small reversible change that touches both-node validation without claiming a performance result.
4. Add the distinct legacy `EXP-86-04` release/rollback scope to `HLX-EXP-20260717-849` before using disposable release/cache artifacts; prove incompatible state is rejected or recomputed.
5. Audit ten completed issues/PRs after initial adoption; record missing links, false closures, review bottlenecks, and label drift as an improvement proposal.

## Contingent decisions

No label catalog, milestone schedule, owner assignment, branch rule, performance threshold, version scheme, security SLA, or contribution requirement becomes project policy solely because it appears as a recommendation in this section. Acceptance requires the named authority, implementation, and validation evidence.
