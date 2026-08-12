---
section_id: "26"
title: "Toolchain open questions"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp", "fewtarius/CachyLLama"]
  software_versions: ["ROCm 7.2.3 research comparison", "CMake 4.4.0 documentation comparison"]
  hardware_revisions: ["gfx1151 Strix Halo"]
related_sections: ["24", "25", "27", "28"]
---

# Toolchain open questions

| ID | Question | Required evidence | Blocks |
|---|---|---|---|
| O26-01 | **[OPEN]** Which exact OS/KMD/firmware/ROCm package tuple is supported and installed on both project APUs? | Immutable compatibility evidence plus host package inventory | Production baseline |
| O26-02 | **[OPEN]** Does the selected HIP build emit and load native `gfx1151` code objects without override variables? | Object inspection plus runtime load test on each node | HIP release build |
| O26-03 | **[OPEN]** Which upstream/fork commit becomes the integration base? | Reviewed decision tied to source diffs | Dependency lock |
| O26-04 | **[OPEN]** Is LLD fully compatible with every linked ROCm/Vulkan/system library? | Clean link, symbol audit, startup and workload tests | Linker choice |
| O26-05 | **[OPEN]** Are host, GPU code, packages and generated shaders bit-reproducible across paths and nodes? | Two clean rebuilds plus diffoscope | Reproducibility claim |
| O26-06 | **[OPEN]** Does LTO improve end-to-end token performance without correctness/debuggability regression? | Matched experiment | LTO promotion |
| O26-07 | **[OPEN]** Which representative prompts/models train PGO without overfitting? | Versioned corpus and held-out comparison | PGO promotion |
| O26-08 | **[OPEN]** Which sanitizer/debug modes work on the actual gfx1151 runtime, and what code is not instrumented? | Fault-injection validation | Diagnostic matrix |
| O26-09 | **[OPEN]** What is the exact Python package/runtime requirement after repository integration? | Source import/build inventory and hashed lock | Python version pin |
| O26-10 | **[OPEN]** How will release manifests be signed and verified by two-rank startup? | Threat model and implementation decision | Supply-chain gate |

## Internet follow-up

- Pin immutable documentation revisions, package manifests, and source commits for the accepted ROCm/HIP/CMake integration; do not substitute a moving `latest` or `develop` snapshot.
- Inventory build options and minimum versions at the chosen full commits of ROCmFPX, llama.cpp and imported projects.
- Resolve the supported Python range from the accepted OS/ROCm packaging, not a generic language support table.

## Machine experiments

Execute O26-01, O26-02, O26-04 through O26-08 on both matched hosts. Store raw evidence and do not promote conclusions from a one-host result to the pair.
