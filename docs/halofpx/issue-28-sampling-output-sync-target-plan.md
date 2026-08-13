# Sampling-sync coalescing canary — matched target plan

Status: **PLANNED — NO TARGET RUN / NO PERFORMANCE CLAIM**

Tracker: [GitHub issue #28](https://github.com/JCFrags/HaloFPX/issues/28)

## Safety prerequisite

**BLOCKED pending the maintenance admission in
[GitHub issue #41](https://github.com/JCFrags/HaloFPX/issues/41).** Do not
compile, quantize, or run this candidate on either target while a production
process owns `/dev/kfd`, a render node, or retained HMM/unified-memory pages.
An isolated port or directory is not resource isolation.

Before any target action, freeze both production service identities and exact
recovery commands, stop them only inside the authorized maintenance window,
and satisfy the #41 no-owner/drained-memory admission checks on both hosts. If
the window cannot meet those conditions, keep this experiment **[OPEN]** and do
not run it.

## Question

Does the default-off sampling-output synchronization canary preserve exact
generation behavior and improve server generation timing on the intended
dual-Strix-Halo CachyOS system?

## Frozen comparison

Run the current documented roles unless a separately frozen plan records a
different isolated assignment:

- `nimo-1`: coordinator/server rank;
- `nimo-2`: RPC worker rank.

Build the same exact source commit twice on both machines. Vary only:

- control: `HALOFPX_SAMPLING_SYNC_COALESCE_CANARY=OFF`;
- candidate: `HALOFPX_SAMPLING_SYNC_COALESCE_CANARY=ON`.

Hold identical the source tree, compiler and CMake inputs, model and tokenizer
bytes, prompts and request JSON, chat template, seed and sampling controls,
context/batch/microbatch sizes, K/V types, flash attention, device placement,
split/topology, rank roles, cache state, warmup, power/clock policy, and server
and client command lines.

## Required capture

Retain:

- exact source commit, dirty-state patch hash, configure/build logs, compiler,
  CMake, and hashes for both binaries and their runtime libraries;
- both machines' OS, kernel, ROCm/Mesa, firmware, device, clock/power, process,
  and invocation identifiers;
- model/tokenizer/request hashes and complete runtime argument tuple;
- raw streaming responses, emitted-token hashes, failures, and logs;
- completed and reused output-barrier counters plus graph-submission and
  output-transfer counts for matched requests;
- client-observed TTFT and inter-token latency plus server generation rate.

`llama-bench` alone is inadmissible because it does not time the sampling path
under test.

## Procedure and acceptance

1. Use isolated fresh disposable processes and identical cold/warm cache rules.
2. Prove exact greedy token/output parity before interpreting timing.
3. Run counterbalanced, interleaved `OFF`/`ON` pairs. Three pairs are
   directional only; require at least five steady-state pairs before labeling a
   result **[MEASURED]**.
4. Preserve all raw records and compute `SHA256SUMS.txt` only after capture is
   complete.
5. Add `RESULTS.md`, `raw/nimo-1/`, and `raw/nimo-2/` only when the target run
   actually occurs.

Any output mismatch, unavailable row, crash, failed request, or unexpected
graph/transfer-count difference blocks timing and promotion. Until this plan is
executed, target correctness and performance remain **[OPEN]**.
