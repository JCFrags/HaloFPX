---
section_id: "82"
title: "Roadmap Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "CachyLLama@6be745998f568e379ea197fcf827baec73ff9940", "llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"]
  software_versions: ["roadmap baseline 2026-07-17"]
  hardware_revisions: ["dual Strix Halo premise; exact BOM unresolved"]
related_sections: ["09", "11", "15", "16", "18", "20", "23", "29", "38", "48", "49", "50", "51", "52", "53", "56", "57", "58", "59", "60", "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "80", "81"]
---

# Roadmap facts and constraints

## Evidence baseline

| ID | Claim | Roadmap consequence |
|---|---|---|
| F82-01 | **[VERIFIED]** The source snapshot pins llama.cpp `788e07d...`, ROCmFPX `a5605a7...`, CachyLLama `6be7459...`, and llama-ai `1017f3d...` [S82-01, S82-02]. | Every build, patch, test, and artifact begins from full object IDs; moving branch names are not evidence. |
| F82-02 | **[VERIFIED]** ROCmFPX has unrelated Git history to the pinned llama.cpp baseline, while CachyLLama retains upstream ancestry; the documented ROCmFPX `b9438` association conflicts with the official tag object [S82-01]. | Normalize ROCmFPX into provenance-recorded patch lanes before long-lived integration; do not use a blind unrelated-history merge. |
| F82-03 | **[VERIFIED]** Current source trees expose build/test surfaces, but workflow presence is not proof of target gfx1151 correctness, reproducibility, or release readiness [S82-02, S82-03]. | G82-00 and G82-20 require target-machine artifacts, not static inspection alone. |
| F82-04 | **[VERIFIED]** CachyLLama contains persistent-cache mechanisms, but the wiki has not produced or fault-tested a checkpoint on the target machines [S82-04]. | Cache porting starts from semantics and a new compatibility/durability contract, not reuse of an unqualified format. |
| F82-05 | **[VERIFIED]** llama.cpp’s documented server and tests provide useful API, backend-op, state, grammar, server, perplexity, and benchmark baselines at the pinned commit [S82-03]. | Reuse upstream tests as comparators, then add HaloFPX distributed/cache/fault cases. |
| F82-06 | **[VERIFIED]** SLSA provenance records where, when, and how artifacts were produced; SPDX defines machine-readable software metadata [S82-07, S82-08]. | Each candidate emits provenance, dependency/license inventory, and digests; these controls do not by themselves prove correctness. |
| F82-07 | **[VERIFIED]** NIST SSDF recommends defined development environments, provenance/integrity protection, review, testing, and vulnerability response [S82-09]. | Security and release work begins in P0 and is reviewed at every gate, not appended only at P7. |

## Project constraints inherited from completed sections

| Constraint | Authority | Consequence |
|---|---|---|
| Candidate SLOs are not sponsor-approved and no project measurements exist. | Sections [09](../../02_Project_Definition/09_Functional_Requirements_SLOs_and_Acceptance_Criteria/README.md), [73](../../11_Verification_and_Performance/73_Benchmark_Methodology_Terminology_Experimental_Controls_and_Data_Schema/README.md) | Use relative-to-baseline prototype gates until targets are ratified; never invent dates or throughput. |
| Replication is the provisional initial distributed baseline; a coupled mode must prove capacity or p99 value. | Section [38](../../07_Distributed_Runtime/38_Distributed_Runtime_Goals_Cost_Model_and_Mode_Selection/README.md) | E82-30 precedes and remains rollback for E82-50..54. |
| Exact BOM, firmware symmetry, dual-port independence, memory limits, storage and thermal envelope are open. | Sections [18](../../04_Hardware_and_OS_Platform/18_Exact_Machine_BOM_BIOS_Firmware_Cabling_and_Revisions/README.md), [20](../../04_Hardware_and_OS_Platform/20_USB4_Physical_Topology_and_Dual_Port_Independence/README.md) | G82-10 blocks comparative distributed claims; single-node exploratory work may continue with manifest labels. |
| Fabric completion must distinguish local buffer reuse, remote receipt, and remote application; transport remains behind an abstraction. | Section [49](../../08_Fabric_and_Transport/49_Fabric_Requirements_and_Transport_Abstraction/README.md) | Protocol conformance precedes optimization; optional kernel work cannot leak into model/cache semantics. |
| Coordinator/rank ownership, epochs, ordered operations, output visibility, and fail-closed mismatch behavior are correctness properties. | Section [48](../../07_Distributed_Runtime/48_Distributed_Correctness_Determinism_Fault_Recovery_and_Degraded_Mode/README.md) | Every coupled prototype includes mismatch, timeout, cancellation, replay, and fallback tests. |
| Invalid cache state must be rejected as a miss/recomputation; committed generations are immutable. | Sections [56](../../09_HaloKV_Persistent_Cache/56_CachyLLama_Cache_Semantics_and_Porting_Map/README.md), [63](../../09_HaloKV_Persistent_Cache/63_Durability_Modes_Atomic_Commit_Crash_Recovery_and_Corruption_Handling/README.md) | G82-60 is correctness/durability first; performance cannot waive corruption handling. |
| API compatibility is an explicitly tested subset and configuration is schema-validated and explainable. | Sections [66](../../10_Product_Server_and_Operations/66_OpenAI_Compatible_API_Server_Semantics_and_Error_Model/README.md), [67](../../10_Product_Server_and_Operations/67_Configuration_Hardware_Profiles_Model_Manifests_and_Plan_Manifests/README.md) | MUPs publish precise supported surfaces, manifests, and stable error behavior. |

## Hardware gates

| Gate | Evidence required | Blocks |
|---|---|---|
| HG82-A Identity | same-window BOM/firmware/cable/port inventory; discrepancies classified | matched comparisons |
| HG82-B Software | exact kernel, firmware, amdgpu, ROCm, Mesa, compiler and package manifest on both nodes | backend qualification and release |
| HG82-C Capacity | allocation staircase and admitted model/context/concurrency memory headroom | MUP-1 and fallback claims |
| HG82-D Fabric | single-link/dual-link curves, independence, failure isolation, identity stability | coupled-mode selection and dual-link claims |
| HG82-E Storage | NVMe/filesystem/SMART baseline, sustained writes, crash behavior | persistent HaloKV |
| HG82-F Sustained | thermal/power/clock behavior under matched inference+fabric+NVMe load | performance and soak release claims |

## Current evidence and decision gaps

**[VERIFIED]** All 86 registered sections now contain `section.yaml`. Sections 50-53 define TCP/`thunderbolt-net` as the bring-up baseline, audit current RPC as useful but not a production transport, propose staged multipath, and specify a candidate framed protocol. They still require carrier capability, integrity, flow-control, authentication, dual-rail, fault, and performance experiments before a production fabric is selected.

**[VERIFIED]** Sections 57-60 propose a canonical compatibility manifest, rank-local restore, immutable page/segment/index structures, and policy-bound prefix sharing. Their schema, state ownership, page/segment sizes, metadata engine, suffix equivalence, isolation, crash behavior, and restore value remain unvalidated on the two hosts.

**[VERIFIED]** Sections 70-72 recommend native systemd first, least-privilege authenticated deployment, and digest-addressed reversible releases. The supported OS/runtime tuple, device permissions, exposure/identity model, protocol/cache compatibility window, migration, backup, RPO/RTO, and rollback authority remain decisions backed by unrun machine exercises.

**[VERIFIED]** Sections 74 and 76-81 now define single-node, distributed, cache, stress, fault, and release evidence programs. They explicitly record zero HaloFPX measurements or evaluated release candidates; runner capacity, statistical thresholds, quality gates, soak/fault budgets, reproducibility, retention, and cutover proof remain unresolved.

**[RECOMMENDATION]** Treat these completed pages as the authoritative experiment and design inputs to their roadmap gates. The blocker is no longer missing research pages; it is acceptance of their candidate contracts and execution/review of their machine evidence.

## No measurements

**[VERIFIED]** A 2026-07-17 artifact audit found no HaloFPX measurement in Section 82: [`section.yaml`](section.yaml) declares `measurements_present: false`, every M82 aggregate remains required, and the linked verification inputs report unrun target-machine work [S82-05]. All thresholds and phase transitions are proposed governance controls pending the named experiments.
