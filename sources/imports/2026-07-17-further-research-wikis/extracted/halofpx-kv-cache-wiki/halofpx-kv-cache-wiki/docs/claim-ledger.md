# Claim ledger

The machine-readable ledger is [`research/claim-ledger.csv`](../research/claim-ledger.csv). This page lists the load-bearing findings and their classification.

| Claim ID | Class | Claim | Primary source IDs | Confidence |
|---|---|---|---|---|
| CL-001 | OBSERVED | CachyLLama checkpoint v3 is a native struct followed by target/draft/spec blobs | C-KV-H, C-KV-CPP | High |
| CL-002 | OBSERVED | Checkpoint and index writers use final-name create/write/truncate rather than temp+rename | C-KV-CPP | High |
| CL-003 | OBSERVED | No checkpoint/index payload checksum or AEAD is present | C-KV-H, C-KV-CPP | High |
| CL-004 | INFERENCE | Same-length payload corruption cannot be detected by the outer CachyLLama format | C-KV-H, C-KV-CPP | High |
| CL-005 | OBSERVED | CachyLLama compatibility is FNV over model description and K/V types | C-PM | High |
| CL-006 | OBSERVED | The fingerprint does not append the build commit despite a nearby comment | C-PM | High |
| CL-007 | OBSERVED | Invalid/missing index resets `next_id` to 1 before/while checkpoint scan recovery | C-KV-CPP | High |
| CL-008 | INFERENCE | Without recomputing `next_id`, a later final-name store can overwrite an existing ID | C-KV-CPP | High |
| CL-009 | OBSERVED | Header scan does not authenticate body or prove total file length before indexing | C-KV-CPP | High |
| CL-010 | OBSERVED | Target engine restore failure returns false; optional draft failure can be tolerated | C-SRV | High |
| CL-011 | OBSERVED | Explicit user caches are routed under `u/{FNV(user_id)}` and excluded from anonymous global scans | C-USER, C-PM | High |
| CL-012 | OBSERVED | System-prompt cache uses a distinct native v1 format and FNV-keyed files | C-SYS-H, C-SYS-CPP | High |
| CL-013 | OBSERVED | Current llama.cpp full and sequence files have strict magic/version identifiers | L-H, L-CTX | High |
| CL-014 | OBSERVED | llama.cpp state save opens the destination in `wb` mode without an outer atomic/digest protocol | L-CTX, L-MMAP | High |
| CL-015 | OBSERVED | llama.cpp server slot files are manual filename-addressed sequence snapshots | L-SRV, L-README | High |
| CL-016 | OBSERVED | LMCache local disk initializes an empty metadata map and does not scan `.pt` files in the audited constructor | M-DISK | High |
| CL-017 | OBSERVED | LMCache local disk read does not check the `readinto` byte count | M-DISK | High |
| CL-018 | OBSERVED | SGLang HiCacheFile writes a unique temp and publishes with `os.replace` | S-STOR | High |
| CL-019 | OBSERVED | SGLang file pages are raw bytes without self-describing checksum/header | S-STOR | High |
| CL-020 | OBSERVED | SGLang page keys use parent-chained SHA-256 over normalized token bytes | S-UTIL, S-HASH | High |
| CL-021 | OBSERVED | SGLang LRU scans existing files and enforces bytes/free-space with reserve/commit/abort | S-LRU | High |
| CL-022 | OBSERVED | vLLM hashes parent, exact block tokens and extras including MM, LoRA, prompt embeddings and salt | V-DESIGN, V-KV | High |
| CL-023 | OBSERVED | vLLM APC is an in-memory block pool, not a persisted object protocol | V-DESIGN, V-KV | High |
| CL-024 | RECOMMENDED | HaloFPX objects should be immutable and content-addressed | This report | Normative |
| CL-025 | RECOMMENDED | Durable publication requires synced temp, verified immutable publish, synced directory and atomic manifest commit | This report | Normative |
| CL-026 | RECOMMENDED | Any invalid or unverifiable state must be `MISS_RECOMPUTE` with live context unchanged | This report | Normative |
| CL-027 | RECOMMENDED | Compatibility fingerprint must cover exact artifacts and execution semantics | This report | Normative |
| CL-028 | RECOMMENDED | Per-segment digest or AEAD must precede engine import | This report | Normative |
| CL-029 | MODELED | Full-state checkpoint write volume can consume hundreds of TB/year at high checkpoint rates | Endurance model | Scenario-dependent |
| CL-030 | RECOMMENDED | Byte quotas, chunk dedup and write admission are required endurance controls | This report | Normative |
| CL-031 | RECOMMENDED | HaloFPX manifest-to-object reachability must be keyed-authenticated by HMAC/signature or equivalent authenticated catalog | This report | Normative |
| CL-032 | RECOMMENDED | Offline validation may yield only an import candidate; public `HIT_VERIFIED` requires isolated engine import and atomic commit | This report | Normative |

## Interpretation rules

- “No checksum” means none was found in the audited outer file format; an opaque engine serializer may contain internal sentinels, but HaloFPX cannot depend on undocumented or incomplete inner checks.
- Unkeyed SHA-256 is collision-resistant corruption detection and content addressing, not proof that an authorized writer selected an object. The proposed catalog HMAC supplies that writer-authenticity binding.
- “Atomic” distinguishes atomic namespace visibility from power-loss durability. SGLang's `os.replace` improves visibility, but without file and directory sync it does not prove durable commit.
- “Persistent” distinguishes bytes surviving a process from entries being rediscoverable and safely reusable after restart. LMCache local files can survive while its audited local catalog starts empty.
- “Compatibility” means semantic proof, not merely an architecture label or likely shape match.
