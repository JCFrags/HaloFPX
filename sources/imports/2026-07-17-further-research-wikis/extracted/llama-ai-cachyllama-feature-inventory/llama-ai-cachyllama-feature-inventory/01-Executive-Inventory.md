# Executive inventory

## Disposition

| Decision | Count | Interpretation |
|---|---:|---|
| RETAIN | 68 | Preserve the capability or target-native implementation with limited reconciliation. |
| REDESIGN | 39 | Keep the outcome but change architecture, durability, security, licensing, or integration. |
| REJECT | 13 | Do not import the pinned implementation or behavior. |

## Category coverage

| Category | Features | Retain | Redesign | Reject |
|---|---:|---:|---:|---:|
| Persistent SSD-backed KV cache | 15 | 7 | 6 | 2 |
| System-prompt caching | 5 | 2 | 2 | 1 |
| Prefix matching and session continuation | 6 | 4 | 2 | 0 |
| Checkpoint restore and hybrid state | 9 | 9 | 0 | 0 |
| Slot affinity and user isolation | 12 | 2 | 6 | 4 |
| Cache administration | 9 | 2 | 6 | 1 |
| Model lifecycle | 10 | 8 | 2 | 0 |
| Server APIs | 22 | 18 | 2 | 2 |
| Observability | 6 | 3 | 2 | 1 |
| Deployment | 9 | 1 | 6 | 2 |
| Strix Halo optimizations | 11 | 6 | 5 | 0 |
| ROCmFPX target baseline | 6 | 6 | 0 | 0 |

## Primary architectural decision

**Extend ROCmFPX's existing prompt-cache engine.** At `a5605a72768c6562241b248e268e33dc92787394`, ROCmFPX already has owner-only per-run directories, temporary state files, target/draft pair validation, atomic rename, directory synchronization, LRU accounting, a save-failure circuit breaker, stale-run cleanup, and focused failure tests. Its current limitation is deliberate run-scoped cleanup. CachyLlama contributes the desired restart-persistent semantics, richer checkpoint metadata, system-prefix catalog, continuation search, and user-aware scheduling—but its final-name writes, permissions, split lifecycle maps, and unauthenticated user namespace should not displace the target's stronger storage base. Evidence: [E-204](20-Evidence-Index.md#e-204)–[E-210](20-Evidence-Index.md#e-210), [E-031](20-Evidence-Index.md#e-031)–[E-041](20-Evidence-Index.md#e-041).

## Recommended target shape

1. Keep `run-scoped` as the safe default and add an explicit `persistent` mode.
2. Commit checkpoint payloads and a versioned manifest atomically.
3. Namespace by exact model/state ABI and an authenticated tenant principal.
4. Preserve target/draft/stateful-MTP exact-boundary validation.
5. Derive system-prefix boundaries from parsed message/template structure.
6. Consolidate cache instances into one registry for aging, quotas, statistics, and administration.
7. Add restart, corruption, concurrent-instance, tenant-isolation, and upgrade tests.
8. Reconcile backend kernels only after source-level diffing against ROCmFPX's newer Strix/MTP work.

## Legal integration boundary

The CachyLlama component is MIT, while the parent launcher/build/deployment repository is GPL-3.0 and identifies a separate documentation content license. Direct source incorporation should focus on MIT component units. Useful parent behavior should be clean-room reimplemented in the MIT target or retained as a separate GPL deployment layer. See [Dependencies and licenses](16-Dependencies-and-Licenses.md).

## Most material maturity findings

- Anonymous per-user throttling is described but not implemented for empty identities.
- User cache maps are omitted from statistics and turn-completion aging.
- The standalone `common/kv_page_manager.*` path is not linked into the common library.
- System-prompt boundary detection is token-text heuristic rather than template-structural.
- Persistent checkpoint publication is not atomic.
- Source defaults, server documentation, and parent runner overrides conflict.
- Router mode, built-in tools, and MCP proxy are explicitly experimental or unsafe for untrusted exposure.
