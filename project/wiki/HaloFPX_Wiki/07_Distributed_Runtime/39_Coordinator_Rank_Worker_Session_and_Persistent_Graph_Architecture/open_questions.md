---
section_id: "39"
title: "Coordinator and Rank Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate"]
  software_versions: []
  hardware_revisions: []
related_sections: ["32", "45", "48", "55"]
---

# Open questions

| ID | **[OPEN]** question | Resolution evidence |
|---|---|---|
| DR39-O1 | Which process is coordinator, and can leadership move without duplicating outputs? | failure protocol and two-node test |
| DR39-O2 | What exact model/shard/cache ABI manifest is implementable in the fork? | loader/cache source inventory |
| DR39-O3 | Which HIP graph nodes and parameter updates work on the target driver/backend? | `DR-39-E2` |
| DR39-O4 | Are full logits transferred, sampled rank-locally, or reduced by distributed top-k? | section 42 design/measurement |
| DR39-O5 | What session state is sufficient for exact restart/migration? | sections 48 and 55-64 |
| DR39-O6 | What deadlines distinguish slow from failed without false fencing? | soak plus injected jitter |
| DR39-O7 | How are credentials and transport integrity established on the private fabric? | security decision |
