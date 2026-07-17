---
section_id: "64"
title: "Eviction, quota, and privacy design"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["59", "63", "71"]
---

# Design implications

**[RECOMMENDATION]** GC marks from committed manifests, active leases, pinned roots, exports, and retained generations; only unmarked objects older than a grace epoch may be swept. Recheck reachability immediately before deletion.

## Quota hierarchy

`filesystem reserve -> global HaloKV budget -> model budget -> tenant/user budget -> session/pinned budget`. Charge physical unique bytes and separately report logical referenced bytes. Shared objects need an explicit accounting rule; never use shared content across identities unless section 60 authorizes it.

**[RECOMMENDATION]** Eviction score combines reachability, active/pinned status, last access, reuse count, restore cost, size, user/model quota debt, and write-amplification cost. Fairness caps prevent one tenant or model from consuming all storage.

**[RECOMMENDATION]** Under disk pressure: stop speculative writes, reduce retention, evict safe cold data, reject new cache stores, and continue inference without cache. Reserve space for metadata/recovery and expose hysteretic pressure states.

## Privacy

**[RECOMMENDATION]** Use authenticated principal IDs mapped to opaque keyed namespace identifiers, restrictive directory permissions, encryption at rest where required, minimal metadata/logging, and auditable deletion. A user-facing delete can make objects unreachable and destroy per-tenant keys; physical-media sanitization is a separate administrative lifecycle action.

