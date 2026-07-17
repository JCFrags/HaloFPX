# Failure, corruption, and recovery table

<span class="badge observed">OBSERVED</span> columns describe audited source behavior.  
<span class="badge recommended">HALOFPX RECOMMENDATION</span> columns are normative.

The complete machine-readable table is [`tables/failure-matrix.csv`](../tables/failure-matrix.csv).

## Core table

| Failure | CachyLLama observed | llama.cpp / comparators | HaloFPX required result |
|---|---|---|---|
| Entry absent | Miss | Read/lookup failure | `MISS_RECOMPUTE(NO_ENTRY)` |
| Namespace unauthorized | User path avoids cross-user fallback; unkeyed path hash | Salt/path policies vary | Deny before existence probe; miss/authorization result without leakage |
| Wrong magic | Checkpoint/system rejected | llama.cpp rejects | Miss; quarantine malformed object |
| Unsupported checkpoint version | Record version not consistently enforced | llama.cpp exact-version reject | Miss; optional copy migrator only |
| Truncated header | Scan/read fails | State read fails | Miss without allocation |
| Complete header, truncated payload | May be indexed; exact read later fails | State parser fails | Exact total-length check before segment allocation; miss |
| Same-length payload bit flip | Undetectable by format | Usually undetectable by outer file | Digest/AEAD failure; miss, quarantine, alert |
| Corrupted size field | Can drive allocation/read | Length decoder may fail | Checked arithmetic + hard cap + exact stat size before allocation |
| Token count above prefix array | Not authenticated/bounded in scan | Session token capacity checked | Miss before indexing |
| Compatibility mismatch | Miss | Architecture mismatch fails | Miss; object may remain valid for another compatible domain |
| Incomplete compatibility coverage | False-compatible hit possible | Architecture-only proof incomplete | Full canonical fingerprint required |
| Crash during final-name write | Visible partial file possible | Visible partial file possible | Only `.partial`; unreachable and cleanable |
| File sync failure | Return not used as commit failure | No explicit sync | Store fails; old manifest retained |
| Crash after object publish, before manifest | N/A | N/A | Safe orphan; no hit; later GC |
| Index/manifest truncation | Index rejected; scan; ID-reset risk | No catalog | Atomic generation replacement; invalid manifest is miss |
| Lost/corrupt catalog | Header scan partially rebuilds | Manual files require caller | Rebuild from validated manifests/objects; accounting is derived |
| Sequential ID collision | Possible after index loss | Filename chosen by caller | Content digest names; no sequential correctness dependency |
| Concurrent duplicate writers | Process mutex only | Caller/process policy | Single-flight; no-replace object publish; manifest CAS |
| Reader/evictor race | Process-local maps; pointer/race gaps | Caller | Open-FD snapshot; logical tombstone; grace/ref epoch |
| ENOSPC during write | Exact write fails and may unlink current file | Save fails | Abort temp; no manifest change; recompute continues |
| ENOSPC during manifest update | Index rewrite may fail after checkpoint exists | N/A | Old manifest remains; new object orphaned |
| Missing object referenced by manifest | N/A | N/A | Miss; quarantine manifest; catalog repair |
| Object filename/digest mismatch | No content-addressed name | N/A | Miss; quarantine and alert |
| Manifest HMAC key unavailable | N/A | N/A | Miss before following the object pointer |
| Manifest HMAC/signature failure | No keyed catalog auth | No keyed catalog auth | Miss; do not reveal/use object; security alert |
| Metadata digest mismatch | No digest | No digest | Miss before parsing semantic fields further |
| Segment digest mismatch | No digest | No digest | Miss; no engine call |
| AEAD key missing | No application encryption | No application encryption | Miss; never plaintext fallback |
| AEAD tag failure | N/A | N/A | Miss, quarantine, security alert |
| Decompression bomb/corrupt stream | No standardized codec | N/A | Ratio/output cap; miss |
| Required target segment missing | Load fails when target absent | State read fails | Miss |
| Optional draft segment invalid | Draft restore may be skipped/catch up | Usually not bundled | Target hit only if declared optional and independently authenticated |
| Recurrent/global state partial | Full state blob | Full state blob | All-or-nothing unless formal composability contract |
| Engine import rejects verified bytes | Target restore returns false | State read/import fails | Miss; staged context discarded |
| Generation rollback | No monotonic generation | No catalog generation | Miss; alert or signed/transactional rollback protection |
| Unknown critical flag/field | Native layout has no extensible critical flags | Exact version mostly | Miss |
| Migration interrupted | No migration engine | Exact version reject | Old manifest intact; new unreachable temp/orphan |
| Symlink/path substitution | Paths constructed normally; no hardened open contract documented | Filename validation but ordinary paths | Directory-FD resolution, no-follow, internally generated names |
| Process killed while holding lease | No process lease | N/A | Lease expires; incomplete writer never published |
| SSD read I/O error | Load false in several paths | Read fails | Miss; I/O reason metric; optional replica repair |
| Bit rot in unused object | Discovered only on later read | Later read | Background scrub verifies digest; quarantine before lookup |

## Recovery order

A HaloFPX startup or repair job should use this order:

1. **Do not trust mutable accounting.** Enumerate manifest files within authorized roots.
2. Bounded-parse and schema-check each manifest, resolve the authorized catalog key, and verify its HMAC/signature before trusting the object pointer. Missing keys and authentication failures are misses. Move malformed or unauthentic files to quarantine by atomic rename where safe.
3. Resolve authenticated referenced objects by digest; reject symlinks and non-canonical names.
4. Verify object size, name digest, header, metadata digest and required segment digests. Full payload scrubbing can be staged by priority, but an object is only an import candidate after lookup-time verification and never a public hit before engine import.
5. Reconstruct byte accounting and access metadata. Treat timestamps as eviction hints, never integrity.
6. Remove stale temporary files after a grace period.
7. Mark unreferenced objects as orphans, wait a separate grace period, then unlink and sync directories.
8. Reconcile lease records and release expired owners.
9. Resume service with invalid entries omitted. Recompute populates them normally.

## Quarantine policy

Quarantine is useful for forensics but must not become a second cache:

- move by digest/reason with restrictive permissions;
- cap bytes and retention;
- never retry quarantined data automatically;
- store no raw prompt identifiers in the quarantine filename;
- include reason code, source path hash, object digest, size and detection time in a separate audit event;
- permit immediate deletion instead when data-retention policy requires it.

## Partial-write proof obligation

A storage implementation passes the partial-write requirement only when a kill/power-loss test at every write boundary produces one of:

- old committed manifest and old valid object;
- no manifest for the new object;
- new committed manifest and complete valid object.

A visible manifest referencing a partial/missing object is permitted as a recoverable crash artifact only if readers deterministically return `MISS_RECOMPUTE`; the preferred commit order prevents this state.
