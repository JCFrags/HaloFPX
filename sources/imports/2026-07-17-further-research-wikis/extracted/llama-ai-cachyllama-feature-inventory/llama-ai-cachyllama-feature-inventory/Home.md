# Home

> **LLM Wiki:** commit-pinned capabilities, evidence, and ROCmFPX disposition.

| Repository | Exact revision | Role | License observed |
|---|---|---|---|
| `fewtarius/llama-ai` | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | launch, build, deployment, model/profile orchestration | GPL-3.0 |
| `fewtarius/CachyLlama` | `6be745998f568e379ea197fcf827baec73ff9940` | pinned inference component | MIT |
| `charlie12345/ROCmFPX` | `a5605a72768c6562241b248e268e33dc92787394` | portability target baseline | MIT |

## Inventory summary

- **120** feature rows.
- **118** evidence records.
- **68 retain**, **39 redesign**, **13 reject**.
- No assessed custom feature is classified M4/stable-release maturity.

## Highest-value capabilities

1. Persistent target/draft/spec sequence-state records and destination-slot restore.
2. Restart-time prefix matching and high-threshold continuation.
3. Cross-conversation system-prefix reuse.
4. Slot save/restore APIs, slot prompt reuse, and resumable output streams.
5. Model router lifecycle, OpenAI/Responses/Anthropic surfaces, metrics, LoRA, and MoE diagnostics.
6. Strix Halo gfx1151 deployment profiles and backend-specific experiments.

## Highest-priority redesigns

1. Build persistence on ROCmFPX's atomic cache engine.
2. Bind cache/slot ownership to an authenticated tenant principal.
3. Replace heuristic system-prompt boundary detection with template/message structure.
4. Consolidate anonymous and per-user cache lifecycle, quotas, and metrics.
5. Use a checksummed manifest, quarantine, and bounded startup reconciliation.
6. Preserve ROCmFPX's MTP exact-boundary and failure-containment behavior.

## Critical rejects

- Direct-final-name persistent writes.
- Shared-readable cache permissions.
- Client-supplied user IDs as a security boundary.
- The claimed anonymous concurrency bucket in the pinned implementation.
- Split user-cache lifecycle paths that omit statistics or turn aging.
- The unbuilt duplicate `kv_page_manager` path.
- Parent `pkill -9` startup cleanup and privileged bootloader mutation.
- Built-in shell/file tools and MCP proxy in an untrusted inference service.

Continue with [Scope and pins](00-Scope-and-Pins.md), [Architecture](02-Architecture.md), or the complete [retain/redesign/reject matrix](15-Retain-Redesign-Reject.md).
