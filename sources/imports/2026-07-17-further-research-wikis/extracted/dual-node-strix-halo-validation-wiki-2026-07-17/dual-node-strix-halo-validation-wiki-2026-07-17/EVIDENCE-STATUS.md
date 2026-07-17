# Evidence Status

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Capability | Status | Evidence class | Release implication |
|---|---|---|---|
| Program structure and navigation | Complete | D0 | No machine claim |
| Experiment definitions | Complete | D0 | Ready to execute |
| Raw-data schemas | Complete and locally parseable | D0/S0 | No hardware claim |
| Gate evaluator | Implemented and synthetic-tested | S0 | Tool behavior only |
| Upstream watch configuration | Implemented; network execution not performed here | D0/S0 | Must be deployed and observed |
| Node A matched baseline | Not run | None | Blocks integration exit |
| Node B matched baseline | Not run | None | Blocks integration exit |
| USB4 link characterization | Not run | None | Blocks dual-node performance claims |
| Dual-node correctness | Not run | None | Blocks release |
| Fault injection and recovery | Not run | None | Blocks release candidate |
| 72-hour stable soak | Not run | None | Blocks stable release |

## Authorized wording

- Allowed now: **“Validation program design complete; machine evidence not yet collected.”**
- Not allowed now: “validated,” “stable,” “benchmark passed,” “USB4 proven,” or “production ready.”
- A synthetic fixture may be described only as a parser or gate-logic test.
