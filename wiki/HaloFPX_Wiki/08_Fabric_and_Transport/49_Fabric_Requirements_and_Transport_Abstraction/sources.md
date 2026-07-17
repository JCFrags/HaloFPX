---
section_id: "49"
title: "Fabric Requirements - Sources"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ROCmFPX@a5605a7", "llama.cpp@788e07d", "linux@fce2dfa"]
  software_versions: []
  hardware_revisions: []
related_sections: ["50", "51", "52", "53"]
---

# Sources

| ID | Primary source and pinned revision | Claims supported | Limitations/conflicts |
|---|---|---|---|
| S49-01 | [ROCmFPX tree `a5605a72768c6562241b248e268e33dc92787394`](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394), commit 2026-07-16; accessed 2026-07-16 | candidate base identity | no target-machine result |
| S49-02 | [llama.cpp `ggml-rpc.cpp` at `788e07dc91d266ad3162a1ce9037665656269689`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/src/ggml-rpc/ggml-rpc.cpp), commit 2026-07-17; accessed 2026-07-16 local time | framing, handshake, synchronous operations, graph/hash behavior | fast-moving experimental code; source date is ahead in UTC+02 |
| S49-03 | [Linux `stream.c` at `fce2dfa773ced15f27dd27cd0b482a7473cdcf2a`](https://github.com/torvalds/linux/blob/fce2dfa773ced15f27dd27cd0b482a7473cdcf2a/drivers/thunderbolt/stream.c), Linux 7.2-rc3-era master, commit 2026-07-16; accessed 2026-07-16 | USB4STREAM semantics and limits | no HaloFPX measurement |
| S49-04 | [RFC 9621, Architecture and Requirements for Transport Services](https://www.rfc-editor.org/rfc/rfc9621.html), November 2024; accessed 2026-07-16 | transport-property vocabulary and racing/multipath considerations | architecture guidance, not implementation |
| S49-05 | [RFC 8684, Multipath TCP](https://www.rfc-editor.org/rfc/rfc8684.html), March 2020; accessed 2026-07-16 | connection/subflow sequencing, failover and duplicate principles | TCP-specific |
| S49-06 | Local `C:\Users\britt\Documents\Agent_Harness\guide\architecture.md`, read 2026-07-16 | evidence promotion and reversible recommendations | governance only |

## Provenance note

Repository facts were checked in local shallow clones at the full hashes above. Internet sources and current branch heads were accessed 2026-07-16. No machine measurements were available.

