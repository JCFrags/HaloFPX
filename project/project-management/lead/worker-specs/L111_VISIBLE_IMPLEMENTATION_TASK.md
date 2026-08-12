# HaloFPX L111 Visible Implementation Task

## Purpose

Continue the current L111 implementation in a user-visible Codex task.
Do not restart the work from memory.
Use exact repository state and retained evidence as authority.

## Required reading

Read these files before any edit:

1. `C:\Users\britt\Documents\Custom_Inference_Project\AGENTS.md`
2. `C:\Users\britt\Documents\Custom_Inference_Project\README.md`
3. `C:\Users\britt\Documents\Custom_Inference_Project\PROJECT_GOAL.md`
4. `project-management\lead\OBJECTIVES.md`
5. `project-management\lead\MONITORING.md`
6. `project-management\lead\CURRENT_STATUS.md`
7. `project-management\lead\DECISIONS.md`
8. `C:\Users\britt\Documents\HaloFPX\docs\halofpx\evidence\l109\`
9. `C:\Users\britt\Documents\HaloFPX\docs\halofpx\evidence\l110\`

Use the Wiki and decisions for context.
Verify important claims against current source and machine evidence.

## Exact handoff

Repository: `C:\Users\britt\Documents\HaloFPX`

Branch: `codex/integration-base-61f2f2d`

Accepted base HEAD: `6c88472bf5f567a1064f27f4d8a90fc8e2b47a02`

Remote: none

The frozen L111 worktree currently contains these authored changes:

- `ggml/src/ggml.c`
- `src/llama-model-loader.cpp`
- `src/llama-model-loader.h`
- `src/llama-model.cpp`
- `src/llama-model.h`
- `src/models/minimax-m2.cpp`
- `tests/CMakeLists.txt`
- `tests/test-halofpx-loader-partition.cpp`

Do not overwrite these changes.
Inspect the complete diff before editing.

Preserve all older untracked L83, L85, L97, and L98 evidence and archives.
They are not L111 work.

## Product direction

L111 is only a loader foundation.
It does not implement expert-parallel execution.

The later product path is:

1. Load real rank-owned expert partitions.
2. Route each MiniMax MoE layer once.
3. Run local and remote expert branches concurrently.
4. Use authenticated asynchronous RPC completion.
5. Join rank partials in a deterministic order.
6. Require exact output and a material matched speed improvement.

Do not implement steps 2 through 6 in L111.

## L111 implementation contract

Create one loader-internal transaction for two physical partitions of one
logical GGUF tensor.

The transaction must meet all these rules:

- Use exactly logical ranks `0` and `1`.
- Use two distinct admitted non-host devices.
- Use exact device-owned buffer types.
- Verify the source tensor, type, dimensions, and byte layout.
- Validate both partitions before the first GGML mutation.
- Require positive and valid dimensions.
- Require complete axis-2 coverage.
- Refuse gaps, overlap, reversal, and out-of-range limits.
- Verify exact packed source-byte offsets.
- Verify context capacity before mutation.
- Verify all accounting arithmetic before mutation.
- Reserve source-offset map capacity before mutation.
- Refuse tensor overrides and ambiguous ownership.
- Refuse a hidden full tensor plus a partial duplicate.
- Do not expose a public raw checkpoint.
- Do not accept caller-asserted primary ownership.
- Bind internal transaction state to its loader and generation.
- Refuse stale and cross-loader transaction state.
- Make the commit path non-throwing after rank-0 tensor creation.
- If a later operation can throw, implement exact rollback first.
- Return an optional or typed refusal for an unknown source offset.
- Never map an unknown source offset to byte zero.

Accounting rules:

- Count one logical source tensor.
- Include both physical allocations exactly once in buffer sizing.
- Preserve exact progress reporting.
- Preserve exact mmap and source offsets.
- Exclude implementation-only tensors from public lookup.
- Leave no partial tensor, offset, lookup, counter, or context mutation on
  failure.

## Legacy path decision

The old default-off MiniMax `peer_half_load` path uses:

- one full local expert tensor; and
- one upper-half peer tensor.

This is not true rank partitioning.
Do not change its graph computation in L111.

Remove or disable the obsolete loader mode and its public raw slice and lookup
exclusion APIs.
Preserve its historical evidence.
Return a clear typed configuration refusal if a user requests the removed mode.

Normal MiniMax loading must remain unchanged.
Feature-off behavior must remain unchanged.

The new atomic pair can remain test-only in L111.
A later graph milestone will adopt true lower-half and upper-half ownership.

## Focused qualification

Use a small GGUF fixture.
Use two genuinely distinct fake or test backend devices.
Two aliases of one device are not sufficient.

Positive tests must verify:

- exact bytes in both target buffers;
- exact tensor shapes and source ranges;
- `done_getting_tensors`;
- logical tensor count;
- physical buffer sizes;
- `size_data`;
- progress;
- mmap and source offsets;
- public lookup;
- deterministic destruction;
- no hidden full allocation; and
- feature-off parity.

Negative tests must verify:

- missing rank;
- duplicate rank;
- rank outside `{0,1}`;
- missing partition;
- gap;
- overlap;
- reversed range;
- out-of-range limit;
- invalid non-axis dimensions;
- same target device;
- host target;
- wrong buffer ownership;
- tensor override;
- ambiguous public tensor;
- hidden full tensor;
- stale transaction;
- cross-loader transaction;
- unknown source offset; and
- injected failure at every precommit boundary.

Every failure must leave:

- context used memory unchanged;
- context tensor chain unchanged;
- offset map unchanged;
- lookup state unchanged;
- `size_data` unchanged;
- logical tensor count unchanged; and
- progress unchanged.

## Toolchain

First inspect the local toolchain.

If local compilation is unavailable, use the existing disposable Linux
toolchain on `nimo-1` only under these limits:

- no production service mutation;
- no model access;
- no production listener use;
- no persistent remote source after cleanup;
- exact source archive identity;
- exact build receipt; and
- bounded cleanup.

Known disposable toolchain:

- CMake 4.3.4
- GCC 16.1.1
- Ninja 1.13.2

## Review and retention

Obtain a fresh independent adversarial review of:

- the exact final diff;
- all focused test evidence;
- accounting;
- failure unwind;
- public API removal;
- feature-off behavior; and
- repository cleanliness.

If review fails:

- remove only the L111 candidate source;
- preserve evidence;
- commit an evidence-only terminal result; and
- report the exact blocker.

If review passes:

- retain the source;
- commit a clean L111 PASS;
- stop before graph or RPC work; and
- report the next bounded graph integration step.

## Prohibited work

Do not:

- run the primary model;
- access model artifacts;
- mutate production;
- change MiniMax graph execution;
- add asynchronous RPC;
- change the scheduler;
- restart cache integration;
- claim performance; or
- run a broad test matrix.

## Reporting

Send terminal events and genuine blockers to Project Lead task
`019fa661-0b7d-7a63-8fb4-07658f368f55`.

Include:

- exact HEAD;
- branch and worktree state;
- exact diff;
- tests and binaries;
- independent review;
- preserved evidence;
- cleanup; and
- requested decision.
