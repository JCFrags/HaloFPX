---
section_id: "04"
title: "Ledger Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["YAML 1.2.2", "JSON Schema 2020-12"]
  hardware_revisions: []
related_sections: ["01", "02", "03", "05"]
---

# Design implications

## Proposed record examples

```yaml
id: "HLX-ASM-0001"
type: "assumption"
title: "Matched nodes are operationally equivalent"
status: "active"
statement: "Both nodes can run the same pinned build and model artifact."
owner: "unassigned"
decision_authority: "unassigned"
confidence: "low"
evidence: []
impact: "Distributed placement and benchmark comparability"
due_condition: "Before accepting cross-node performance comparisons"
validation_method: "Inventory and matched single-node benchmark"
affected_sections: ["43", "49", "73"]
related_adrs: []
related_experiments: []
related_issues: []
created_at: "2026-07-16T00:00:00Z"
updated_at: "2026-07-16T00:00:00Z"
supersedes: []
superseded_by: null
history: []
```

This is an illustrative **[ASSUMPTION]**, not a populated project ledger entry.

## Type-specific fields

| Type | Additional required fields |
|---|---|
| assumption | `premise`, `failure_consequence`, `validation_method` |
| open question | `question`, `why_it_matters`, `answer_criteria`, `blocking` |
| research task | `prompt`, `deliverables`, `dependencies`, `completion_checks` |
| decision/ADR | `context`, `decision_drivers`, `options`, `decision`, `consequences`, `reconsideration_trigger` |

## Status enums

- Assumption: `proposed`, `active`, `validated`, `invalidated`, `superseded`.
- Open question: `open`, `investigating`, `answered`, `deferred`, `closed-no-longer-relevant`, `superseded`.
- Research task: `ready`, `in-progress`, `blocked`, `completed`, `cancelled`, `superseded`.
- ADR: `proposed`, `accepted`, `rejected`, `deprecated`, `superseded`.

**[RECOMMENDATION]** Validate legal transitions, not only final values. An accepted ADR can become deprecated/superseded but should not revert to proposed. A question answered by weak evidence can reopen through a new history event without deleting the prior answer.

## Storage and generated views

**[RECOMMENDATION]** Store one record per file under typed directories and generate indexes by status, owner, due condition, affected section, and blocking relationship. Keep ADR prose human-readable; place common metadata in YAML front matter validated by a versioned schema.

**[INFERENCE]** Typed records reduce false promotion: research output can close a question while the decision authority still evaluates alternatives.
