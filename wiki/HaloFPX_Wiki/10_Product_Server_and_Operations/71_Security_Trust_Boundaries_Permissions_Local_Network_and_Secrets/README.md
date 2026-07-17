---
section_id: "71"
title: "Security, Trust Boundaries, Permissions, Local Network, and Secrets"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "llama.cpp", "CachyLlama"]
  software_versions: ["llama.cpp 788e07dc91d266ad3162a1ce9037665656269689", "CachyLlama 6be745998f568e379ea197fcf827baec73ff9940"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["66", "67", "68", "69", "70", "72"]
---

# Security, Trust Boundaries, Permissions, Local Network, and Secrets

**[RECOMMENDATION]** Default to loopback-only client service, least-privilege static service identities, authenticated operator actions, verified read-only model artifacts, private per-principal cache namespaces, and mutually authenticated peer traffic. A direct cable or trusted LAN is not an authentication mechanism.

**[VERIFIED]** The pinned llama.cpp security policy says untrusted models should be isolated/sandboxed, artifacts should be hash-verified, network data should be encrypted, and multi-tenant isolation is the operator's responsibility [S71-01]. Its RPC documentation calls RPC fragile and insecure [S71-02].

## Research split

- Online/source research completed: upstream threat warnings, zero-trust principles, TLS 1.3, systemd credentials/sandboxing, SLSA provenance, SSDF, and cache source inspection.
- Machine/source validation required: listeners/firewall, peer authentication and replay behavior, effective permissions, secret handling, path/parser fuzzing, cache isolation, GPU residual state, and artifact verification.
- Contingent decisions: client/peer identity scheme, certificate/key lifecycle, tenant model, encryption requirement, secret store, model signing authority, and supported exposure boundary.

See [facts and constraints](facts_and_constraints.md), [design implications](design_implications.md), [procedures and checks](procedures_and_checks.md), [open questions](open_questions.md), and [sources](sources.md).
