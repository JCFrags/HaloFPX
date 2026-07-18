# Literal claim labels

Every material conclusion in this bundle is prefixed with one of the following literal labels. The labels describe the provenance or support boundary of the claim, not its desirability.

| Label | Meaning |
|---|---|
| `[UPSTREAM]` | Present in a pinned upstream open-source project or mainline Linux source. |
| `[VENDOR-ONLY]` | Available only through AMD's supported distribution, package set, documentation, model asset, or compatibility path. |
| `[WINDOWS-ONLY]` | Documented for Windows but not supported on Linux in the captured source. |
| `[PREVIEW]` | Preview, planned, experimental, or not generally supported. |
| `[UNSUPPORTED]` | Explicitly outside the documented support boundary. |
| `[MISSING]` | Required evidence or component was not found or was not available on the target. |
| `[FIELD-REPORT]` | Anecdotal field evidence; none is used as a decision foundation here. |
| `[INFERENCE]` | A reasoned conclusion derived from cited primary evidence rather than a literal source statement. |
| `[UNKNOWN]` | Evidence is insufficient to classify the behavior or target state. |
| `[TARGET-DISTRO]` | A statement that must be verified on the actual target Linux installation. |
| `[DECISION]` | A dated project decision or gate. |

## Reading rule

A claim is not upgraded from `[UNKNOWN]` to `[UPSTREAM]` merely because the hardware enumerates. Kernel recognition, firmware availability, userspace compatibility, graph compilation, operator placement, correctness, latency, energy, suspend/recovery behavior, and packaging reproducibility are separate gates.

The authoritative machine-readable claim list is [`manifests/claims.csv`](manifests/claims.csv).
