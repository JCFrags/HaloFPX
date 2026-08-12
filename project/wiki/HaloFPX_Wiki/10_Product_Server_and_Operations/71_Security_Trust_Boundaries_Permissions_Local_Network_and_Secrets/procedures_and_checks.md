---
section_id: "71"
title: "Security Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["security design candidate 2026-07-16"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["69", "70", "72"]
---

# Procedures and checks

## Safety and privilege boundary

Run abuse, parser, cache, and transport mutation tests only in an isolated disposable deployment with copied fixtures and a dedicated sacrificial cache/store. Resolve and record every target path/interface before mutation; preserve sole model/source/evidence copies; declare root, network, device, or tracing privileges; set CPU/memory/disk/time ceilings and stop conditions; keep out-of-band recovery access; and record cleanup. Never target the production listener, boot disk, workspace, production cache/model store, or unredacted secrets. Physical, kernel, device-reset, or cable faults require the Section 80 authorization procedure.

## SEC-71-E1 — threat/control verification

Inventory every listener, process identity, device node, directory mode/ACL, credential source, firewall rule, and peer identity. In the isolated deployment, attempt unauthorized client/admin/peer access, expired/revoked credentials, downgrade, replay, and tampered control and bulk DATA traffic. Retain packet captures only after confirming they do not expose secrets/content.

## SEC-71-E2 — authorization, path, and parser abuse

Test role boundaries and fuzz route/model/cache identifiers with absolute paths, `..`, alternate separators, symlinks, hard links, races, oversized metadata, malformed GGUF, truncation, and decompression/resource bombs where applicable. Pass if requests fail closed without escaping roots, corrupting state, or wedging readiness.

## SEC-71-E3 — cache isolation and privacy

Use two principals and secret canaries. Exercise hit/miss, eviction, restart, cancellation, bundle generation, backup, and corrupt/tampered objects. Pass if no cross-principal hit or disclosure occurs by default and every invalid object produces miss/recompute or explicit safe failure.

## SEC-71-E4 — peer MITM and replay

Place a controlled proxy between disposable peer endpoints. Attempt peer impersonation, certificate/key substitution, byte modification, valid-CRC payload modification, replay, reordering, truncation, capability downgrade, and plaintext fallback against both control and bulk DATA records. Pass only if peers fail closed before accepting modified inference state, record a redacted audit event, and do not silently accept reduced security.

## SEC-71-E5 — RPC artifact and exposure gate

Without starting an unproven listener, record both peers' source commit and relevant RPC blob IDs, executable and loaded-library hashes, toolchain/CMake identity, and `GGML_RPC` setting. Mechanically confirm the reviewed serialization and null-buffer/non-null-data rejection guards. In an isolated disposable network, prove the listener binds only the intended private address, management/LAN paths are rejected, service/device/filesystem privileges are minimal, and a synthetic non-operational malformed tensor is rejected before backend graph execution. Preserve redacted evidence and restart both peers from the proven artifacts. If any provenance, guard, exposure, or privilege check is missing, stop/disable RPC and record the gate as failed; do not run public exploit payloads against a live service.

## Release blockers

- **[RECOMMENDATION]** Treat unauthenticated remote admin, secret leakage, accepted corrupt cache, path escape, silent peer downgrade, unverified model activation, or enabled RPC without proven fixed-artifact provenance and bounded exposure as release-blocking.
- **[RECOMMENDATION]** Record unresolved upstream parser/RPC vulnerabilities against the exact commit and preserve reproduction artifacts before upgrading or patching.
