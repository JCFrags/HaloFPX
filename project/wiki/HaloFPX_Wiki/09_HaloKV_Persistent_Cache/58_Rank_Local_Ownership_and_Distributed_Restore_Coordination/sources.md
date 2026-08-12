---
section_id: "58"
title: "Rank-local restore primary sources"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "fewtarius/CachyLLama", "ggml-org/llama.cpp"]
  software_versions: ["ROCmFPX a5605a72768c6562241b248e268e33dc92787394", "CachyLLama 6be745998f568e379ea197fcf827baec73ff9940"]
  hardware_revisions: ["two gfx1151 Strix Halo hosts; exact topology pending"]
related_sections: ["49", "52", "54", "57", "61", "63", "75"]
---

# Rank-local restore primary sources

Accessed 2026-07-16; code links are immutable full commits.

| ID | Source | Supports | Limitations |
|---|---|---|---|
| S58-01 | ROCmFPX [`common/arg.cpp`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/common/arg.cpp), commit `a5605a72` | Split modes, tensor proportions, RPC device CLI | CLI semantics do not specify durable state ownership. |
| S58-02 | ROCmFPX [`tools/rpc/README.md`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/rpc/README.md) and [`rpc-server.cpp`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/rpc/rpc-server.cpp) | Remote ggml device model and TCP topology | RPC offload is not a HaloKV coordination protocol. |
| S58-03 | ROCmFPX [`src/llama-context.cpp`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/src/llama-context.cpp) and [`include/llama.h`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/include/llama.h) | Sequence state APIs and tensor-split context path | Runtime format/ownership is not a stable distributed schema. |
| S58-04 | CachyLLama [`server-context-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-ssd-cache.cpp), commit `6be74599` | Target/draft sequence serialization/restore | Single server context integration; no two-rank manifest. |
| S58-05 | CachyLLama [`common/kv-ssd-cache.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.h) | Checkpoint/component metadata | Weak compatibility/identity; not rank-local. |
| S58-06 | MPI Forum, [MPI 4.1 standard](https://www.mpi-forum.org/docs/mpi-4.1/mpi41-report.pdf), Nov 2023 | Collective process/communication semantics reference | HaloFPX need not use MPI; standard does not define cache restore. |
| S58-07 | NIST, [FIPS 180-4 Secure Hash Standard](https://csrc.nist.gov/pubs/fips/180-4/upd1/final), Aug 2015 | Collision-resistant digest family for manifests | Section 57 chooses exact algorithm/encoding. |
| S58-08 | ggml-org/llama.cpp [commit `788e07dc`](https://github.com/ggml-org/llama.cpp/commit/788e07dc91d266ad3162a1ce9037665656269689) | Current upstream comparison point | Does not prove HaloFPX distributed restore. |

## Evidence gap

No primary source currently specifies the proposed HaloFPX two-rank ownership/restore protocol. Protocol statements are explicitly recommendations awaiting implementation and fault-injection evidence.

