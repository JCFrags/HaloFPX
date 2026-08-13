---
section_id: "83"
title: "Risk Register Sources"
status: "verified"
last_verified: "2026-08-12"
applies_to:
  repositories:
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
  software_versions: ["source snapshot 2026-07-17"]
  hardware_revisions: ["dual Strix Halo premise; exact BOM open"]
related_sections: ["02", "11", "13", "20", "21", "22", "23", "31", "37", "48", "63", "65", "71"]
---

# Sources

Internet sources were accessed 2026-07-17 America/Los_Angeles. Local wiki pages are derived project evidence: their claim labels, scopes, and source limitations remain binding. They are not promoted to primary authority merely by reuse here.

## Project evidence and completed technical research

| ID | Source, revision, and location | Supports | Limitations |
|---|---|---|---|
| S83-01 | HaloFPX Section 11, `../../03_Repository_and_Engineering/11_Repository_Lineage_Branches_Commits_and_Frozen_Baselines/`, verified 2026-07-16; independent `git ls-remote --symref` refresh 2026-07-17 | Exact heads, lineage, CachyLLama divergence, ROCmFPX independent root | Moving remote snapshot; not a build baseline by itself |
| S83-02 | HaloFPX Section 13, `../../03_Repository_and_Engineering/13_ROCmFPX_Feature_Kernel_Format_and_Patch_Inventory/`, ROCmFPX `a5605a7`, llama.cpp `788e07d` | Custom formats/type IDs, HIP/Vulkan paths, MTP/cache overlaps, patch conflict heat | Static/source evidence; no target build or numerical/performance result |
| S83-03 | HaloFPX Section 20, `../../04_Hardware_and_OS_Platform/20_USB4_Physical_Topology_and_Dual_Port_Independence/`, historical receipts dated 2026-07-10/12 | Two observed domains/rails and limits of independence evidence | Historical environment-specific measurement; no paired causal proof |
| S83-04 | HaloFPX Section 21, `../../04_Hardware_and_OS_Platform/21_NVMe_and_Storage_Topology_Performance_and_Endurance/`, verified 2026-07-17 with live source S21-L01 | Installed NVMe model/firmware, Btrfs state, capacity/headroom, baseline SMART counters, and the remaining endurance/durability evidence contract | Environment-specific inventory; no negotiated PCIe/queue qualification, workload write amplification, vendor TBW/PLP proof, or power-loss recovery result |
| S83-05 | HaloFPX Section 22, `../../04_Hardware_and_OS_Platform/22_Power_Thermals_Cooling_and_Sustained_Clocks/`, verified 2026-07-16 | Thermal/power unknowns and sustained-test requirement | No promoted sustained machine result |
| S83-06 | HaloFPX Section 23, `../../04_Hardware_and_OS_Platform/23_Linux_Kernel_amdgpu_Firmware_ROCm_and_Mesa_Compatibility_Matrix/`, verified 2026-07-16 | gfx1151 tuple, KFD fixes, ROCm/Mesa/USB4STREAM boundaries | Compatibility docs do not qualify ROCmFPX on target nodes |
| S83-07 | HaloFPX Section 31, `../../06_Models_Quantization_and_Inference/31_Conversion_Imatrix_Calibration_and_Quality_Validation/`, verified 2026-07-16 | Conversion/imatrix/quantization provenance and quality gate | No HaloFPX target-model quality result |
| S83-08 | HaloFPX Section 37, `../../06_Models_Quantization_and_Inference/37_gfx1151_HIP_and_Vulkan_Kernel_Optimization_Opportunities/`, verified 2026-07-16 | Backend code surface and hotspot/performance uncertainty | No HaloFPX hotspot or performance measurement |
| S83-09 | HaloFPX Section 48, `../../07_Distributed_Runtime/48_Distributed_Correctness_Determinism_Fault_Recovery_and_Degraded_Mode/`, verified 2026-07-16 | Determinism scope, asynchronous errors, fencing/recovery gaps | Protocol and target fault behavior remain open |
| S83-10 | HaloFPX Section 63 and pinned [CachyLLama cache source](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp), commit `6be745998f568e379ea197fcf827baec73ff9940` | Current checkpoint write/fsync/scan/header behavior and missing payload digest | No power-loss guarantee; bounded source inspection |
| S83-11 | HaloFPX Section 65, `../../09_HaloKV_Persistent_Cache/65_Cache_Inspection_Migration_Benchmarking_Write_Amplification_and_SSD_Endurance/`, verified 2026-07-16 | Cache counters versus device endurance evidence gap | Host writes are not NAND writes |
| S83-12 | HaloFPX Section 71, `../../10_Product_Server_and_Operations/71_Security_Trust_Boundaries_Permissions_Local_Network_and_Secrets/`, verified 2026-07-16 | Trust boundaries, insecure RPC warning, cache/privacy constraints | Threat model and machine controls remain open |

