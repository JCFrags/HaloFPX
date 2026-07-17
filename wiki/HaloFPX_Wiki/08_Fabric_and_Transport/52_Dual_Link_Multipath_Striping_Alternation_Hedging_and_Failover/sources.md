---
section_id: "52"
title: "Dual-Link Multipath - Sources"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["linux@fce2dfa"]
  software_versions: ["MPTCP v1", "QUIC v1"]
  hardware_revisions: []
related_sections: ["49", "50", "53", "55"]
---

# Sources

| ID | Primary source and revision/date | Claims supported | Limitations |
|---|---|---|---|
| S52-01 | [RFC 8684: TCP Extensions for Multipath Operation with Multiple Addresses](https://www.rfc-editor.org/rfc/rfc8684.html), March 2020; accessed 2026-07-16 | subflows, data sequence mapping, reorder, duplicate/reinjection, fallback | MPTCP/TCP semantics, not direct stream |
| S52-02 | [Linux MPTCP documentation](https://docs.kernel.org/networking/mptcp.html), rolling docs accessed 2026-07-16 | path manager, packet scheduler, socket API | rolling page; exact deployed kernel must be pinned |
| S52-03 | [Linux MPTCP sysctl documentation](https://docs.kernel.org/networking/mptcp-sysctl.html), rolling docs accessed 2026-07-16 | stale subflow detection and scheduler exclusion | implementation controls can vary by kernel |
| S52-04 | [RFC 9000: QUIC](https://www.rfc-editor.org/rfc/rfc9000.html), May 2021; accessed 2026-07-16 | packet-number monotonicity, duplicate suppression | design analogue, not chosen carrier |
| S52-05 | [RFC 9002: QUIC Loss Detection and Congestion Control](https://www.rfc-editor.org/rfc/rfc9002.html), May 2021; accessed 2026-07-16 | explicit packet-reordering treatment | Internet congestion assumptions differ from USB4 peer link |
| S52-06 | [Linux source tree `fce2dfa773ced15f27dd27cd0b482a7473cdcf2a`](https://github.com/torvalds/linux/tree/fce2dfa773ced15f27dd27cd0b482a7473cdcf2a/net/mptcp), commit 2026-07-16; accessed 2026-07-16 | pinned implementation authority for later source audit | no target configuration/result |
| S52-07 | Local Agent Harness `guide/architecture.md`, read 2026-07-16 | evidence promotion and reversible choice | governance only |

No numeric scheduler threshold is sourced because HaloFPX must derive it from its own matched measurements.

