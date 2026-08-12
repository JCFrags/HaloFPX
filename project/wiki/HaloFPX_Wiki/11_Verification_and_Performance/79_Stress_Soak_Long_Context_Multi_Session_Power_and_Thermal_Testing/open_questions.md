---
section_id: "79"
title: "Stress, Soak, Long-Context, Multi-Session, Power, and Thermal Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: ["AMD SMI CLI documentation 26.5.0", "Linux documentation accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["73", "74", "75", "76", "77", "78", "80", "81"]
---

# Open questions

1. [OPEN] What exact ambient, junction, skin, NVMe, power, and throttling limits apply to each hardware revision and enclosure?
2. [OPEN] Which power profiles are supported release configurations, and how are their controls and actual applied state recorded?
3. [OPEN] What soak duration covers the longest expected cache/writeback, model-switch, and thermal failure timescale?
4. [OPEN] What representative arrival, prompt-length, output-length, session, cancellation, and idle-time distributions are approved?
5. [OPEN] What fairness and interactive-latency limits are user-visible release requirements?
6. [OPEN] What is the declared context maximum for each model/backend/topology, and which retrieval fixtures validate it?
7. [OPEN] Which device and driver counters expose USB4 rail errors, retraining, retransmits, and negotiated width/speed on the target nodes?
8. [OPEN] What resource-slope and performance-drift thresholds distinguish leakage from measurement noise and cache warm-up?
9. [OPEN] What scratch storage capacity, write budget, endurance margin, and abort thresholds are safe?
10. [OPEN] Which model-switch sequences and cache working sets represent production?
