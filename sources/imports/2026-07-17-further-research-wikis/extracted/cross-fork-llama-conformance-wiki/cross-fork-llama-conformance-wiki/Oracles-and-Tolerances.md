# Oracles and tolerances

## Oracle taxonomy

| Oracle | Appropriate for | Gate before calibration? |
|---|---|---:|
| Exact bytes/text | GGUF round trip, detokenization, chat rendering | Yes |
| Exact token sequence | tokenization, greedy generation, deterministic restore/speculative parity | Yes |
| Exact/normalized structure | metadata, JSON, SSE event order, counters | Yes |
| Error contract | exit/status, normalized class, required envelope | Yes |
| Language membership | grammar, JSON schema, tool arguments | Yes |
| Metamorphic invariant | cache/replay, batching, sequence isolation, enable/disable instrumentation | Yes |
| Native test | upstream CTest/pytest pass/fail | Yes |
| Numeric reference | logits, embeddings, dequantized values, backend operations | **No** |
| Distributional reference | non-greedy sampling | **No** |
| Quality reference | perplexity/KLD/task metrics | **No** |
| Performance reference | latency/throughput/memory | **No**, unless separately approved |

## Exact does not mean raw process equality

Normalize only fields declared volatile by the case. For example, request IDs and wall-clock timings may be removed from an API response, but token IDs, stop reason, usage counters, tool arguments, and status remain semantic.

## Numeric profiles

The template at `references/tolerances/UNCALIBRATED.template.json` contains these optional metrics, all set to `null`:

- maximum absolute difference;
- maximum relative difference;
- mean absolute difference;
- cosine distance;
- top-1 equality;
- top-k token overlap;
- a preregistered distribution statistic;
- a p-value floor when an independently reviewed test supplies one;
- quality delta;
- scope fields for cases, forks, backends, model digest, build family, lane, and driver.

A comparator rejects a numeric/distributional profile unless its status is `APPROVED`.

## Why no universal tolerance exists

Numerical behavior depends on model bytes, quantization type, operation, tensor shape, backend, compiler, instruction set, fast-math choices, GPU architecture, driver/runtime, batching, context position, and sometimes kernel selection. One permissive global epsilon can conceal a real kernel bug; one strict epsilon can reject a valid lower-precision implementation.

## Calibration workflow

1. Pin source, model, fixtures, binaries, backend, device, driver/runtime, and all controls.
2. Select a semantic reference. Use pinned upstream CPU for shared features. Use a fork-specific CPU reference only for a feature absent upstream.
3. Collect repeated calibration observations.
4. Calculate observed deltas with `scripts/calibrate-tolerances.py`. Its normative fields remain `null`.
5. Review error distributions by operation, tensor type, shape, position, and backend.
6. Propose scoped metrics with engineering justification.
7. Evaluate disjoint validation observations once.
8. Approve through a protected workflow with named reviewers.
9. Store the immutable profile and raw evidence.
10. Recalibrate when any scoped identity changes materially.

Observed maxima are evidence, not automatically the correct threshold.

## Deterministic ranking checks

A numeric logit profile may also require the top token to match or a declared top-k overlap. These are not substitutes for numeric comparison. Near-ties should be included explicitly so reviewers can decide whether exact rank is a semantic requirement for that lane.

## Native numeric assertions

Upstream native tests may contain their own expected numeric constants and tolerances. Reuse those tests unchanged. The no-invention policy means this suite does not create new model/kernel expected values without evidence; it does not strip or weaken established upstream assertions.

## Reference lifecycle

`UNCALIBRATED → PROPOSED → APPROVED → RETIRED`

Only `APPROVED` references and profiles may gate. A candidate fork job has read-only access to approved artifacts and cannot update them.
