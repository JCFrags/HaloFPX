# Maturity and risk register

## Maturity distribution

| Level | Count | Meaning |
|---|---:|---|
| M0 | 1 | Documented, dead, duplicate, or unbuilt path. |
| M1 | 25 | Prototype/integrated path with material correctness, security, observability, or validation gaps. |
| M2 | 68 | Operational source path with active churn, recent fixes, or incomplete focused tests. |
| M3 | 26 | Integrated path with focused tests or repeated operational evidence; may still be explicitly experimental. |
| M4 | 0 | Stable, released, compatibility-governed feature. No custom feature in this assessment qualifies. |

## Critical risks

| Risk | Severity | Affected features | Evidence | Required control |
|---|---|---|---|---|
| Persistent final-name partial files after crash/power loss | Critical | F-001, F-005, F-013, F-015 | [E-034](20-Evidence-Index.md#e-034), [E-206](20-Evidence-Index.md#e-206) | Use target temp-file/atomic-rename protocol and an atomic manifest. |
| Cache ownership based on client-supplied body metadata | Critical | F-011, F-036–F-047 | [E-081](20-Evidence-Index.md#e-081), [E-087](20-Evidence-Index.md#e-087) | Bind tenant from authenticated middleware; use opaque keyed scope. |
| System boundary can include user content | Critical | F-016, F-020 | [E-054](20-Evidence-Index.md#e-054) | Derive boundary from message/template AST and verify tokens. |
| Shared-readable persistent state | High | F-012 | [E-041](20-Evidence-Index.md#e-041) | Preserve 0700/0600; optional encryption at rest. |
| Anonymous cap documentation contradicts implementation | High | F-039, F-040 | [E-080](20-Evidence-Index.md#e-080), [E-084](20-Evidence-Index.md#e-084) | One scheduler admission policy; require auth for multi-tenant mode. |
| User caches omitted from stats and turn aging | High | F-043, F-044, F-054, F-084 | [E-064](20-Evidence-Index.md#e-064) | One cache registry and lifecycle interface. |
| Stateful target/draft/spec partial restore | High | F-003, F-029, F-033, F-109 | [E-067](20-Evidence-Index.md#e-067), [E-207](20-Evidence-Index.md#e-207) | Exact pair/shape validation; clear partial state; safe miss fallback. |
| Startup scan grows without bound | High | F-015, F-052 | [E-031](20-Evidence-Index.md#e-031) | Checksummed manifest, startup budget, quarantine, background reconciliation. |
| Experimental router exposed to untrusted users | High | F-057–F-061, F-074 | [E-096](20-Evidence-Index.md#e-096) | Authz, preset allowlists, quotas, loopback/private network. |
| Built-in tools or MCP proxy exposed with inference API | Critical | F-111, F-112 | [E-121](20-Evidence-Index.md#e-121), [E-123](20-Evidence-Index.md#e-123) | Reject from inference service; isolate in sandbox/proxy. |
| Destructive startup process cleanup | High | F-087 | [E-009](20-Evidence-Index.md#e-009) | PID/service ownership and graceful shutdown. |
| Privileged bootloader mutation | High | F-091 | [E-012](20-Evidence-Index.md#e-012) | External administrator procedure only. |
| GPL code copied into MIT target | High | F-049, F-063, F-085–F-093 | [E-003](20-Evidence-Index.md#e-003), [E-021](20-Evidence-Index.md#e-021), [E-201](20-Evidence-Index.md#e-201) | Clean-room reimplementation or separate GPL package. |

## Validation gaps

- No focused CachyLlama test suite was identified for restart persistence, system-prefix false boundaries, concurrent instances, or authenticated tenant isolation.
- The standalone page-manager test source is not evidence that the active server path is tested.
- The target's disk prompt-cache tests are strong for run-scoped failure containment but do not yet prove restart persistence.
- Recent cold-continuation corrective history lowers confidence in slot-ID restore maturity.
- Strix performance notes are useful engineering evidence but mix local environments and are not a release-level benchmark contract.

## Release gate for a ROCmFPX port

A persistent mode should not be production-enabled until all of the following pass:

1. abrupt process termination during every publication step;
2. restart with valid, partial, stale-version, corrupt, and orphan entries;
3. two server instances sharing a configured root;
4. target-only, target+draft, and stateful MTP records;
5. sequence-ID remap across different source/destination slots;
6. tenant A cannot discover or restore tenant B state;
7. anonymous requests cannot participate in cross-restart reuse by default;
8. system-prefix boundary golden tests across all supported templates;
9. quotas, eviction, breaker state, and metrics are internally consistent;
10. matched Strix Halo correctness and performance tests for Vulkan and HIP.
