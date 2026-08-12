---
section_id: "60"
title: "Prefix sharing facts and safety constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloKV design", "fewtarius/CachyLLama"]
  software_versions: ["CachyLLama 6be745998f568e379ea197fcf827baec73ff9940", "HaloKV proposal v0"]
  hardware_revisions: ["dual gfx1151 Strix Halo; exact hosts pending"]
related_sections: ["56", "57", "59", "61", "64"]
---

# Prefix sharing facts and safety constraints

## Pinned predecessor

- **[VERIFIED]** `kv_ssd_system_cache` stores serialized attention+recurrent state in `sys-{hash}.bin`, with token count, a fixed maximum token prefix, compatibility hash and last-used data. [S60-01]
- **[VERIFIED]** Lookup hashes all supplied tokens with FNV-1a, then verifies count and the stored prefix; source comments acknowledge it cannot fully verify beyond that prefix. [S60-01]
- **[VERIFIED]** System-boundary discovery decodes tokens and searches for user/human role/end markers with template-oriented heuristics. [S60-01]
- **[VERIFIED]** Explicit `llama_user_id` routes to a hashed `u/` namespace and disables fuzzy cross-user continuation; absent IDs retain content-derived continuation behavior. [S60-02][S60-03]

## Safe-sharing predicates

All must be true:

1. **[RECOMMENDATION]** Exact compatibility root matches: model bytes/metadata, tokenizer, rendered chat/tool template, RoPE/numeric/KV/runtime schema, rank/topology and commits per section 57.
2. Exact rendered token sequence and token count are verified using collision-resistant digest plus byte-for-byte token comparison before restore.
3. Prefix boundary is supplied by the rendering/orchestration layer and names its semantic class; no cache parser guesses it.
4. Required attention, recurrent, MTP/speculative and position state coverage is valid per section 61.
5. Sharing policy authorizes the producer/consumer scopes and checkpoint has not expired/revoked.
6. Every referenced page/object passes section 59/63 integrity and commit checks.

## Isolation classes

| Class | Default scope | Examples | Sharing rule |
|---|---|---|---|
| Public immutable | deployment/model-wide | published static system policy with no secrets | May share only from operator-provisioned/verified origin. |
| Deployment private | service-wide | tool schemas or internal instructions | Share within one configured trust domain; never cross deployment. |
| Tenant | authenticated tenant | tenant policy, private tools | Same tenant only. |
| User/session | principal/session | personal context, conversation history | Same authorized principal/session; explicit continuation capability. |
| Non-shareable | request only | secrets, retrieved private data, untrusted tool results | No dedup visibility outside request; delete by retention policy. |

**[RECOMMENDATION]** Physical deduplication across scopes is disabled in v0 unless encryption, reference/deletion accounting and side-channel analysis prove it safe. Identical plaintext does not erase ownership/privacy obligations.

## Poisoning and leakage constraints

- Untrusted requests cannot publish into global/public prefix namespaces.
- Hash/digest equality is not authorization; tenant/session checks precede lookup result disclosure.
- Miss/hit timing, quota and eviction can reveal presence. Normalize external responses and rate-limit probing; record internal reasons privately.
- Corruption, revoked origin, expired policy or ambiguous identity is a miss/recompute, never fallback to a weaker scope.

