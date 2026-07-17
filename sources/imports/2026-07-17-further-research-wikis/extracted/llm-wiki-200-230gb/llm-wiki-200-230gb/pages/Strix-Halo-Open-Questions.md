# Strix Halo: Open Questions, Not Predictions

## What is measured

The pinned ROCmFPX README contains local Strix Halo (`gfx1151`) results for smaller Qwen 27B and 35B models, including ROCmFP4 versus Q4_K_M comparisons and MTP tests. Those data demonstrate that the backend and quant kernels can run on the tested system.

## What is not measured here

No direct measurement was located for any selected 200–230 GB artifact on Strix Halo. Therefore this wiki does **not** predict:

- prompt-fill or decode tokens/second;
- two-node RPC scaling;
- effective memory bandwidth under expert routing;
- Vulkan versus HIP/ROCm winner for target models;
- thermal steady state or power-limited performance;
- 128K/native-context throughput;
- MTP acceptance rate or speedup for Step at this size.

## Capacity facts

- One 128 GiB-class node cannot hold a selected artifact plus runtime overhead.
- Two nodes have sufficient aggregate memory for several candidates under the package's equal-split planning envelope.
- Aggregate capacity is not equivalent to a supported or efficient distributed deployment.

## Minimum measurement matrix

For each target candidate and quant, record:

| Variable | Required values |
|---|---|
| Backend | Vulkan, HIP/ROCm |
| Topology | single node partial offload, two-node RPC if used |
| Context | 4K, 32K, 64K, 128K where supported |
| Workload | pp512, pp4096, tg128, tg512, production prompts |
| Cache | F16, Q8_0, optionally Q4_0 |
| Batch | documented `-b` and `-ub` values |
| Thermals | cold start and steady-state |
| Network | link type, measured one-way bandwidth/latency, isolation/security |

Publish command lines, runtime commit, driver versions, model shard hashes, and raw logs. Until those exist, performance remains an open empirical question.
