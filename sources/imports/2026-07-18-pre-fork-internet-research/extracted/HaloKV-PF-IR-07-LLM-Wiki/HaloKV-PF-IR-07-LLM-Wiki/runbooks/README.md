# PF-IR-07 operational runbooks

These runbooks are implementation templates. Replace placeholders with the exact
service, filesystem, KMS, cryptsetup, systemd, and media-provider procedures in
the deployment. Command success is never by itself evidence of cryptographic
erase or media sanitization.

| Runbook | Purpose |
|---|---|
| [RB-01](RB-01-boot-unlock.md) | Cold boot, lower-layer unlock, key authorization, and cache readiness |
| [RB-02](RB-02-epoch-rotation-and-revocation.md) | Routine and emergency object-key epoch lifecycle |
| [RB-03](RB-03-nonce-state-loss-and-crash.md) | Nonce allocator uncertainty, clone, restore, and crash recovery |
| [RB-04](RB-04-backup-and-quarantine-restore.md) | Coherent backup exception and non-serving restore |
| [RB-05](RB-05-deletion-ce-and-media.md) | Logical deletion, CE decision, and physical-media workflow |
| [RB-06](RB-06-export-and-scope-transition.md) | Authorized export to a different principal/sharing domain |
| [RB-07](RB-07-quarantine-and-key-incident.md) | Object/key/publisher incident containment and recovery |
| [RB-08](RB-08-fscrypt-luks-busy-drain.md) | Busy files, mappings, process keys, and shutdown verification |

Every runbook preserves the caller-facing invariant: any invalid or unavailable
cache object is `MISS_RECOMPUTE`.