## Current primary sources

| ID | Primary source and revision | Supports | Limitations |
|---|---|---|---|
| S83-13 | [AMD Ryzen AI Max+ PRO 395 product specifications](https://www.amd.com/en/products/processors/laptop/ryzen-pro/ai-max-pro-300-series/amd-ryzen-ai-max-plus-pro-395.html), live page accessed 2026-07-17 | Two native USB4 40-Gbps ports, 128-GB memory maximum, 45-120-W cTDP, 100-C Tjmax, PCIe 4.0 | SoC capability, not the exact OEM BOM/topology/cooling/sustained limit |
| S83-14 | [AMD ROCm compatibility matrix](https://rocm.docs.amd.com/en/latest/compatibility/compatibility-matrix.html), current docs accessed 2026-07-17 | Current supported OS/kernel/device lanes | Broad support matrix; selected libraries and ROCmFPX still require tests |
| S83-15 | [AMD Ryzen limitations and recommended settings](https://rocm.docs.amd.com/projects/radeon-ryzen/en/latest/docs/limitations/limitationsryz.html), current docs accessed 2026-07-17 | Current gfx1151 LLM crash/performance caveats | Vendor-wide notice; applicability/fix status needs exact-tuple reproduction |
| S83-16 | [Linux USB4 and Thunderbolt documentation](https://docs.kernel.org/admin-guide/thunderbolt.html), rolling docs accessed 2026-07-17 | Domains, security/IOMMU, USB4NET/USB4STREAM, bandwidth events, firmware/retimers | Rolling documentation may describe a newer kernel than target nodes |
| S83-17 | [USB4 Specification v2.0](https://www.usb.org/document-library/usb4r-specification-v20), USB-IF, document page dated 2026-04-02 | Dynamic sharing, lanes, protocol tunneling, capability negotiation | Specification capability is not OEM topology or application performance |
| S83-18 | [llama.cpp SECURITY.md](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/SECURITY.md), commit `788e07dc91d266ad3162a1ce9037665656269689` | Operator duties for untrusted artifacts/networks/multi-tenancy | Policy, not proof of HaloFPX controls |
| S83-19 | [llama.cpp RPC README](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/rpc/README.md), commit `788e07dc91d266ad3162a1ce9037665656269689` | RPC proof-of-concept, fragile/insecure warning | Does not assess a future authenticated HaloFPX transport |
| S83-20 | [GHSA-j8rj-fmpv-wcxw](https://github.com/ggml-org/llama.cpp/security/advisories/GHSA-j8rj-fmpv-wcxw), ggml-org/llama.cpp, published 2026-03-26 | Concrete unauthenticated RPC RCE failure mode and exposure consequence | Advisory version statement must be checked against exact candidate code; not used to label the pinned commit vulnerable |
| S83-21 | [NVM Express Base Specification 2.2](https://nvmexpress.org/wp-content/uploads/NVM-Express-Base-Specification-Revision-2.2-2025.03.11-Ratified-1.pdf), ratified 2025-03-11 | SMART/health, write/flush/controller framework | Device behavior, TBW, firmware, and NAND write amplification are implementation-specific |
| S83-22 | Agent Harness `AGENTS.md`, `guide/architecture.md`, and `reviews/AGENTS.md`, canonical local path `C:\Users\britt\Documents\Agent_Harness`, read 2026-07-17 | Evidence promotion, reversibility, independent review, improvement closeout | Conceptual/governance authority; not technical proof about HaloFPX hardware |
| S83-23 | [2026-08-12 nimo-2 HMM/global-OOM safety incident](../../../../../docs/halofpx/evidence/2026-08-12-target-hmm-oom-incident/README.md), base `b77f2bce6e7875ab065e09894f45915585c9f156` | Raw kernel OOM/HMM accounting, interrupted build boundary, both-rank restart/recovery identities, and health-versus-real-inference readiness distinction | Environment-specific accidental incident; no calibrated allocation envelope, benchmark, or performance result |

## Freshness rule

**[RECOMMENDATION]** Refresh S83-01 and S83-14 through S83-20 before every release; refresh S83-13/17/21 when the BOM, transport, or storage design changes. Preserve new observations rather than rewriting the dated 2026-07-17 snapshot.
