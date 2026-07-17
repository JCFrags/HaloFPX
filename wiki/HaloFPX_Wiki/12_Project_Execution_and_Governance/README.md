# 12 — Project Execution and Governance

Turns the technical design into an actionable, traceable implementation program.

Research status: draft-complete; needs machine validation and human policy decisions.

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
