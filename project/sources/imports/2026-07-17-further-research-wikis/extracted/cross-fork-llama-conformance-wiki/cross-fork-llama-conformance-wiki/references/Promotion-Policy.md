# Reference promotion policy

> [!IMPORTANT]
> A candidate run never updates its own oracle. Promotion is a separate, reviewed workflow.

## Promotion states

| State | Meaning | May gate? |
|---|---|---:|
| `UNCALIBRATED` | No approved threshold or baseline exists | No |
| `PROPOSED` | Evidence has been collected and a profile has been drafted | No |
| `APPROVED` | Independent validation and human review are complete | Yes |
| `RETIRED` | Kept for history; no longer used | No |

## Required provenance

Every promoted observation records the repository and commit, dirty-tree state, binary SHA-256, compiler and flags, model and fixture SHA-256 values, operating system, CPU architecture, backend, device, driver/runtime versions, all determinism controls, exact command, and raw artifact digests.

## Independence rules

Calibration runs select candidate metrics or envelopes. Validation runs are disjoint and are evaluated once against the proposed profile. Do not tune a profile after viewing validation failures without discarding that validation set and collecting a new one.

The upstream fork is normally the semantic reference for features shared with upstream. A fork-specific CPU reference may be used only for a feature absent upstream, such as a ROCmFPX tensor type or a persistent-cache format. Accelerator output is not its own reference.

## Numeric policy

`calibrate-tolerances.py` computes observed deltas and writes `PROPOSED` evidence. It deliberately leaves every normative threshold `null`. Reviewers must justify and enter approved limits in a scoped profile. Universal tolerances across models, quant types, backends, drivers, and architectures are prohibited.
