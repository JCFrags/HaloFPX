# Source Registry

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.

All sources are primary project/vendor documentation or source repositories. Access snapshot: **2026-07-17**. Upstream content can change; each machine run must store its own retrieval time and immutable commit/tag where available.

## SRC-001

- **Title:** ROCm Core SDK 7.14.0 release notes
- **Publisher:** AMD
- **URL:** `https://rocm.docs.amd.com/en/latest/about/release-notes.html`
- **Claim scope:** release/support/profiler changes
- **Accessed:** 2026-07-17

## SRC-002

- **Title:** ROCm 7.14.0 compatibility matrix
- **Publisher:** AMD
- **URL:** `https://rocm.docs.amd.com/en/latest/compatibility/compatibility-matrix.html`
- **Claim scope:** supported OS, APU, driver and component combinations
- **Accessed:** 2026-07-17

## SRC-003

- **Title:** Ryzen Linux support matrices
- **Publisher:** AMD
- **URL:** `https://rocm.docs.amd.com/projects/radeon-ryzen/en/latest/docs/compatibility/compatibilityryz/native_linux/native_linux_compatibility.html`
- **Claim scope:** gfx1151/Ryzen production support channel
- **Accessed:** 2026-07-17

## SRC-004

- **Title:** AMD RDNA 3.5 system optimization
- **Publisher:** AMD
- **URL:** `https://rocm.docs.amd.com/en/latest/how-to/system-optimization/rdna3-5.html`
- **Claim scope:** Strix Halo kernel requirements and system settings
- **Accessed:** 2026-07-17

## SRC-005

- **Title:** AMD Ryzen AI Halo developer platform
- **Publisher:** AMD
- **URL:** `https://www.amd.com/en/products/processors/desktops/ryzen/ryzen-ai-halo.html`
- **Claim scope:** platform family and current product/software positioning
- **Accessed:** 2026-07-17

## SRC-006

- **Title:** Trillion-parameter LLM on Ryzen AI Max+ cluster
- **Publisher:** AMD
- **URL:** `https://www.amd.com/en/developer/resources/technical-articles/2026/how-to-run-a-one-trillion-parameter-llm-locally-an-amd.html`
- **Claim scope:** llama.cpp RPC cluster reference; not USB4 validation
- **Accessed:** 2026-07-17

## SRC-007

- **Title:** llama.cpp RPC README
- **Publisher:** ggml-org
- **URL:** `https://github.com/ggml-org/llama.cpp/blob/master/tools/rpc/README.md`
- **Claim scope:** RPC topology, proof-of-concept and security warning
- **Accessed:** 2026-07-17

## SRC-008

- **Title:** llama-bench README
- **Publisher:** ggml-org
- **URL:** `https://github.com/ggml-org/llama.cpp/blob/master/tools/llama-bench/README.md`
- **Claim scope:** prompt-processing, text-generation and combined benchmark modes
- **Accessed:** 2026-07-17

## SRC-009

- **Title:** llama.cpp server README
- **Publisher:** ggml-org
- **URL:** `https://github.com/ggml-org/llama.cpp/blob/master/tools/server/README.md`
- **Claim scope:** server timings, streaming and prompt-cache behavior
- **Accessed:** 2026-07-17

## SRC-010

- **Title:** Linux USB4 and Thunderbolt guide
- **Publisher:** Linux kernel community
- **URL:** `https://docs.kernel.org/admin-guide/thunderbolt.html`
- **Claim scope:** USB4NET host-to-host operation and security model
- **Accessed:** 2026-07-17

## SRC-011

- **Title:** USB4 specification overview
- **Publisher:** USB Implementers Forum
- **URL:** `https://www.usb.org/usb4`
- **Claim scope:** USB4 signaling and architecture; nominal capability only
- **Accessed:** 2026-07-17

## SRC-012

- **Title:** Linux Thunderbolt sysfs ABI
- **Publisher:** Linux kernel community
- **URL:** `https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-bus-thunderbolt`
- **Claim scope:** RX/TX speed, lane and topology attributes
- **Accessed:** 2026-07-17

## SRC-013

- **Title:** AMDGPU power and thermal monitoring
- **Publisher:** Linux kernel community
- **URL:** `https://docs.kernel.org/gpu/amdgpu/thermal.html`
- **Claim scope:** GPU busy, memory busy, gpu_metrics, power and thermal fields
- **Accessed:** 2026-07-17

## SRC-014

