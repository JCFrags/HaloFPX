# ADR-0056: versioned Strix A/B runtime comparison

- Status: accepted design; offline implementation qualified; target execution blocked
- Date: 2026-08-12
- Qualified/rebased base: `b6b0d46c461819edafe81a631ba9500d04fae008`
- Scope: issue #15 outer-batch direction screen through the issue #37 harness

## Context

Plan v1 compares feature builds. It requires matched runtime arguments and
distinct OFF/ON condition identity. It cannot honestly express a runtime-only
comparison because it stores one common `runtime.batch` value and forbids
condition arguments.

The first low-risk prompt-processing screen compares outer batch 512 with 2048
while keeping microbatch 512. Allowing arbitrary condition argv would make the
independent variable untyped and could accidentally compare builds, commits,
worker commands, or another runtime setting at the same time.

## Decision

Keep `halofpx.strix-ab-plan.v1` unchanged. Existing v1 plans, canonical plan
digests, generated schedules, generated commands, initialized run roots,
preflights, samples, and analyses are not migrated or normalized.

Add `halofpx.strix-ab-plan.v2` with this closed top-level authority:

```json
"comparison": {
  "kind": "runtime_n_batch",
  "control": "off",
  "candidate": "on"
}
```

Plan v2 replaces `runtime.batch` with:

```json
"batch_by_condition": {"off": 512, "on": 2048}
```

For `runtime_n_batch`, `runtime.ubatch` is exactly 512. OFF and ON must bind the
same source commit and identical complete coordinator/worker artifact objects,
including both path and SHA-256. Path equality matters because identical bytes
in different directories can resolve different origin-relative dependencies.
Condition-specific argv remains empty. Worker commands are byte-identical.

The common coordinator argv omits `-b`, `--batch-size`, and `-ub`, including
underscore-normalized long aliases such as `--batch_size`. It contains one
canonical `--ubatch-size 512` pair and refuses noncanonical spellings. The environment omits
`LLAMA_ARG_BATCH` and `LLAMA_ARG_UBATCH`. Command generation appends exactly one
typed `--batch-size` pair: 512 for OFF and 2048 for ON. After removing that
generated pair, coordinator commands must be byte-identical.

Plan v2 also names `feature_build`. That kind requires 512/512 for both
conditions and retains the distinct-binary rule. It does not weaken or
reinterpret the v1 path.

Plan-v2 analysis adds the comparison kind, control/candidate labels, condition
batch map, ubatch, and hashes of both complete condition commands. The plan
digest continues to bind every generated schedule, command, preflight, sample,
and adapter receipt.

## Adapter and safety boundary

The CachyOS adapter accepts equal binaries only for a validated
`runtime_n_batch` plan. It rechecks the typed batch against generated argv, and
its existing live identity gate binds exact argv and executable SHA-256 for
every fresh warmup and measured process. A changed live batch or executable
identity fails closed and enters cleanup.

`TARGET_EXECUTION_ENABLED` remains `False`. The adapter refuses the real SSH
path before contacting either target. Issue #41 still requires an authorized
maintenance window, exact before-state authority, an empty foreign
KFD/render/HMM-owner census, clean OOM authority, reviewed cleanup, and exact
two-rank recovery with a real minimal inference after identity change.
Therefore this decision establishes no CachyOS, ROCm, `gfx1151`, prompt-rate,
generation-rate, or performance result.

## Qualification

Focused core and adapter tests must retain the v1 document identities, exercise
a complete plan-v2 init/preflight/record/analyze path, prove the exact generated
command delta, reject every untyped or mismatched runtime control, exercise
equal-artifact preflights, bind fake live process identities, and prove the
target SSH gate remains unreachable. The existing issue #37 safety and cleanup
tests remain in the same focused continuous-integration row.

## Rejected alternatives

- Put `--batch-size` in condition argv: rejected because an arbitrary argv delta
  does not establish a singular typed comparison.
- Use separately built or separately located identical binaries: rejected
  because build and origin-relative dependency identity would be uncontrolled.
- Rewrite plan v1 in place: rejected because it would invalidate existing
  portable evidence and silently change the accepted feature-build contract.
- Enable the target adapter with the schema change: rejected because a plan
  contract does not satisfy issue #41 maintenance and recovery custody.
