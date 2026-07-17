---
section_id: "42"
title: "Tensor Parallel Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate"]
  software_versions: []
  hardware_revisions: []
related_sections: ["30", "31", "44", "48", "51", "52"]
---

# Open questions

| ID | **[OPEN]** question | Needed evidence |
|---|---|---|
| DR42-O1 | Which target architectures/quant formats have valid two-way shard axes and kernels? | `DR-42-E1` plus section 30/31 |
| DR42-O2 | Can RCCL use the intended USB4 paths and backend, or is a custom collective required? | build/source audit and `DR-42-E2` |
| DR42-O3 | What are one-link and striped dual-link p99 curves for real payloads? | `DR-42-E2` |
| DR42-O4 | Which collective dtype meets numerical and speed requirements? | oracle/error/latency matrix |
| DR42-O5 | How should MQA or odd KV-head models place K/V and cache? | memory/performance comparison |
| DR42-O6 | Which output-head strategy preserves sampler semantics affordably? | head prototype and sampler tests |
| DR42-O7 | Can reductions overlap useful work on actual graphs? | trace-based dependency/overlap proof |
| DR42-O8 | Does TP ever beat replication at p99, or is it capacity-only? | `DR-42-E4` |
| DR42-O9 | How are rank-local persistent caches/checkpoints represented and recovered? | sections 48, 55-64 |
