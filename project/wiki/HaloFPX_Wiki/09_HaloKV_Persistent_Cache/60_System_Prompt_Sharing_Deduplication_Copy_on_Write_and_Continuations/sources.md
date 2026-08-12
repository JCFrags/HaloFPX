---
section_id: "60"
title: "Prefix sharing primary sources"
status: "draft"
last_verified: "2026-07-17"
applies_to:
  repositories: ["HaloKV design", "fewtarius/CachyLLama", "ggml-org/llama.cpp"]
  software_versions: ["CachyLLama 6be745998f568e379ea197fcf827baec73ff9940", "HaloKV proposal v0"]
  hardware_revisions: ["dual gfx1151 Strix Halo; exact hosts pending"]
related_sections: ["56", "57", "58", "59", "61", "63", "64"]
---

# Prefix sharing primary sources

Accessed 2026-07-16.

| ID | Source | Supports | Limitations |
|---|---|---|---|
| S60-01 | CachyLLama [`kv-ssd-system-cache.h/.cpp`](https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940/common), commit `6be74599` | Global prefix pool, FNV key, bounded token verification, expiry and boundary heuristic | No universal safety/correctness proof. |
| S60-02 | CachyLLama [`user-isolation-design.md`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/docs/development/user-isolation-design.md), same commit | User namespace and continuation privacy intent | Design doc; caller authentication is external. |
| S60-03 | CachyLLama [`server-context-page-manager.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-page-manager.cpp) and [`kv-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp) | Explicit-user routing, prefix/continuation lookup | Slot/content-derived design is not HaloKV authorization. |
| S60-04 | NIST, [FIPS 180-4 Secure Hash Standard](https://csrc.nist.gov/pubs/fips/180-4/upd1/final), Aug 2015 | SHA-256 digest definition used by section 57 | Hash equality does not provide authorization. |
| S60-05 | IETF, [RFC 2104 HMAC](https://www.rfc-editor.org/rfc/rfc2104), Feb 1997 | Candidate authenticated continuation identifier construction | Key lifecycle/protocol still require design. |
| S60-06 | NIST, [SP 800-207 Zero Trust Architecture](https://csrc.nist.gov/pubs/sp/800/207/final), Aug 2020 | Resource-access authorization principles | General architecture guidance, not LLM-cache semantics. |
| S60-07 | ggml-org/llama.cpp [`llama-chat.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-chat.cpp) and tokenizer/model source at commit `788e07dc` | Rendered chat/token semantics are source/version dependent | Does not define HaloKV boundaries. |
| S60-08 | CachyLLama [`include/llama.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/include/llama.h) and [`src/llama-context.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/src/llama-context.cpp), commit `6be745998f568e379ea197fcf827baec73ff9940`, accessed 2026-07-17 | Attention/recurrent/partial sequence state surfaces | Fork-specific API; sharing correctness remains machine/model-specific. |

## Evidence boundary

All HaloKV policy, content-key, copy-on-write and continuation-capability structures are **[RECOMMENDATION]** designs. Sources establish predecessor facts and security primitives, not implementation validation.
