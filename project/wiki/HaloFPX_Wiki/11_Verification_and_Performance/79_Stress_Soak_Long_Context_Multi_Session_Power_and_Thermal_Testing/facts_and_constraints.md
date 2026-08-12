---
section_id: "79"
title: "Stress, Soak, Long-Context, Multi-Session, Power, and Thermal Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: ["AMD SMI CLI documentation 26.5.0", "Linux documentation accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["73", "74", "75", "76", "77", "78", "80", "81"]
---

# Facts and constraints

- **[VERIFIED]** Pinned `llama.cpp` provides `llama-bench`, `batched-bench`, a server k6 workload, and a CI-oriented server speed benchmark [S79-01][S79-02][S79-03].
- **[VERIFIED]** The server k6 harness can issue concurrent requests for a declared duration and records request/token/truncation metrics [S79-01]. A benchmark harness is a workload generator, not proof of HaloFPX stability.
- **[VERIFIED]** AMD SMI CLI exposes monitoring and, on supported devices, control surfaces for AMD GPU metrics [S79-05]. Available fields and controls must be discovered on the actual Strix Halo nodes.
- **[VERIFIED]** Linux hwmon defines standardized sensor attributes, but exact sensor names and limits are driver/device dependent [S79-06].
- **[VERIFIED]** Linux USB4/Thunderbolt documentation exposes device/topology and authorization behavior; link-specific telemetry availability still requires target inspection [S79-07].
- **[VERIFIED]** ROCmFPX includes a long-context smoke script whose default is a synthetic 8192-token-class test [S79-04]. That is a smoke mechanism, not maximum-context proof.
- **[RECOMMENDATION]** Obtain temperature and power limits from the exact hardware/BOM and exposed firmware/driver controls; do not invent safe limits.
- **[RECOMMENDATION]** Thermal results require ambient temperature, power profile, fan/cooling configuration, enclosure state, clocks, and throttling indicators.
- **[RECOMMENDATION]** Long-context validity requires correctness assertions, not merely successful allocation or token emission.
- **[RECOMMENDATION]** Storage stress must use a dedicated scratch file or sacrificial filesystem with an explicit free-space budget, never a raw production model/cache device.
- **[RECOMMENDATION]** Reserve root privileges for specifically approved telemetry or controls; ordinary load generation and result collection should be unprivileged.
- **[OPEN]** No stress, soak, thermal, power, fairness, or maximum-context result was measured in this research pass.
