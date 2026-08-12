---
section_id: "60"
title: "Prefix sharing open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloKV design", "fewtarius/CachyLLama"]
  software_versions: ["HaloKV proposal v0"]
  hardware_revisions: ["dual gfx1151 Strix Halo; exact hosts pending"]
related_sections: ["57", "58", "59", "61", "63", "64"]
---

# Prefix sharing open questions

| ID | Question | Evidence required |
|---|---|---|
| O60-01 | **[OPEN]** Which rendered system/tool boundaries are explicitly available from each server/template path? | Source/API inventory and token vectors. |
| O60-02 | **[OPEN]** Which prefixes are operator-public, deployment-private, tenant, session or non-shareable? | Product/privacy threat decision. |
| O60-03 | **[OPEN]** Are attention/recurrent/MTP/spec components reusable at identical boundaries for every supported model? | Section 61 deterministic tests. |
| O60-04 | **[OPEN]** Must exact token arrays be stored, or can immutable token pages provide full verification efficiently? | Format/lookup benchmarks and threat analysis. |
| O60-05 | **[OPEN]** What authenticated principal/session system binds continuation capabilities? | Serving architecture and authorization tests. |
| O60-06 | **[OPEN]** Is anonymous prefix reuse disabled entirely or limited to operator-public objects? | Explicit privacy decision; safest default is public-only. |
| O60-07 | **[OPEN]** Can hit/miss timing or eviction/quota effects reveal private prefix presence? | Cross-tenant side-channel measurements. |
| O60-08 | **[OPEN]** What expiry/revocation epoch and deletion SLA apply per sharing class? | Section 64 policy and concurrent deletion tests. |
| O60-09 | **[OPEN]** Is physical cross-tenant dedup ever worth encryption/reference/deletion complexity? | Security review and measured capacity benefit. |
| O60-10 | **[OPEN]** What branch/page granularity minimizes copy-on-write amplification? | Section 59/65 workload measurements. |
| O60-11 | **[OPEN]** How are global prefixes materialized independently for both rank-local topologies? | Section 58 two-rank restore tests. |
| O60-12 | **[OPEN]** How are compromised/poisoned operator prefixes revoked and audited? | Privileged publication, signing and incident drill. |

Internet follow-up must track exact upstream template/tokenizer/state API changes at chosen commits. No general textual prompt equality is accepted as cache compatibility.

