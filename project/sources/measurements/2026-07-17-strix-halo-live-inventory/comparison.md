# Matched-pair comparison and implications

## Confirmed matched properties

- Same Nimo MME3L product and NIMO Mini PC board naming/version.
- Same BIOS release/date, CPU model/stepping/microcode, memory class, GPU PCI identity/revision/CU count, current CachyOS kernel, ROCm 7.2.4 family, Mesa 26.1.4, firmware package date, NVMe model/firmware, and two USB4 host-router functions.
- Same 32-core/one-NUMA topology and approximately 124 GiB usable host memory.
- Same two private USB4 subnets, MTU 9000, MPTCP two-subflow policy, and two-lane 20.0 Gb/s-per-lane negotiated reports.

## Material asymmetries

| Area | nimo-1 | nimo-2 | Project consequence |
|---|---|---|---|
| Role | private RPC worker | coordinator / LAN API | Rank and failure semantics must retain this ownership explicitly. |
| Storage free | about 43 GiB | about 318 GiB | nimo-1 cannot safely host a growing HaloKV tier without cleanup, quota, reserve, or additional storage. |
| RPC tensor cache | about 112 GiB / 187 files | absent | Existing cache placement is strongly asymmetric and must not be confused with HaloKV. |
| Swap | priority `-1`, negligible use | priority `100`, about 456 MiB used | Normalize or deliberately document before matched memory/OOM tests. |
| Boot options | no explicit zswap flag captured | `zswap.enabled=0` | Boot-policy parity is incomplete. |
| Package subset | `hipcub` and `rocm-hip-sdk` present | not seen in matched package subset | Reproducible builds require an explicit package lock rather than “ROCm 7.2.4” alone. |
| LTS package | 6.18.38 installed | 6.18.37 installed | Not active, but rollback lanes are not identical. |
| USB4 netdev-to-domain mapping | tb0→domain0, tb1→domain1 | tb0→domain1, tb1→domain0 | Never infer physical path identity from `thunderboltN` alone; bind by address plus sysfs ancestry. |
| SSD SMART | 1% used, 14 unsafe shutdowns | 0% used, 17 unsafe shutdowns | Drives look healthy, but durability testing must retain per-node baselines. |

## Live distributed baseline

- The running stack is not the requested future ROCmFPX integration fork. It is `charlie12345/rocmfp4-llama@4860505e...`.
- nimo-2 owns the model and LAN-facing API; nimo-1 owns the private RPC device endpoint.
- Explicit `RPC0,ROCm0` ordering and `--tensor-split 1,1` are present in the running command.
- The current model is approximately 121.86 GB on disk and its server reports about 228.7B parameters. This proves a large distributed baseline can be loaded, not that a 200–230 GB stored model will fit or perform well.
- The MPTCP connection had two active subflows and cumulative traffic on both rails. Additive bandwidth and failure independence remain unproven.

## Immediate gates for the project goal

1. Resolve nimo-1 storage headroom before enabling any additional persistent cache or staging a 200–230 GB artifact.
2. Treat the deployed RPC cache as a model-weight transfer cache with a separate compatibility and security boundary.
3. Add cryptographic content verification, atomic publication, bounded size/eviction, and corruption-as-miss behavior before retaining the RPC file-cache design.
4. Pin the complete build/package/boot tuple on both nodes; current package and swap-policy skew would invalidate matched comparisons.
5. Keep TCP/MPTCP over `thunderbolt-net` as the working baseline. The active 7.1.3 kernel does not expose USB4STREAM.
6. Measure single-rail, dual-rail, simultaneous, and GPU-to-peer-GPU behavior only in a controlled window; the current five-packet RTT sample is diagnostic evidence only.
7. Review world-writable `/dev/kfd` and render-node permissions before treating the cluster as multi-user or exposing broader service access.

