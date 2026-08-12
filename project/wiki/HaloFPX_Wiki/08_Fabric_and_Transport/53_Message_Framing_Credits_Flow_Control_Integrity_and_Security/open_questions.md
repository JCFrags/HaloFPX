---
section_id: "53"
title: "Framed transport open questions"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: ["HaloFPX wire protocol v1 proposal"]
  hardware_revisions: ["Two-host Strix Halo USB4 cluster; exact revisions OPEN"]
related_sections: ["48", "50", "52", "54", "55", "71", "75", "80"]
---

# Open questions

| ID | Question | Required evidence | Dependency |
|---|---|---|---|
| S53-OQ-001 | **[OPEN]** What exact `max_record`, `max_message`, session/per-rail receive budgets, descriptor caps and credit windows avoid deadlock and bound memory for real activation shapes? | S53-EXP-002/003 plus authorized activation trace. | 34, 43, 54 |
| S53-OQ-002 | **[OPEN]** Should production retain delta credits or version a monotonic absolute-limit credit scheme? | Property/model checking and duplicate/loss/reconnect complexity comparison. | 48 |
| S53-OQ-003 | **[OPEN]** Are implicit per-rail record counters sufficiently observable for diagnosis, or should v2 add an explicit `record_seq` header field? | EXP-001/004 fault analysis and wire-review outcome. | 48, 69 |
| S53-OQ-004 | **[OPEN]** Which handshake, progress, heartbeat, reassembly, drain and close deadlines meet liveness without false failure under real workload tails? | Retained latency distributions from EXP-002/003/006. | 55, 73, 75 |
| S53-OQ-005 | **[OPEN]** For which model-bearing deployments, if any, may Section 71 accept `AUTH_INTEGRITY` without payload confidentiality? Bulk authentication, peer identity, freshness, and replay protection are not optional. | Section-71 threat review and explicit deployment boundary. | 71 |
| S53-OQ-006 | **[OPEN]** Is ChaCha20-Poly1305 the right v1 AEAD on Zen 5, or should a separately versioned AES-GCM profile be admitted? | Independent crypto implementation review and matched CPU/latency measurements. | 17, 55, 71 |
| S53-OQ-007 | **[OPEN]** What priority burst/fairness rule prevents bulk starvation while keeping credits, cancellation and heartbeat live? | Adversarial saturation results from EXP-002/003. | 46, 55 |
| S53-OQ-008 | **[OPEN]** Which upper-layer operations are retry-safe, cancel-safe, or committed, and what ACK constitutes a durable logical boundary? | Rank ownership/state-machine specification and fault tests. | 39, 45, 48, 58, 61 |
| S53-OQ-009 | **[OPEN]** Does USB4STREAM pass the accepted standalone correctness/performance gates on both rails without degrading USB4NET recovery? | EXP-003/004/006 with section-50/55 receipts. | 50, 55, 75 |
| S53-OQ-010 | **[OPEN]** How should compatibility and feature registries be allocated and reviewed to avoid conflicting private extensions? | Wire registry and governance decision before a second implementation. | 01, 03, 86 |

## Internet/source follow-up

- **[RECOMMENDATION]** Re-diff the exact Linux `stream.c` and testing ABI at stable-kernel selection; do not assume the v7.2-rc2 behavior remained unchanged.
- **[RECOMMENDATION]** Before implementation freeze, obtain cryptographic review of transcript canonicalization, HKDF labels, nonce construction, usage limits, tag handling and PSK lifecycle.
- **[RECOMMENDATION]** Check current errata/status for all cited RFCs and pin the chosen crypto library release/source commit.

## Decision blockers

- USB4STREAM remains optional/default-off until OQ-009 resolves positively.
- AEAD deployment policy waits for OQ-005/006 and section 71.
- RPC retry/cancel semantics wait for OQ-008.
- Wire v1 cannot be marked verified until EXP-001 and an independent implementation interoperate.