- **Title:** ROCm Compute Profiler compatible APUs
- **Publisher:** AMD
- **URL:** `https://rocm.docs.amd.com/projects/rocprofiler-compute/en/develop/reference/compatible-accelerators.html`
- **Claim scope:** Strix/Halo profiler support
- **Accessed:** 2026-07-17

## SRC-015

- **Title:** vLLM production metrics
- **Publisher:** vLLM project
- **URL:** `https://docs.vllm.ai/en/stable/design/metrics/`
- **Claim scope:** TTFT, inter-token latency, E2E and serving metrics
- **Accessed:** 2026-07-17

## SRC-016

- **Title:** vLLM benchmark CLI
- **Publisher:** vLLM project
- **URL:** `https://docs.vllm.ai/en/latest/benchmarking/cli/`
- **Claim scope:** serving benchmark and load-ramp methodology
- **Accessed:** 2026-07-17

## SRC-017

- **Title:** MLPerf Inference
- **Publisher:** MLCommons
- **URL:** `https://mlcommons.org/working-groups/benchmarks/inference/`
- **Claim scope:** benchmark reproducibility and inference measurement context
- **Accessed:** 2026-07-17

## SRC-018

- **Title:** MLPerf Client benchmark
- **Publisher:** MLCommons
- **URL:** `https://mlcommons.org/benchmarks/client/`
- **Claim scope:** separate accuracy qualification from performance
- **Accessed:** 2026-07-17

## SRC-019

- **Title:** Linux /proc/sys/vm drop_caches
- **Publisher:** Linux kernel community
- **URL:** `https://docs.kernel.org/admin-guide/sysctl/vm.html#drop-caches`
- **Claim scope:** test-only cache control and caveat
- **Accessed:** 2026-07-17

## SRC-020

- **Title:** Linux control group v2
- **Publisher:** Linux kernel community
- **URL:** `https://docs.kernel.org/admin-guide/cgroup-v2.html`
- **Claim scope:** per-cgroup resource and IO accounting
- **Accessed:** 2026-07-17

## SRC-021

- **Title:** Linux Pressure Stall Information
- **Publisher:** Linux kernel community
- **URL:** `https://docs.kernel.org/accounting/psi.html`
- **Claim scope:** CPU, memory and IO stall telemetry
- **Accessed:** 2026-07-17

## SRC-022

- **Title:** GitHub scheduled workflow events
- **Publisher:** GitHub
- **URL:** `https://docs.github.com/actions/using-workflows/events-that-trigger-workflows#schedule`
- **Claim scope:** cron behavior, delay and default-branch constraints
- **Accessed:** 2026-07-17

## SRC-023

- **Title:** GitHub REST commits API
- **Publisher:** GitHub
- **URL:** `https://docs.github.com/en/rest/commits/commits`
- **Claim scope:** commit polling and signature metadata
- **Accessed:** 2026-07-17

## SRC-024

- **Title:** AMD Product Security
- **Publisher:** AMD
- **URL:** `https://www.amd.com/en/resources/product-security.html`
- **Claim scope:** security bulletins and briefs
- **Accessed:** 2026-07-17

## SRC-025

- **Title:** Mesa release notes
- **Publisher:** Mesa project
- **URL:** `https://docs.mesa3d.org/relnotes.html`
- **Claim scope:** Mesa feature and bug-fix release stream
- **Accessed:** 2026-07-17

## SRC-026

- **Title:** ROCm GitHub repository
- **Publisher:** AMD
- **URL:** `https://github.com/ROCm/ROCm`
- **Claim scope:** ROCm releases, issues and source coordination
- **Accessed:** 2026-07-17

## SRC-027

- **Title:** TheRock repository
- **Publisher:** AMD
- **URL:** `https://github.com/ROCm/TheRock`
- **Claim scope:** nightly/build platform and GPU support readiness
- **Accessed:** 2026-07-17

## SRC-028

- **Title:** ROCm Systems repository
- **Publisher:** AMD
- **URL:** `https://github.com/ROCm/rocm-systems`
- **Claim scope:** current AMD SMI and profiler source location
- **Accessed:** 2026-07-17

## SRC-029

- **Title:** ROCm 7.14 ecosystem post
- **Publisher:** AMD
- **URL:** `https://rocm.blogs.amd.com/ecosystems-and-partners/rocm-7.14-blog/README.html`
- **Claim scope:** Strix Halo profiler and telemetry expansion
- **Accessed:** 2026-07-17

## SRC-030

- **Title:** GitHub REST API overview
- **Publisher:** GitHub
- **URL:** `https://docs.github.com/en/rest`
- **Claim scope:** authenticated polling and rate-limit-aware automation
- **Accessed:** 2026-07-17
