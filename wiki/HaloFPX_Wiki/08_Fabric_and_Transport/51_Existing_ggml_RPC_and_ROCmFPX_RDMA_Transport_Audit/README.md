---
section_id: "51"
title: "Existing ggml RPC and ROCmFPX RDMA Transport Audit"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: ["libibverbs when detected on Linux"]
  hardware_revisions: ["RDMA device availability unverified"]
related_sections: ["11", "12", "15", "32", "39", "49", "50", "53", "54", "55"]
---

# 51 - Existing ggml RPC and ROCmFPX RDMA Transport Audit

**[VERIFIED]** At the pinned revisions, ROCmFPX and upstream llama.cpp have byte-identical `ggml-rpc/transport.cpp` and `CMakeLists.txt` files (SHA-256 recorded in [facts_and_constraints.md](facts_and_constraints.md)). The RDMA path is therefore not a distinct ROCmFPX-only transport at this snapshot.

**[RECOMMENDATION]** Reuse RPC for bring-up, remote allocation/tensor/graph semantics, and a TCP baseline. Do not treat its current synchronous host-staged verbs path as the final HaloFPX fabric.

**[RECOMMENDATION] SECURITY GATE:** Keep RPC disabled unless both peers prove that the deployed executables and loaded libraries were built from reviewed source containing the `ba38f3b` malformed-tensor rejection, and the listener/exposure/least-privilege checks pass. The pinned llama.cpp and ROCmFPX source contain the relevant guards, but installed artifact provenance and current network exposure remain **[OPEN]**; no patched-release claim is made [S51-07, S51-08].

## Pages

- [Facts and constraints](facts_and_constraints.md)
- [Design implications](design_implications.md)
- [Procedures and checks](procedures_and_checks.md)
- [Open questions](open_questions.md)
- [Sources](sources.md)
