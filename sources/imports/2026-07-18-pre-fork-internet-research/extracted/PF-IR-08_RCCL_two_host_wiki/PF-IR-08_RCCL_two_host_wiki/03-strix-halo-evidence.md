# Strix Halo evidence and support boundary

## Evidence ledger

| Evidence | Literal class | What it establishes | What it does not establish |
|---|---|---|---|
| ROCm 7.2 Ryzen matrix | `[NORMATIVE_DOC]` | gfx1151/Ryzen AI Max+ 395 platform and PyTorch support. | RCCL component support, multi-node collectives, or USB4 networking. |
| PR #3415 | `[MAINTAINER_STATEMENT]` | gfx1151 source enablement merged; single-GPU unit tests and PyTorch load reported. | Multi-GPU or multi-node suitability; the PR says those were pending. |
| PR #4875 | `[MAINTAINER_REPORTED_MACHINE_EVIDENCE]` | 1–4 Strix Halo nodes over a 10 Gbps Ethernet switch reportedly ran RCCL without failure. | USB4, exact target hosts, stock-vs-custom transport path, or independently reproduced logs. |
| Issue #2026 | `[MAINTAINER_STATEMENT]` | Historical “no plans” reply, later redirected to monorepo tracking. | Current prohibition; it was superseded by later work. |
| Issue #2788 | `[ISSUE_COMMENTARY]` | Community interest in a Thunderbolt/USB4 plugin and gfx1151 enablement. | AMD validation of that plugin or topology. |
| PR #2075 | `[COMMUNITY_REPORTED_MACHINE_EVIDENCE]` | Unmerged community report of two Strix Halo nodes, rccl-tests, and vLLM TP. | Normative support, reproducibility, or proof of a DMA-BUF/GPU-direct Socket path. |

## Interpretive rule

Architecture enablement, framework support, and a successful load are three different facts. None substitutes for the two-host test. The strongest relevant upstream machine report uses an Ethernet switch; the exact Ethernet-over-USB4 topology remains unvalidated in this corpus.
