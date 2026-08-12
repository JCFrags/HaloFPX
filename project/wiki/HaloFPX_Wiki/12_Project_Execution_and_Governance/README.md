# 12 — Project Execution and Governance

## Category manifest

- **Purpose:** Route technical design into traceable work, risks, experiments, and reviews.
- **Authoritative files:** This manifest, the five linked section artifact sets, and Project Lead records.
- **Current owner:** Project Lead owns work authorization. Documentation workers own routing.
- **Status:** Draft complete. Machine validation and human policy choices remain open.
- **Last verified date:** 2026-07-29 for routing. Section claims retain their own dates.
- **Source commits:** Documentation baseline `d30814ed08fe395f1bb1d292281ce82edb6bdab4`; implementation commits remain in accepted decisions.
- **Related decisions:** [Project Lead decisions](../../../project-management/lead/DECISIONS.md) and [decision map](../decision-map.md).
- **Related evidence:** [Evidence map](../evidence-map.md) and [current state](../../../CURRENT_STATE.md).
- **Open work:** Keep worker tasks, reviews, and evidence linked to the active Project Lead boundary.
- **Next safe action:** Start at [`WORKER_START_HERE.md`](../../../WORKER_START_HERE.md) and confirm task ownership.

Turns the technical design into an actionable, traceable implementation program.

Research status: draft-complete; needs machine validation and human policy decisions.

The canonical titles use architecture decision record (ADR) and continuous integration (CI).

## Authoritative draft pages

- [82 — Implementation Roadmap, Epics, Dependencies, and Exit Criteria](82_Implementation_Roadmap_Epics_Dependencies_and_Exit_Criteria/README.md) owns proposed phases, minimum useful products, dependencies, and exit gates.
- [83 — Risk Register, Failure Modes, Mitigations, and Contingencies](83_Risk_Register_Failure_Modes_Mitigations_and_Contingencies/README.md) owns provisional risk scoring, triggers, mitigations, and fallback requirements.
- [84 — On-Machine Research Plan, Experiment Cards, and Lab Notebook](84_On_Machine_Research_Plan_Experiment_Cards_and_Lab_Notebook/README.md) owns the canonical physical experiment sequence and card contract.
- [85 — Internet Research Backlog, Upstream Watch, and Knowledge Freshness](85_Internet_Research_Backlog_Upstream_Watch_and_Knowledge_Freshness/README.md) owns feed authority, freshness classes, revalidation triggers, and change propagation.
- [86 — Issues, Labels, Milestones, ADRs, Code Review, and Contribution Process](86_Issues_Labels_Milestones_ADRs_Code_Review_and_Contribution_Process/README.md) owns the proposed evidence-to-change workflow and definition of done.

Structural completion is not implementation readiness. All five sections remain `needs-machine-validation`; repository hosting, human owners, security contacts, acceptance thresholds, and policy approvals remain open.

## Cross-category dependencies

- Sections 01-05 govern evidence, claims, identifiers, ledgers, and research artifacts.
- Sections 11-16 govern source baselines, patch provenance, builds, licensing, CI, and coding-agent records.
- Sections 18-81 own the machine facts, architecture contracts, experiments, and acceptance evidence consumed by this category.
- Evidence flows through sources and experiments into reviewed Wiki claims, then into human-approved ADRs, implementation work, and release gates; a governance artifact cannot approve its own unsupported claim.
