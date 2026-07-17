---
section_id: "82"
title: "Implementation Roadmap Sources"
status: "draft"
last_verified: "2026-07-17"
applies_to:
  repositories: ["llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "CachyLLama@6be745998f568e379ea197fcf827baec73ff9940", "llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"]
  software_versions: ["SLSA 1.2", "SPDX 3.0.1", "NIST SP 800-218 v1.1"]
  hardware_revisions: []
related_sections: ["11", "15", "16", "38", "48", "49", "50", "51", "52", "53", "56", "57", "58", "59", "60", "66", "70", "71", "72", "73", "74", "76", "77", "78", "79", "80", "81"]
---

# Implementation roadmap sources

Internet sources were accessed 2026-07-17. Independent `git ls-remote` refreshes on that date found the four named branch tips still at the pinned objects recorded here. Repository commit/blob links are immutable; dynamic project state must still be refreshed before changing the baseline. Local wiki pages are scoped synthesis, not substitutes for their cited primary evidence.

### S82-01 - Repository lineage and integration authorities

- Publisher/path: HaloFPX Wiki Sections [11](../../03_Repository_and_Engineering/11_Repository_Lineage_Branches_Commits_and_Frozen_Baselines/README.md) and [15](../../03_Repository_and_Engineering/15_Integration_Patch_Stack_and_Upstream_Synchronization_Strategy/README.md)
- Revision/date: local snapshot last verified 2026-07-16.
- Supports: exact pins, unrelated ROCmFPX history, `b9438` contradiction, six-lane integration/rollback model.
- Limitation: planned integration has not been built or machine-validated.

### S82-02 - Build, dependency, license, CI and agent workflow authority

- Publisher/path: HaloFPX Wiki Section [16](../../03_Repository_and_Engineering/16_Build_Dependencies_Licensing_CI_and_AI_Agent_Workflow/README.md)
- Revision/date: local snapshot 2026-07-16.
- Supports: build entry points, reproducibility/provenance controls, license boundaries, target-machine gates.
- Limitation: no target build or legal review is claimed.

### S82-03 - Pinned llama.cpp tree and test surfaces

- Publisher/repository: `ggml-org/llama.cpp`
- URLs: [tree](https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689), [backend tests](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tests/test-backend-ops.cpp), [server tests](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/tests/README.md), [llama-bench](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/llama-bench/README.md)
- Revision/date: `788e07dc91d266ad3162a1ce9037665656269689`, committed 2026-07-17T06:42:59Z.
- Supports: upstream source/test comparator and benchmark surface.
- Limitation: test presence is not HaloFPX success; microbenchmarks exclude client/server work.

### S82-04 - ROCmFPX and CachyLLama donors

- Publishers/revisions: [ROCmFPX `a5605a72768c6562241b248e268e33dc92787394`](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394), [CachyLLama `6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940), [llama-ai `1017f3dfdce3ca2b06aa9007b23295db3bb35722`](https://github.com/fewtarius/llama-ai/tree/1017f3dfdce3ca2b06aa9007b23295db3bb35722)
- Revision/date: commits dated 2026-07-17 UTC for ROCmFPX and 2026-07-09 UTC for the two fewtarius repositories.
- Supports: donor identities and source features feeding epics.
- Limitation: repository claims/benchmarks are not imported as measurements; only selected semantics are candidates.

### S82-05 - Architecture, transport, cache, product and verification dependency authorities

- Publisher/paths: Sections [38](../../07_Distributed_Runtime/38_Distributed_Runtime_Goals_Cost_Model_and_Mode_Selection/README.md), [48](../../07_Distributed_Runtime/48_Distributed_Correctness_Determinism_Fault_Recovery_and_Degraded_Mode/README.md); transport [49](../../08_Fabric_and_Transport/49_Fabric_Requirements_and_Transport_Abstraction/README.md), [50](../../08_Fabric_and_Transport/50_USB4STREAM_and_thunderbolt_net_Implementation_Options/README.md), [51](../../08_Fabric_and_Transport/51_Existing_ggml_RPC_and_ROCmFPX_RDMA_Transport_Audit/README.md), [52](../../08_Fabric_and_Transport/52_Dual_Link_Multipath_Striping_Alternation_Hedging_and_Failover/README.md), [53](../../08_Fabric_and_Transport/53_Message_Framing_Credits_Flow_Control_Integrity_and_Security/README.md); cache [56](../../09_HaloKV_Persistent_Cache/56_CachyLLama_Cache_Semantics_and_Porting_Map/README.md), [57](../../09_HaloKV_Persistent_Cache/57_Compatibility_Fingerprints_Versioning_and_Topology_Identity/README.md), [58](../../09_HaloKV_Persistent_Cache/58_Rank_Local_Ownership_and_Distributed_Restore_Coordination/README.md), [59](../../09_HaloKV_Persistent_Cache/59_Immutable_Pages_Segment_Files_Indexes_and_Prefix_DAG/README.md), [60](../../09_HaloKV_Persistent_Cache/60_System_Prompt_Sharing_Deduplication_Copy_on_Write_and_Continuations/README.md); product [66](../../10_Product_Server_and_Operations/66_OpenAI_Compatible_API_Server_Semantics_and_Error_Model/README.md), [70](../../10_Product_Server_and_Operations/70_Packaging_systemd_Containers_Deployment_and_Cold_Boot_Procedure/README.md), [71](../../10_Product_Server_and_Operations/71_Security_Trust_Boundaries_Permissions_Local_Network_and_Secrets/README.md), [72](../../10_Product_Server_and_Operations/72_Upgrades_Rollbacks_Protocol_and_Cache_Migration_Backup_and_Runbooks/README.md); verification [73](../../11_Verification_and_Performance/73_Benchmark_Methodology_Terminology_Experimental_Controls_and_Data_Schema/README.md), [74](../../11_Verification_and_Performance/74_Single_Node_HIP_and_Vulkan_Baseline_Matrix/README.md), [76](../../11_Verification_and_Performance/76_Distributed_Mode_Benchmark_Matrix_and_Break_Even_Analysis/README.md), [77](../../11_Verification_and_Performance/77_HaloKV_Restore_Writeback_Hit_Rate_and_Endurance_Benchmarks/README.md), [79](../../11_Verification_and_Performance/79_Stress_Soak_Long_Context_Multi_Session_Power_and_Thermal_Testing/README.md), [80](../../11_Verification_and_Performance/80_Fault_Injection_Cable_Pulls_Restarts_OOM_Disk_Full_and_Corruption/README.md), and [81](../../11_Verification_and_Performance/81_CI_Matrix_Release_Gates_Reproducibility_and_Performance_Regression_Policy/README.md).
- Revision/date: local pages last verified 2026-07-16 or 2026-07-17; completeness re-audited 2026-07-17.
- Supports: replication baseline, ownership/failure rules, candidate transport/cache/deployment contracts, and complete verification plans.
- Limitation: the pages consistently report unrun target-machine work and no HaloFPX performance measurements; candidate contracts still need ADR acceptance and implementation validation.

### S82-06 - Git bundle documentation

- Publisher: Git project.
- URLs: [git-bundle 2.54.0](https://git-scm.com/docs/git-bundle/2.54.0), [git-fsck 2.54.0](https://git-scm.com/docs/git-fsck/2.54.0), [git-range-diff 2.54.0](https://git-scm.com/docs/git-range-diff/2.54.0).
- Revision/date: Git 2.54.0 documentation; accessed 2026-07-17.
- Supports: offline bundle verification, object integrity and patch-series comparison.
- Limitation: Git integrity does not include external dependencies, model artifacts, working-tree changes, correctness or provenance truth.

### S82-07 - SLSA provenance

- Publisher: OpenSSF/SLSA.
- URL: <https://slsa.dev/spec/v1.2/provenance>
- Revision/date: specification 1.2; accessed 2026-07-17.
- Supports: verifiable source/build/artifact provenance fields.
- Limitation: using the format does not claim a SLSA build level or trustworthy builder.

### S82-08 - SPDX Specification

- Publisher: SPDX / Linux Foundation.
- URL: <https://spdx.github.io/spdx-spec/v3.0.1/>
- Revision/date: 3.0.1; accessed 2026-07-17.
- Supports: machine-readable software, build, integrity and license metadata.
- Limitation: an SBOM is not a legal opinion or vulnerability disposition.

### S82-09 - Secure Software Development Framework

- Publisher: NIST.
- URL: <https://csrc.nist.gov/pubs/sp/800/218/final>
- Revision/date: NIST SP 800-218 v1.1, February 2022; accessed 2026-07-17.
- Supports: preparing development environments, protecting software, producing well-secured releases, and responding to vulnerabilities.
- Limitation: practices require project tailoring; this section does not claim compliance.

### S82-10 - Agent Harness governance authority

- Publisher/path: local canonical Agent Harness, `C:\Users\britt\Documents\Agent_Harness\AGENTS.md` and `guide\architecture.md`, routed by `../../../../references/agent-harness.md`.
- Revision/access date: inspected 2026-07-17.
- Supports: evidence promotion, reversible candidates, review, scoped memory, and closeout improvement.
- Limitation: conceptual harness guidance; root HaloFPX rules take precedence.

## Source conflicts and freshness

- **[VERIFIED]** ROCmFPX documents a llama.cpp lineage that its Git graph does not encode, and its `b9438` association conflicts with the official tag target. Preserve this conflict through E82-01 [S82-01].
- **[OPEN]** Repository heads, ROCm/kernel/Mesa documentation, security advisories, and hardware firmware are volatile. Refresh them under I82-01..05 without overwriting the frozen baseline.
