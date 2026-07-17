---
section_id: "80"
title: "Fault Injection Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: ["Linux fault-injection documentation accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["54", "56", "62", "65", "68", "76", "77", "78", "79", "81"]
---

# Open questions

1. [OPEN] What are the exact coordinator, worker, rank, rail, and request epoch fields and ownership transitions?
2. [OPEN] Which requests are safe to retry, and what idempotency key prevents duplicate tokens, tool calls, or side effects?
3. [OPEN] Under what model placement and state conditions is single-node fallback actually complete?
4. [OPEN] What recovery-time and lost-work budgets apply to each fault class?
5. [OPEN] Which GPU reset mechanism is supported for the exact Strix Halo driver/firmware stack without rebooting or risking the host?
6. [OPEN] Which USB4 counters and control surfaces can identify each rail and a retraining event?
7. [OPEN] What cache/state atomicity and durability contract is required across process and power loss?
8. [OPEN] Which disposable filesystem/device topology can safely reproduce ENOSPC, read-only, EIO, timeout, and corruption?
9. [OPEN] What stale-version combinations must be tested across coordinator, worker, model, cache schema, and protocol?
10. [OPEN] Which combined faults are credible and safe after single-fault qualification?
11. [OPEN] Does the production supervisor impose restart-rate limits or escalation behavior that the harness must account for?
