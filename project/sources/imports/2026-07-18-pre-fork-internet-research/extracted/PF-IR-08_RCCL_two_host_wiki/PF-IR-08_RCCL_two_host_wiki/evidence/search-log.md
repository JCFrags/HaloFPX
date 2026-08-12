# Search log

Access date: **2026-07-18**

Audited repositories/sites: `ROCm/rocm-systems`, `ROCm/rccl`, official ROCm/RCCL documentation.

Search terms and code identifiers included:

`gfx1151`, `Strix Halo`, `USB4`, `Thunderbolt`, `NCCL_SOCKET_IFNAME`, `NCCL_SOCKET_FAMILY`, `ncclFindInterfaces`, `ncclNetSocketGetProperties`, `NCCL_PTR_HOST`, `regMrDmaBuf`, `NCCL_DMABUF_ENABLE`, `ncclCommRegister`, `ncclGetUniqueId`, `ncclCommInitRank`, `ncclCommGetAsyncError`, `ncclCommAbort`, `ncclCommShrink`, `ncclCommRevoke`, `ncclCommGrow`, `ncclTimeout`, `NetSocketTests`, MPI multi-node, remote error, reconnect.

Negative result: no audited upstream integration test or maintainer report exactly described **two Strix Halo hosts using Linux Ethernet-over-USB4 with stock RCCL Socket**. A merged tuning PR described 1–4 nodes over a 10 Gbps Ethernet switch. Community issues/PRs discussed Thunderbolt/USB4 and reported experiments, but they are not normative support.
