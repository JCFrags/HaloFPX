---
section_id: "71"
title: "Security Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "llama.cpp", "CachyLlama"]
  software_versions: ["llama.cpp 788e07dc91d266ad3162a1ce9037665656269689", "CachyLlama 6be745998f568e379ea197fcf827baec73ff9940"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["69", "70", "72"]
---

# Facts and constraints

## Source-backed facts

- **[VERIFIED]** Upstream llama.cpp does not claim hostile-input or multi-tenant isolation; its security policy delegates isolation to the operator and warns against untrusted network use [S71-01].
- **[VERIFIED]** The pinned llama.cpp RPC README describes the facility as proof-of-concept, fragile, and insecure [S71-02].
- **[VERIFIED]** NIST SP 800-207 rejects implicit trust based solely on network location or physical location [S71-03].
- **[VERIFIED]** TLS 1.3 defines authenticated key exchange and record protection; secure deployment still depends on identity validation and key handling [S71-04].
- **[VERIFIED]** systemd supports credentials delivered outside ordinary command-line arguments and provides filesystem/process/device sandbox controls [S71-05].
- **[VERIFIED]** SLSA 1.2 defines provenance and verification concepts for software supply chains; NIST SSDF 1.1 defines secure development practices [S71-06, S71-07].
- **[VERIFIED]** At CachyLlama commit `6be7459`, the inspected cache format contains magic/version and a compatibility hash, writes a truncated mode-0644 destination, and uses FNV-derived token hashing; inspection did not identify cryptographic blob authentication or an atomic temporary-file rename [S71-08].
- **[VERIFIED]** GitHub advisory GHSA-j8rj-fmpv-wcxw describes an unauthenticated llama.cpp RPC remote-code-execution path and, as checked 2026-07-17, names no patched version. Pinned llama.cpp source `788e07dc91d266ad3162a1ce9037665656269689` contains source fix commit `ba38f3becce7d1283585c73d796eb47d72bbbd30` and the guards that block the advisory's documented malformed tensor state [S71-09, S71-10]. This is source applicability, not a patched-release or deployed-runtime claim.
- **[OPEN]** The installed executables, loaded libraries, build options, listener addresses, firewall paths, and process privileges on both intended nodes have not been mapped to the reviewed fixed source [S71-10].

## Constraints and limits

- **[INFERENCE]** Cache objects can reveal prompt/token prefix and model-state information; they are privacy-sensitive even when users never see the raw format.
- **[RECOMMENDATION]** Cache compatibility hashes are not security integrity proofs. HaloKV must cryptographically validate identity/content and reject or recompute invalid state.
- **[RECOMMENDATION]** Shared GPU/process execution is not a demonstrated hostile-tenant security boundary. Hostile tenants require stronger process/VM/device isolation than this design currently proves.
- **[RECOMMENDATION]** Never expose unmodified llama.cpp RPC to an untrusted interface. A product transport must add authenticated, integrity-protected protocol framing or replace it.
- **[RECOMMENDATION]** Keep RPC disabled unless both peers' executable and loaded-library provenance prove the reviewed fixed source and the listener/exposure/least-privilege gates pass. A source pin or release name alone is insufficient, and no patched llama.cpp release is claimed here.
