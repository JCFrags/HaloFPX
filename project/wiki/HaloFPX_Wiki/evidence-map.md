# HaloFPX Evidence Map

Status: authoritative routing index

Last verified: 2026-07-29

This page locates evidence.
The linked artifact remains authoritative for its own contents.

## Evidence precedence

1. Exact source code and commit.
2. Machine evidence and immutable receipts.
3. Accepted decisions.
4. Official documentation or standards.
5. Research papers.
6. Labeled secondary evidence.

## Evidence locations

| Evidence class | Canonical location | Use |
|---|---|---|
| Imported source and archives | `sources/` | Preserve provenance, hashes, and licenses |
| Repository source pins | [`sources/repositories/`](../../sources/repositories/README.md) | Inspect exact upstream and donor source |
| Machine experiments | `experiments/` | Retain environment data and raw results |
| Research assignments | [`research/prompts/`](../../research/prompts/README.md) | Preserve the research question and routing |
| Reviewed Wiki claims | [Wiki categories](README.md#categories) | Explain source-backed findings |
| Project reviews | [`reviews/`](../../reviews/README.md) | Retain findings and improvement proposals |
| Project Lead authority | [`project-management/lead/`](../../project-management/lead/README.md) | Record current state and accepted boundaries |
| HaloFPX implementation evidence | [`docs/halofpx/`](../../../docs/halofpx/README.md) | Index implementation-local milestones and receipts |

## Claim labels

| Label | Required meaning |
|---|---|
| `[MEASURED]` | The result applies to the stated environment. |
| `[VERIFIED]` | Primary evidence supports the claim. |
| `[INFERENCE]` | Evidence supports a conclusion that is not a direct fact. |
| `[ASSUMPTION]` | Work currently depends on an unverified condition. |
| `[RECOMMENDATION]` | The text proposes an action or design. |
| `[OPEN]` | The issue remains unresolved. |

## Performance evidence

Keep prompt performance separate from generation performance.
Keep synthetic results separate from full-model results.
State the feature-on and feature-off conditions.
Name the host, model, commit, binary, and runtime tuple.
Do not claim an improvement without a matched accepted comparison.

## Distributed evidence

State coordinator and worker ownership.
State rank ownership for model and cache state.
State retry, failure, recovery, and single-node fallback behavior.
Link the applicable accepted decision.

## Cache evidence

Authenticate all accepted state.
Treat corrupt, incomplete, incompatible, or unauthorized state as a miss.
Recompute after a miss.
Never accept invalid cache state.
