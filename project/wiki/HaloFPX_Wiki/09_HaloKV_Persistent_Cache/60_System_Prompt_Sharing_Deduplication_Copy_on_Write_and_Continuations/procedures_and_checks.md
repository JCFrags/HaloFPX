---
section_id: "60"
title: "Prefix sharing security and correctness checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["HaloKV design", "fewtarius/CachyLLama"]
  software_versions: ["CachyLLama 6be745998f568e379ea197fcf827baec73ff9940", "HaloKV proposal v0"]
  hardware_revisions: ["dual gfx1151 Strix Halo; exact hosts pending"]
related_sections: ["57", "58", "59", "61", "63", "64"]
---

# Prefix sharing security and correctness checks

## Safety and data boundary

Use synthetic non-sensitive tenants/prompts and a disposable store/service instance with exact resolved root/store UUID, resource ceilings, preserved recovery/evidence access, stop conditions, cleanup, and a receipt. Refuse production identities, real private prompts, cache/model/workspace/boot paths, backups, and sole evidence copies. Fixture mutation needs no root; process, filesystem, kernel, device, or timing-fault instrumentation must declare privileges and use the Section 80 authorization boundary where disruptive.

## Pinned source audit

```bash
git clone https://github.com/fewtarius/CachyLLama.git
git -C CachyLLama checkout 6be745998f568e379ea197fcf827baec73ff9940
rg -n "hash_tokens|token_prefix|find_system_boundary|compat_hash|retention" \
  CachyLLama/common/kv-ssd-system-cache.*
rg -n "llama_user_id|namespace_prefix|find_continuation|user_id" \
  CachyLLama/common CachyLLama/tools/server CachyLLama/docs/development/user-isolation-design.md
```

No root access is required. Preserve blob hashes and compare design claims with executable code/tests.

## Correctness matrix

For every supported model/template/tool schema and state type:

1. Render exact token streams and explicit boundaries; save the prefix.
2. Restore in same session, another authorized session and every prohibited scope.
3. Replay divergent suffixes/branches; compare deterministic logits/output with full recomputation and prove suffix-only execution.
4. Change one fingerprint field, boundary token, token ID, component, rank or topology; require diagnostic miss.
5. Test divergence at page start/middle/end and verify parent bytes/digest never change.
6. Test recurrent/hybrid/MTP/spec state; missing required component must miss.

## Adversarial matrix

- Attempt FNV/digest collision fixtures, truncated stored-token material, alternate tokenization with same text, template/Unicode/tool-order changes and forged metadata.
- Have tenant B guess tenant A content, object/checkpoint/session IDs and observe timing/status/quotas. External responses must not disclose existence or internal miss reason.
- Attempt publishing untrusted request/tool content to public/deployment scope; require authorization failure.
- Revoke/expire/delete while reads and branches are active; test reference and generation safety.
- Corrupt or swap shared pages/policy records; require miss/quarantine and no consumer publication.

## Performance/privacy evidence

Measure full recompute versus shared-prefix restore for realistic token/state sizes, cold/warm/hot storage, concurrent branches and both ranks. Record TTFT/p50/p99, bytes read, evaluation saved, DRAM/page cache/NVMe, lookup complexity and profiler overhead. Separately measure hit/miss timing distinguishability and rate-limit behavior.

## Acceptance

- Full token verification and complete section 57 identity.
- Explicit policy authorization before existence disclosure.
- No cross-tenant/session tail access.
- Immutable parents and correct copy-on-write tails.
- Revocation/expiry enforced at lookup.
- Corruption always miss/recompute.
- No measurement claim without raw environment-bound evidence.
