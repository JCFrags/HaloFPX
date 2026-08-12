---
section_id: "24"
title: "HIP, HSA, RCCL, Memory Coherence, and Synchronization"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "ROCm/HIP@bc9af25177f96c0fea93198b89cf4c3cf08f3ea3"
    - "ROCm/ROCR-Runtime@e5498ba92dad7099d2027bd22bd7295ca1caf833"
    - "ROCm/rccl@96a25b5fd6f73fba58c7d83eb57cf19a50230aa4"
  software_versions: ["ROCm 7.2.3", "HIP 7.2.53211 documentation", "RCCL 2.27.7"]
  hardware_revisions: ["AMD Strix Halo gfx1151; exact two-machine revisions open"]
related_sections: ["19", "23", "25", "27", "42", "54", "74", "75"]
---

# 24: HIP, HSA, RCCL, memory coherence, and synchronization

## Orientation

**[VERIFIED]** ROCm 7.2 documents gfx1151 support for Ryzen AI Max systems, and AMD's Strix Halo guidance describes GPU memory access through per-process GPUVM rather than a separate discrete-VRAM pool. This establishes platform relevance, not the behavior of either project machine [S24-001, S24-002].

**[VERIFIED]** The research baseline is ROCm 7.2.3, released 2026-05-04 at repository commit `14f8138863403a26e0caef6671cfab9b09aa636e`; the release component table identifies RCCL 2.27.7 [S24-017].

**[RECOMMENDATION]** Treat local GPU-to-CPU publication and cross-host delivery as two different protocols:

1. a HIP/HSA completion plus local visibility boundary;
2. a transport send/receive plus remote protocol acknowledgment boundary.

**[VERIFIED]** HSA system scope covers agents in one HSA system. **[INFERENCE]** It cannot itself order or publish bytes across USB4 networking to the other host; the transport must supply that second happens-before edge [S24-007].

## Authoritative pages

- [Facts and constraints](facts_and_constraints.md) — API and memory-model facts.
- [Design implications](design_implications.md) — HaloFPX buffer and synchronization choices.
- [Procedures and checks](procedures_and_checks.md) — non-destructive two-machine experiment plan.
- [Open questions](open_questions.md) — unresolved platform and transport decisions.
- [Sources](sources.md) — primary-source ledger.

## Research split

### Internet and source-code research completed

- ROCm 7.2.3 release/tag identity, gfx1151 support, Strix Halo GPUVM guidance.
- HIP allocation flags, coherence modes, fences, atomics, streams, events, graphs, and peer-capability APIs.
- HSA memory-pool, queue, packet-fence, and signal concepts.
- RCCL network-plugin and profiling interfaces.

### Actual-machine work still required

- Run experiments EX24-01 through EX24-10 on both matched hosts.
- Record exact BIOS, kernel, firmware, ROCm packages, GPU agent properties, memory pools, queue limits, and transport topology.
- Prove visibility and ordering with litmus tests; no performance or correctness result is currently labeled `[MEASURED]`.

### Decisions contingent on measurements

- mapped fine-grained host buffers versus explicit staged copies;
- CPU polling versus event-driven completion;
- HIP graphs around stable compute phases;
- RCCL sockets, a network plugin, or the project transport abstraction for two-rank collectives.

## Status

This section is source-backed but remains `needs-machine-validation`. It must be revisited when ROCm, the kernel/firmware matrix, or the transport implementation changes.
