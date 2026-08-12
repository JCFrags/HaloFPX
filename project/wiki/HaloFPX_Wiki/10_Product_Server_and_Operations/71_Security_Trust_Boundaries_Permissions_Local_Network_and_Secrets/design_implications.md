---
section_id: "71"
title: "Security Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["security design candidate 2026-07-16"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["66", "67", "68", "69", "70", "72"]
---

# Design implications

## Assets and boundaries

| Boundary | Threats | Candidate controls |
|---|---|---|
| Client to API | unauthorized use, content leakage, DoS | loopback default; TLS 1.3 plus scoped authentication off-host; quotas/timeouts |
| Operator to admin | privilege misuse, secret theft | separate protected listener/socket; roles; audit; short-lived/rotatable credentials |
| Coordinator to rank | impersonation, tampering, replay, downgrade | mutual peer identity, transcript integrity, freshness, version/capability negotiation |
| Process to files | traversal, symlink race, corruption | fixed roots, canonical descriptor-based access, no absolute/`..`, ownership/modes, hashes, atomic writes |
| Model/parser | malicious GGUF/metadata, resource exhaustion | staging sandbox, size/depth limits, signed/hash manifest, no implicit remote fetch |
| Cache/multi-user | cross-principal disclosure, stale/corrupt state | per-principal namespace/quota, authenticated object identity, reject/recompute, explicit sharing policy |
| Build/update | dependency or artifact compromise | pinned source, SBOM, provenance/signature verification, reproducible evidence where feasible |

## Secure defaults

- **[RECOMMENDATION]** Bind client API and admin API to loopback by default. Off-host client access requires TLS and authentication; admin access should prefer a local Unix socket or mutually authenticated dedicated listener.
- **[RECOMMENDATION]** Peer commands and every transferred inference-state DATA record require mutual peer authentication, integrity, freshness, and replay protection. CRC alone is only accidental-corruption detection. Encrypt by default; a plaintext lab profile requires recorded threat acceptance and a cryptographic MAC over bulk records plus security context, sequence/epoch, and negotiated metadata. Any transport profile that authenticates only control traffic must not carry accepted inference state.
- **[RECOMMENDATION]** Load API/peer keys through protected credential files or `LoadCredential=`; avoid secrets in process arguments, environment dumps, logs, manifests, and support bundles. Define rotation and revocation before deployment.
- **[RECOMMENDATION]** Candidate modes are configuration `0750 root:halofpx`, secret material `0400` or narrowly `0440`, service state/cache `0700`, and verified model files root-owned/read-only. Exact UID/group/device ACLs require machine inspection.
- **[RECOMMENDATION]** A model enters the serving store only after isolated staging, license/provenance capture, SHA-256 verification, format/size validation, and atomic promotion. Production serving does not accept arbitrary URLs or user paths.
- **[RECOMMENDATION]** Cache reuse across principals is off by default. Cancellation/deletion must remove namespace references; physical erasure and GPU-memory zeroization claims require experiments.
- **[RECOMMENDATION]** Logs expose stable error/correlation codes, not prompts, output, credentials, authorization headers, peer keys, or raw cache content.

## RPC deployment security gate

**[RECOMMENDATION]** `GGML_RPC` and its listener remain disabled by default. Enabling bounded lab use requires retained evidence for both peers: exact reviewed source/blob identities including the `ba38f3b` guard, executable and loaded-library hashes, compiler/build options, listener bind, firewall reachability, service identity/device ACLs, and isolated negative-path behavior. Rebuild and restart both peers; never infer runtime remediation from a checked-out source tree or replace loaded libraries in place. Even after this gate, unmodified RPC remains unauthenticated lab-only transport and must not be presented as production-secure.

**[OPEN]** Current deployed artifact provenance and network exposure are unverified. Until they are observed, RPC is not an approved HaloFPX deployment capability.

**[OPEN]** Whether local household users are mutually trusted or separate security principals changes the required process/cache isolation and must be decided explicitly.
