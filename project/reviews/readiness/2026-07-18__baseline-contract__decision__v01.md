---
type: decision
status: accepted-for-feature-off-baseline
date: 2026-07-18
decisions: [OPEN-BASE-01, OPEN-API-01]
---

# Initial feature-off baseline contract

The local fork may begin with all HaloFPX additions disabled. The first milestone
must preserve model loading, CLI generation, `/health`, `/v1/models`,
`/v1/chat/completions`, the existing `/completion` compatibility route, and
diagnostic `/props` and `/slots` behavior where the pinned base exposes them.

Non-loopback HTTP requires authentication or a protected proxy. RPC remains off
by default in release configuration. Existing ephemeral prompt/cache behavior,
quantization, MTP/speculation, HIP/Vulkan selection, and feature-off RPC behavior
must be characterized before donor-derived capability work.

This closes `OPEN-BASE-01` and `OPEN-API-01` only for the initial local
implementation contract. It does not promise external API compatibility or close
G3; executable fixtures, complete endpoint schemas, security hardening, and
matched feature-off evidence remain milestone work.
