# ADR-0043: guarded exact-primary correctness and cache canary

- Status: rejected at independent pre-mutation review; L19 terminal NOT PROMOTED
- Date: 2026-07-21
- Base: `93c61eadd167285be448ef1e99b80f429fa4299a`
- Scope: one controller-managed maintenance transition and one material load

## Decision

L19 may perform exactly one material load of the pinned 159,873,097,824-byte
MiniMax ROCmFPX artifact after the existing production controller has admitted
the frozen host, unit, process, listener, HTTP, rollback, protected-key, and
capacity authorities. The disposable worker must complete exact HELLO and
HFXCAP2 admission before the coordinator starts.

The command is frozen to `--device RPC0,ROCm0 --split-mode layer
--tensor-split 1,1`, the 1,129-token request, 1,128-token saved boundary, 128
generated tokens, context 4096, batch and ubatch 512, Q8_0 K/V, seed 1234, and
temperature zero. The L18 real architecture-loader plan is authoritative:

- RPC0: layers 0-31 and `output.weight`, one 80,950,550,528-byte material
  weight allocation;
- ROCm0 device: layers 32-61 and `output_norm.weight`, one 78,280,456,704-byte
  material weight allocation; and
- ROCm host: `token_embd.weight`, one 633,802,752-byte material weight
  allocation.

The runner must retain the exact L18 plan as pre-allocation admission and bind
the material run to observed loader and worker allocation evidence. A missing,
monolithic, reordered, or inconsistent material group aborts the attempt. The
material observations do not replace L18's complete 809-tensor accounting.

The single sequence is capture, clean cold, restart restore, supported
missing-object and plan-mismatch cold fallbacks, and one matched runtime-off
cold control. All suffix token and decoded-byte hashes must be identical.
Capture and restore state windows must contain zero legacy GET_TENSOR or
SET_TENSOR operations. Every state failure remains discard-and-cold; no partial
state is authoritative.

## Operational boundary

Only `scripts/halofpx-production-transition.py` may mutate the production
units. It stops nimo-1's coordinator before nimo-2's worker and restores
nimo-2's worker/50052 before nimo-1's coordinator/8081 and HTTP 200. Its
preflight snapshot and fresh identical mode-0600 channel keys precede the first
mutation, and its abnormal-exit trap owns rollback.

L19 is terminal after this attempt whether it passes or fails. It does not
enable production cache use, promote performance, authorize tuning or retries,
or open L20.

## Review disposition

The independent pre-mutation review rejected the inherited executable path.
Each canary mode starts a separate process whose initialization material-loads
the primary model before mode dispatch, so the requested six-mode sequence
would perform six material loads rather than the one authorized here. The
production controller is also still explicitly bound to L16 key paths,
disposable-unit names, and child binary identity. Re-labeling only the child
would make protected-key consumption fail after shutdown and could leave L19
units outside rollback cleanup authority.

No controller shutdown or production mutation was attempted. L19 therefore
closes as a reviewed narrow blocker under its literal one-load boundary.
