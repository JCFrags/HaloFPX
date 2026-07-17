---
section_id: "65"
title: "Administration and migration design"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: []
related_sections: ["57", "59", "63", "64"]
---

# Design implications

## Tool contract

| Command | Default behavior |
|---|---|
| list/show | metadata only; redact token/content/user identifiers |
| validate | read-only schema/size/digest/reachability check; JSON report |
| export | consistent committed generation, checksummed bundle, optional encryption |
| import | dry-run compatibility/quota/collision report; stage then atomic publish |
| migrate | source preserved; transform to new namespace; verify before switch |
| compact | dry-run byte/rewrite estimate; foreground throttle and rollback |
| delete | show reachability/tenant scope; tombstone/unreachability then GC |
| benchmark | isolated output path; never overwrite production cache |

**[RECOMMENDATION]** Every mutating command requires explicit root path, expected store UUID, namespace, operation ID, dry-run by default, and machine-readable receipt. Refuse symlinks/path escapes and live-object mutation.

## Migration

**[RECOMMENDATION]** Export bundles bind source format/producer, compatibility fingerprint, object digests, user namespace, generation graph, and encryption metadata. Import never bypasses compatibility checks; unsupported streams remain preserved but unavailable, or the import fails.

**[RECOMMENDATION]** Compare projected annual host writes and rated TBW/percentage-used trend with safety margin. Endurance alarms should first reduce speculative checkpoint frequency/compaction, not affect inference correctness.

