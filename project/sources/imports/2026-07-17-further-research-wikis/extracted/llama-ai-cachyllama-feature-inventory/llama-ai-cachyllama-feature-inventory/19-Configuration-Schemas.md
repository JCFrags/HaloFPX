# Configuration schemas

The repositories expose configuration through shell variables, CLI flags, environment variables, and C++ defaults—not a single canonical JSON document. This bundle supplies normalized JSON Schemas so values and precedence can be reviewed mechanically.

## Included schemas

| Schema | Status | Purpose |
|---|---|---|
| [`llama-ai-runner.schema.json`](schemas/llama-ai-runner.schema.json) | Normalized observed parent behavior | Captures principal `llama-run.sh` defaults and SSD profile overrides. |
| [`cachyllama-server-cache.schema.json`](schemas/cachyllama-server-cache.schema.json) | Normalized observed component behavior | Captures source-level cache/isolation fields at `6be745998f568e379ea197fcf827baec73ff9940`. |
| [`rocmfpx-persistent-cache-port-plan.schema.json`](schemas/rocmfpx-persistent-cache-port-plan.schema.json) | Proposed target design | Defines a secure persistent-mode contract; it is not an existing ROCmFPX API. |

## Current default/override comparison

| Setting | CachyLlama source default | llama-ai runner behavior | Port recommendation |
|---|---:|---:|---|
| SSD path | empty / disabled until configured | defaults non-SSM profiles to project `kv-cache` | preserve explicit target mode and path |
| Max checkpoints | 64 | 64 | retain; add byte quota |
| Hot window | 16,384 tokens | 4,096 tokens | resolve through one generated profile |
| Warm window | 32,768 tokens | profile-dependent / may remain component default | explicit value in persistent profile |
| Max cold | 0 | 32 | byte quota primary; count secondary |
| Page size | 1,024 tokens | profile-dependent | benchmark per state format |
| Max conversations | 16 | component behavior unless overridden | per-tenant byte/entry quota |
| Hot/warm RAM | 0 / automatic | profile-dependent explicit values | explicit first; cgroup-aware auto optional |
| System prompt entries | 8 | 8 | retain with bytes/tenant scope |
| Max unused days | 30 | 30 | retain |
| fsync | enabled | enabled unless `SSD_NO_FSYNC=true` | strict default |
| Per-user concurrency | 0 / disabled | optional override | authenticated scheduler quota |
| Checkpoint min step | 8,192 in source-level fields | parent can inject `--checkpoint-min-step`; server docs at the pin report a conflicting value | one generated default and test |
| Slot prompt similarity | component default | parent sets 0.20 | authorization-aware ranking |

Evidence: [E-006](20-Evidence-Index.md#e-006), [E-022](20-Evidence-Index.md#e-022), [E-023](20-Evidence-Index.md#e-023), [E-024](20-Evidence-Index.md#e-024).

## Proposed ROCmFPX persistent-mode invariants

The proposed schema requires:

- mode is explicit: `disabled`, `run-scoped`, or `persistent`;
- persistent mode supplies a base path, model fingerprint, prefix policy, and retention policy;
- atomic commit remains mandatory;
- directories and files remain owner-only;
- authenticated tenant binding is mandatory;
- raw user IDs are not logged;
- anonymous cross-restart reuse is disabled;
- corruption is quarantined, not silently indexed;
- startup reconciliation has a bounded time budget.

Example: [`examples/10-rocmfpx-proposed-persistent-config.json`](examples/10-rocmfpx-proposed-persistent-config.json).

## Recommended precedence

```text
compiled safe defaults
  < versioned config file
  < named hardware/model profile
  < environment override
  < explicit CLI override
```

Every effective value should appear in one startup configuration report with its source layer. Profiles should be data, not shell string mutation.

## Migration compatibility

Existing ROCmFPX `--cache-disk` behavior should retain run-scoped semantics unless a new persistent mode is explicitly selected. A persistence feature must not silently change cleanup or confidentiality for existing deployments.
