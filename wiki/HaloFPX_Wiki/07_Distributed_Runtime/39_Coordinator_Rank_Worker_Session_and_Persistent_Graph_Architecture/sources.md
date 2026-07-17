---
section_id: "39"
title: "Coordinator and Rank Sources"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ROCmFPX", "llama.cpp"]
  software_versions: ["HIP docs 7.2.53211"]
  hardware_revisions: []
related_sections: ["38", "45", "48"]
---

# Sources

| ID | Source | Claims supported | Limitations |
|---|---|---|---|
| S39-01 | [ROCmFPX commit `a5605a7`](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394), 2026-07-16; accessed 2026-07-16 | candidate base revision | no proposed protocol implementation established |
| S39-02 | [llama.cpp server developer guide](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/README-dev.md) and [`server-context.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/server-context.cpp), commit `788e07d`; accessed 2026-07-16 | slot, queue, batching, sampling lifecycle patterns | single server architecture; fast-moving |
| S39-03 | [llama.cpp RPC README](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/rpc/README.md), commit `788e07d`; accessed 2026-07-16 | remote device and security/status constraints | proof-of-concept, not HaloFPX protocol |
| S39-04 | [AMD HIP graphs](https://rocm.docs.amd.com/projects/HIP/en/latest/how-to/hip_runtime_api/hipgraph.html), HIP documentation `7.2.53211`; accessed 2026-07-16 | graph create/instantiate/launch/destroy lifecycle | no target-machine result |
| S39-05 | [Orca](https://www.usenix.org/conference/osdi22/presentation/yu), OSDI 2022 | iteration-level scheduling | architecture analogue, not code compatibility |
| S39-06 | Local Agent Harness `guide/architecture.md`, read 2026-07-16 | explicit authority and evidence lifecycle | governance only |
