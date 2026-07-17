---
section_id: "86"
title: "Governance Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["HaloFPX integration repository (proposed)"]
  software_versions: ["GitHub repository workflow (proposed)"]
  hardware_revisions: ["dual AMD Strix Halo / gfx1151 (planned)"]
related_sections: ["02", "03", "04", "05", "15", "16", "73", "78", "82", "83", "84", "85"]
---

# Procedures and checks

These procedures are proposed; they were not applied to a hosted HaloFPX repository. Ordinary steps need no root. Machine fault injection must use disposable data and declare privileges before execution.

## 1. Intake and triage

1. Route suspected vulnerabilities to the private procedure below. Do not request exploit details in public.
2. Search open/closed issues, ADRs, open questions, and upstream trackers for duplicates.
3. Select an issue form: bug, feature/design, research/experiment, upstream-sync, or documentation.
4. Assign native issue identity, one `type/`, one `status/`, one priority after triage, affected `area/` and `risk/` labels, human owner, milestone if applicable, and native dependency edges.
5. Link canonical `HLX-OQ-*` IDs, claim/source evidence, affected wiki sections, and acceptance criteria; add the exact Section 82 epic/gate, Section 83 risks, Section 84 card, and Section 85 freshness trigger when applicable.
6. If the answer requires measurement, allocate `HLX-EXP-*` before implementation; if it requires a significant choice, allocate `HLX-ADR-*`.

**Pass condition:** the issue is answerable or implementable without guessing authority, scope, evidence, or completion.

## 2. Coding-agent task packet

**[RECOMMENDATION]** A human owner or coordinating agent gives each coding agent a bounded packet:

```yaml
issue: "owner/repo#123"
human_owner: "REQUIRED"
objective: "one observable outcome"
write_scope: ["explicit/path/**"]
authorities: ["relative/wiki/path", "HLX-ADR-0000"]
evidence: ["source-id", "exact upstream commit"]
required_outputs: ["code", "tests", "docs", "validation receipt"]
prohibited: ["invent measurements", "change unrelated files", "push", "merge", "release"]
validation: ["exact command or experiment ID"]
handoff: ["changed paths", "results", "risks", "open questions"]
```

The agent must inspect `git status` before and after, preserve concurrent user/agent changes, and stop if authority conflicts. Agents may draft candidates and reviews; they do not self-approve, conceal provenance, or claim human authorization [S86-12, S86-13].

## 3. ADR and experiment gates

### ADR

