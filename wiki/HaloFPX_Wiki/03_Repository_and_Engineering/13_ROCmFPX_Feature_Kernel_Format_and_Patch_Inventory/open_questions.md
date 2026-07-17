---
section_id: "13"
title: "ROCmFPX Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: []
  hardware_revisions: ["Two matched HaloFPX Strix Halo nodes"]
related_sections: ["15", "23", "30", "33", "36", "37", "74", "78"]
---

# Open questions

| ID | Question | Why unresolved | Evidence / owner route |
|---|---|---|---|
| S13-OQ-01 | What exact upstream commit should become the new patch-stack base? | fork has no merge base; `b9438/22cadc194` is historical documentation, not current ancestry | semantic tree comparison; section 11/15 |
| S13-OQ-02 | Are custom GGML/GGUF numeric IDs collision-free for the chosen base and durable file exchange? | IDs 100–107 are fork-local | enum/schema audit plus read/write round trips |
| S13-OQ-03 | Will HaloFPX implement Vulkan Q2, restrict Q2 to HIP/CPU, or exclude it? | Q2 Vulkan wiring is absent at `a5605a7` | design decision after quality/need assessment |
| S13-OQ-04 | Which weight formats meet model-quality and agent/tool gates? | repository reports are model/machine-specific; no Halo measurements | section 78 matched evaluation |
| S13-OQ-05 | Do HIP and Vulkan custom kernels agree for every required op/shape on both nodes? | dispatch presence is not runtime proof | backend-op matrix with fallback tracing |
| S13-OQ-06 | Which backend wins for prefill/decode at target contexts and model families? | historical tables are not universal or fully reproducible here | section 74 controlled benchmarks |
| S13-OQ-07 | Are Q6 endpoint and Q2 correction histories fully represented in every CPU/HIP/Vulkan path? | multiple late corrective commits indicate drift risk | exhaustive codebook vectors and backend parity |
| S13-OQ-08 | Can the marker-based capability detector be replaced by parsed GGUF metadata/tensors? | bounded byte/filename scanning has false-positive/negative modes | implement parser; real-model fixture corpus |
| S13-OQ-09 | Which MTP architectures and head counts are correct at this pin? | Qwen/Gemma/Step/HY3 ports have model-specific graph fixes | per-architecture conversion, metadata, greedy parity |
| S13-OQ-10 | How does MTP state map to two ranks and degraded single-node mode? | current wrappers are single-process/local-device oriented | distributed state protocol and fault injection |
| S13-OQ-11 | Are Turbo3/4 acceptable for K, V, draft K/V, and long contexts? | asymmetric policy is a fork recommendation, not Halo evidence | filled-context quality/throughput matrix |
| S13-OQ-12 | Should the fork SSD prompt cache be ported, replaced, or disabled? | overlaps HaloKV and lacks the project’s complete durability/ownership contract | section 56–65 semantic audit and corruption tests |
| S13-OQ-13 | Which scripts belong in CI versus machine-only qualification? | many depend on private/local models, Linux GPU state, or large storage | section 16/81 test classification |
| S13-OQ-14 | Are repository benchmark claims reproducible from committed raw data and full environment metadata? | docs contain summaries but not a complete Halo-ready artifact bundle | source author follow-up; reproduce under section 73 |
| S13-OQ-15 | Which project-specific patches are already upstream-equivalent? | attribution is partial and upstream moved substantially | patch-id/semantic comparison at selected base |

Open-question count: **15**.

<a id="s13-internet-followup"></a>
## Immediate Internet follow-up

- Check ROCmFPX issues/PRs for Q2 Vulkan, format-ID stability, and release/tag policy.
- Map every local commit to upstream PR/commit or a project-owned patch ID.
- Inspect upstream changes to quant enums, HIP/Vulkan type traits, MTP, and server prompt cache after the selected base.
- Request raw benchmark manifests/logs for any result proposed as an acceptance floor.

<a id="s13-machine-followup"></a>
## Immediate machine follow-up

- Build the pin on both nodes and run references plus backend ops.
- Create one matched BF16→Q4/Q6/Q8 artifact set with hashes and tensor manifests.
- Establish CPU/HIP/Vulkan correctness before throughput.
- Exercise one MTP model and one non-MTP model; verify detector output against parsed metadata.
- Run asymmetric TurboQuant and cache-corruption tests at filled context.
