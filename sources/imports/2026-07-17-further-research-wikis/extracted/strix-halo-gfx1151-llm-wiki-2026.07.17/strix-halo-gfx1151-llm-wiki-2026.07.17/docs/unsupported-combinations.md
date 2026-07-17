# Unsupported, known-bad, and unvalidated combinations

| Combination | Classification | Reason | Replacement |
|---|---|---|---|
| `linux-firmware-20251125` / MES 0x83 era | Known bad | GPU page faults, hangs, and distribution regressions | Fixed/current distribution firmware; Fedora 20260110 line is community-validated |
| Kernel below AMD’s RDNA 3.5 threshold | Officially below stable floor | Missing target fixes | HWE >=6.17.0-19, OEM >=6.14.0-1018, or >=6.18.4 elsewhere |
| ROCm 7.1.x as a new gfx1151 deployment | Officially unsupported in current target table | CWSR/MES and application regressions | ROCm 7.2.1–7.2.3 official target lane or current 7.14 Core SDK |
| ROCm 6.4.x as an official current profile | Officially unsupported in current target table | Community patches do not change vendor status | Use supported line; retain patched images only for controlled comparison |
| `HSA_OVERRIDE_GFX_VERSION=11.0.0` or `11.0.3` as a generic fix | Unsupported workaround | Failed to repair reported old-stack faults and can select wrong ISA assumptions | Native gfx1151 identification on current stack |
| ROCm 7.14 runtime with ROCm 7.2 LLVM/CMake packages | Unsupported-by-scope | Mixed compiler resource, bitcode, CMake, and runtime generations | One SDK root and clean build directory |
| b10064 ROCm 7.2 prebuilt loaded against arbitrary 7.14 libraries | Unvalidated | Binary dynamic-dependency scope differs | Run with intended ROCm 7.2 runtime or rebuild from source for 7.14 |
| `GGML_HIP_ROCWMMA_FATTN=ON` as gfx1151 default | Community-reported regression | Long-context prefill loss in reported ROCm 7.2.1 test | Keep OFF, then A/B locally |
| AMDVLK for any large model without allocation test | Risky | Reported approximately 2 GiB single-allocation failure | RADV first |
| Mesa 26.1.5 called “gfx1151 validated” without local test | Unvalidated wording | It is current upstream, while captured target reports used 26.0.2/26.0.3 | Label current-upstream candidate and run acceptance gate |
| ROCmFPX commit a5605 with ROCm 7.14 assumed compatible | Unvalidated | Fork baseline is ROCm 7.2.1 | Use pinned container or perform a new qualification |
| `thunderbolt-ibverbs` in production or untrusted environments | Explicitly unsupported by project | Buggy and insecure research driver | Standard `thunderbolt_net` TCP/IP or production RDMA hardware |
| `amd_iommu=off` combined with untrusted USB4/RDMA | Security-risk combination | Weakens DMA isolation | Default IOMMU or `iommu=pt`, then benchmark |

Evidence: [AMD-RDNA35](sources.md#amd-rdna35), [ROCM-ISSUE-5724](sources.md#rocm-issue-5724), [ROCM-ISSUE-5590](sources.md#rocm-issue-5590), [ROCM-ISSUE-5824](sources.md#rocm-issue-5824), [LLAMA-ISSUE-24437](sources.md#llama-issue-24437), [LLAMA-ISSUE-15054](sources.md#llama-issue-15054), [THUNDERBOLT-IBVERBS-76BA39B](sources.md#thunderbolt-ibverbs-76ba39b).

## “Unsupported” versus “not yet validated”

- **Unsupported** means an authority explicitly excludes the combination or a known-bad record exists.
- **Unvalidated** means no source in this snapshot certifies the combined profile. It may work; it must not be represented as supported until tested or documented.
- **Historical** means the path was once official but has been superseded by a current matrix.
