---
section_id: "71"
title: "Security Sources"
status: "verified"
last_verified: "2026-07-16"
applies_to:
  repositories: ["llama.cpp", "CachyLlama", "systemd", "Custom_Inference_Project"]
  software_versions: ["source snapshot 2026-07-16"]
  hardware_revisions: []
related_sections: ["69", "70", "72"]
---

# Sources

| ID | Source | Revision/access | Scope and use | Limitations |
|---|---|---|---|---|
| S71-01 | [llama.cpp security policy](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/SECURITY.md) | Commit `788e07dc91d266ad3162a1ce9037665656269689`; accessed 2026-07-16 | Untrusted model/input/network and multi-tenant warnings | Upstream policy; it does not implement HaloFPX isolation |
| S71-02 | [llama.cpp RPC README](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/rpc/README.md) | Commit `788e07dc91d266ad3162a1ce9037665656269689`; accessed 2026-07-16 | RPC insecurity/fragility warning | Does not specify a secure replacement transport |
| S71-03 | [NIST SP 800-207](https://doi.org/10.6028/NIST.SP.800-207) | Final publication, August 2020; accessed 2026-07-16 | Zero-trust resource and identity principles | Architecture guidance, not a product-specific control set |
| S71-04 | [RFC 8446: TLS 1.3](https://www.rfc-editor.org/rfc/rfc8446) | RFC 8446, August 2018; accessed 2026-07-16 | Transport security protocol | Secure use still depends on identity, configuration, and key lifecycle |
| S71-05 | [systemd manuals](https://github.com/systemd/systemd/tree/8009fa49845cd6fb7b7014ab06218b68fe702006/man) | Commit `8009fa49845cd6fb7b7014ab06218b68fe702006`; accessed 2026-07-16 | Credentials, privilege, filesystem and device sandboxing | Effective confinement requires unit and machine verification |
| S71-06 | [SLSA specification 1.2](https://slsa.dev/spec/v1.2/) | Version 1.2; accessed 2026-07-16 | Provenance and verification model | Does not choose project builders, signers, or policy |
| S71-07 | [NIST SP 800-218 SSDF 1.1](https://doi.org/10.6028/NIST.SP.800-218) | Version 1.1, February 2022; accessed 2026-07-16 | Secure software development practices | Process guidance, not runtime proof |
| S71-08 | [CachyLLama `kv-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp) and [`kv-ssd-cache.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.h) | Commit `6be745998f568e379ea197fcf827baec73ff9940`; source paths rechecked 2026-07-17 | Direct bounded inspection of cache format, hashing, permissions, and write path | Two files at one commit; not a whole-repository audit or runtime exploit proof |
| S71-09 | [GHSA-j8rj-fmpv-wcxw](https://github.com/ggml-org/llama.cpp/security/advisories/GHSA-j8rj-fmpv-wcxw) and [source fix commit `ba38f3b`](https://github.com/ggml-org/llama.cpp/commit/ba38f3becce7d1283585c73d796eb47d72bbbd30) | Advisory published 2026-03-26 and commit merged 2026-03-27; accessed 2026-07-17 | Documents the unauthenticated RPC RCE path and the exact source change that blocks its malformed tensor state | Advisory still names no patched version; does not establish deployed artifact provenance or general RPC safety |
| S71-10 | [Project RPC advisory mapping](../../../../reviews/follow-ups/2026-07-16__llama-cpp-rpc-rce__research__v01.md) | Local research follow-up, verified 2026-07-17 | Maps the advisory/fix to exact llama.cpp and ROCmFPX pins and records the deployment-evidence gap | Research input only; it explicitly did not inspect live binaries, listeners, firewalls, or processes |

**[VERIFIED]** The CachyLlama statement is a bounded source inspection, not a claim about every branch or a runtime exploit. Machine and mutation tests remain required.
