---
section_id: "56"
title: "CachyLLama cache facts and behavior map"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["fewtarius/CachyLLama", "ggml-org/llama.cpp", "charlie12345/ROCmFPX"]
  software_versions: ["CachyLLama 6be745998f568e379ea197fcf827baec73ff9940"]
  hardware_revisions: ["dual gfx1151 Strix Halo; exact hosts pending inventory"]
related_sections: ["57", "58", "59", "60", "61", "62", "63", "64"]
---

# CachyLLama cache facts and behavior map

| Required behavior | Pinned-source evidence | Port disposition |
|---|---|---|
| Conversation checkpoints | **[VERIFIED]** `kv-ssd-cache` v3 writes per-checkpoint `ckpt-{id}.bin` records containing target state plus optional draft/spec blobs; an index holds next ID and a 64-bit compatibility hash. [S56-02] | Retain checkpoint concept; redesign format/publication/checksums. |
| Hot/warm/cold tiers | **[VERIFIED]** Hot and warm maps retain serialized blobs in RAM; LRU demotion removes warm bytes while the file remains cold on disk. Cold-count ring eviction sorts by creation turn. [S56-02] | Retain tier abstraction; redesign budgets/quotas and measure duplicate page-cache memory. |
| Slot lookup | **[VERIFIED]** The cache indexes latest checkpoint per slot and within-conversation lookup chooses a token-prefix match subject to compatibility checks. [S56-02] | Reject slot ID as durable identity; use checkpoint/prefix IDs independent of runtime allocation. |
| Continuation discovery | **[VERIFIED]** `kv_ssd_find_continuation` scans conversation directories and uses stored token prefixes plus compatibility hash for fuzzy overlap. [S56-02] | Redesign as exact verified longest-prefix lookup; fuzzy global scanning is unsafe/expensive. |
| System-prompt cache | **[VERIFIED]** A global pool stores attention+recurrent bytes under a 64-bit FNV-1a token hash, verifies stored token count and only a bounded token prefix, and expires/evicts entries. [S56-03] | Retain reusable-prefix goal; reject weak/bounded-only verification and heuristic boundary authority. |
| User namespaces | **[VERIFIED]** Explicit `llama_user_id` traffic is routed under `u/{FNV(user_id)}`; source design disables cross-user continuation lookup and adds per-user slot caps/affinity. Anonymous requests retain content-derived routing. [S56-04][S56-05] | Retain explicit tenant boundary; redesign opaque keyed tenant IDs, authorization, quotas and anonymous policy. |
| Retention | **[VERIFIED]** Conversation cold-count and system-cache max-entry/unused-age policies delete files. [S56-02][S56-03] | Retain policy hooks; redesign reference-aware GC, auditability and privacy deletion guarantees. |
| Prefetch | **[VERIFIED]** Cold checkpoint reads issue Linux `posix_fadvise(...WILLNEED)` or platform readahead before synchronous reads. [S56-02] | Retain hints as fallback; add measured async I/O/tier planner in section 62. |
| Compatibility | **[VERIFIED]** Stored 64-bit `compat_hash` is checked before load, but the source comments describe it only as model config/architecture/dimensions/cache types. [S56-02][S56-03] | Reject as sole gate; replace with section 57 manifest/fingerprint. |
| MTP/speculative blobs | **[VERIFIED]** v3 record includes separately sized draft/MTP and speculative implementation blobs, and the server integration serializes/restores them when present. [S56-02][S56-06] | Retain extensible typed components; require independent schemas and all-or-recompute rules. |
| Recurrent state | **[VERIFIED]** Pinned CachyLLama extends llama state handling for attention-only removal, recurrent rollback snapshots and partial/on-device sequence state flags. [S56-06][S56-07] | Retain semantic separation only after model-specific correctness tests; section 61 is authoritative. |

## Critical source limitations

- **[VERIFIED]** FNV-1a is used for token, conversation, system-prefix and user-directory identities. [S56-02, S56-03, S56-04] It is not a collision-resistant digest.
- **[VERIFIED]** System files call `fsync(fd)` but are written directly to their final pathname; the inspected path does not establish temp-write + atomic rename + parent-directory sync. [S56-03]
- **[VERIFIED]** System-boundary detection decodes tokens and heuristically searches for user/human role markers and end markers. [S56-03] **[INFERENCE]** It cannot be a universal semantic authority across arbitrary templates.
- **[OPEN]** The README reports 1–4 second restores and 17,800+ tokens, but no raw environment-matched evidence was found in the files reviewed; these are repository claims, not HaloFPX measurements. [S56-01]