1. Confirm the trigger in [design implications](design_implications.md#adr-boundary).
2. Create a proposed section-04 record with context, drivers, evidence, options, decision, consequences, rollback/fallback, authority, and reconsideration trigger.
3. Invite affected code, security, experiment, operations, and release roles.
4. Accept/reject through the named human authority; retain rejected options and review evidence.
5. Link implementation PRs and later validation. Supersede rather than edit accepted rationale silently.

### Experiment/performance claim

1. Create the issue and `HLX-EXP-*` protocol before running it.
2. Declare hypothesis, fixed keys, independent variable, metrics, repetitions, exclusions, failure oracle, and acceptance criteria.
3. Capture source/build/model/workload/environment/topology identities under section 05.
4. Preserve failures and immutable raw outputs; derive summaries only from finalized hashes.
5. A different reviewer checks comparison keys, exclusions, statistics, and applicability.
6. Use `[MEASURED]` only for reproducible local results with the run link. A regression gate requires an approved baseline and variance-aware threshold from sections 73/78.

## 4. Pull request and review

Open a draft PR early and link rather than close the issue while work is incomplete. The PR body must cover every contract field in [design implications](design_implications.md#pull-request-contract-and-definition-of-done).

Required review sequence:

1. deterministic diff/schema/generated-file/provenance checks;
2. affected path owners and explicit cross-lane reviewers;
3. experiment/security/license/release reviewers when triggered;
4. latest-push independent approval and resolved conversations;
5. authorized human merge after all required checks.

Configure rules to require PRs, no bypass for ordinary changes, required checks from expected apps, stale-review protection or latest-push approval, code-owner review, and conversation resolution [S86-17]. Protect governance files themselves. Do not enable a required check until its name and expected source are stable.

## 5. Secure CI on the Strix hosts

**[RECOMMENDATION]** Fast hosted/sandboxed checks may run on untrusted PRs with read-only token permissions and no secrets. The persistent Strix hosts accept only reviewed commits from trusted branches after explicit authorization; never run arbitrary public-fork code on them [S86-20].

For each hardware job:

- use an isolated disposable worktree/build/cache directory and minimal network/credential access;
- record runner image/state, commit, dirty state, toolchain, device, model/workload hashes, and command;
- serialize exclusive GPU use and clean declared state between jobs;
- upload sanitized logs and manifests; never expose model data, cache content, tokens, or private prompts;
- destroy disposable state, preserve only approved evidence, and report cleanup failures.

Third-party actions and reusable workflows must be reviewed and pinned to full commit SHAs. Workflows declare minimal `permissions:` and protect `.github/workflows/` with owners [S86-20].

## 6. Security intake and remediation

1. Publish supported versions and a private reporting route in `SECURITY.md`; enable GitHub private vulnerability reporting if the repository is public and the feature is selected [S86-19].
2. A restricted security owner acknowledges, deduplicates, assesses scope/severity, and records affected releases without moving details into a public issue.
3. Develop the fix and tests in the private advisory/fork path when available; rotate exposed secrets immediately.
4. Coordinate release, advisory, credit, CVE request where appropriate, and changelog wording.
5. After the fix is available, publish only the disclosure approved by the security authority and link a sanitized public follow-up.

## 7. Documentation and release

For every material merge, update affected reference/API docs, wiki claim labels and applicability, source/decision/experiment links, and the curated changelog. Changing prose to `[VERIFIED]` or `[MEASURED]` requires the same evidence review as code.

Release rehearsal:

1. freeze the candidate and exact patch/baseline manifest from sections 11/15;
2. audit milestone membership against exit criteria; open items require explicit disposition;
3. run required clean builds/tests on both nodes and record rank/fallback/cache behavior;
4. verify SBOM, notices, provenance, AI-log linkage, artifacts, checksums, migrations, and rollback;
5. generate notes, then human-edit them against merged PRs and the curated changelog;
6. create an immutable annotated/signed tag under the approved policy; publish only by human authorization;
7. perform install/upgrade/rollback smoke checks and record the supported tuple and known limitations.

## Required validation scopes and canonical aliases

Section 84 owns canonical experiment cards. The former `EXP-86-*` names below are local scope aliases only. Their candidate mappings identify the nearest card family; they do **not** claim that the current Section 84 card already contains the governance-specific protocol. Before external use or execution, amend and review the referenced card so its schema, controls, safety boundary, and acceptance rule explicitly include the scope.

| Local alias | Candidate canonical card/scope | Pass condition |
|---|---|---|
| `EXP-86-01` | `HLX-EXP-20260717-850`, independently reproduced repository-governance fixture subprotocol; card amendment required | Forms, label manifest, dependencies, CODEOWNERS, rules, PR checks, and release-note categories accept/reject fixtures as documented; no owner or required check is invented |
| `EXP-86-02` | `HLX-EXP-20260717-849`, CI trust-boundary subprotocol; card amendment required | Fork code cannot reach secrets/Strix hosts; trusted job is isolated, sanitized, and attributable |
| `EXP-86-03` | `HLX-EXP-20260717-850`, independent traceability/reproduction subprotocol; card amendment required | One change resolves requirement/evidence/issue/ADR-or-experiment/PR/tests/docs/AI-log links with no dangling ID |
| `EXP-86-04` | `HLX-EXP-20260717-849`, distinct release/rollback subprotocol; card amendment required | Exact tag, manifests, artifacts, notes, checksums, migration, rollback, cache invalidation, and fallback all resolve and pass |

## Deterministic closeout checks

From repository root, no elevation:

```powershell
git status --short
git diff --check
rg -n 'Closes |HLX-ADR-|HLX-EXP-|HLX-RUN-|S[0-9]+-' .github docs wiki experiments engineering
```

The future validator should emit JSON and fail on missing/dangling IDs, illegal ledger transitions, source-count mismatches, unpinned Actions, mutable dependency refs, changed material paths without issue/AI-log linkage, `[MEASURED]` without a run bundle, or release artifacts without checksums/provenance. Regex output is triage, not proof.
