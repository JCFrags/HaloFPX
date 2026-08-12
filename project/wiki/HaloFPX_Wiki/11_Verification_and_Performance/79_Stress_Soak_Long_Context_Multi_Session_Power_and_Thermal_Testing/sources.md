---
section_id: "79"
title: "Stress, Soak, Long-Context, Multi-Session, Power, and Thermal Sources"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: ["AMD SMI CLI documentation 26.5.0", "Linux documentation accessed 2026-07-17"]
  hardware_revisions: []
related_sections: ["73", "74", "75", "77", "78", "80"]
---

# Sources

Accessed 2026-07-17. Source mechanisms were inspected; no target workload was run.

| ID | Source and revision | Claims supported | Limitations/conflicts |
|---|---|---|---|
| S79-01 | [llama.cpp server benchmark at `788e07d`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/bench/README.md) | Concurrent duration-based k6 workload and request metrics | Workload generator, not stability evidence |
| S79-02 | [llama.cpp `llama-bench` at `788e07d`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/llama-bench/README.md) | Controlled model benchmark mechanism | Excludes some request-serving work; not soak proof |
| S79-03 | [llama.cpp batched benchmark at `788e07d`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/batched-bench/README.md) | Multi-sequence/batched workload mechanism | Does not establish fairness or long-run cleanup |
| S79-04 | [ROCmFPX long-context smoke at `a5605a7`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/check-rocmfpx-long-context-smoke.sh) | Long-context smoke inputs and default synthetic size | Model-dependent smoke can skip; not maximum-context proof |
| S79-05 | [AMD SMI CLI documentation 26.5.0, accessed 2026-07-17](https://rocm.docs.amd.com/projects/amdsmi/en/latest/how-to/amdsmi-cli-tool.html) | AMD monitoring and supported control interface | Moving documentation; installed version and gfx1151 fields require inventory |
| S79-06 | [Linux hwmon sysfs interface, accessed 2026-07-17](https://docs.kernel.org/hwmon/sysfs-interface.html) | Standard sensor attributes and device-dependent limits | Exact target kernel/driver fields remain open |
| S79-07 | [Linux USB4/Thunderbolt documentation, accessed 2026-07-17](https://docs.kernel.org/admin-guide/thunderbolt.html) | Device/topology and authorization interfaces | Does not guarantee link telemetry on target hardware |
| S79-08 | [fio documentation, accessed 2026-07-17](https://fio.readthedocs.io/en/latest/fio_doc.html) | Versionable storage workload controls | Exact fio version must be frozen; use only under section safety restrictions |
