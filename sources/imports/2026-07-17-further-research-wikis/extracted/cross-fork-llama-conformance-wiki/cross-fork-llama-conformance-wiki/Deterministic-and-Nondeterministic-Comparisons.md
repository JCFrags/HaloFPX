# Deterministic and nondeterministic comparisons

## Deterministic lane definition

Exact repeatability is meaningful only when all relevant controls are fixed:

- model and tokenizer bytes;
- source and binary digests;
- backend, device, driver/runtime;
- compiler and build flags;
- CPU thread count and affinity where applicable;
- context, rope, KV type, batch, and microbatch;
- prompt token IDs;
- seed and complete sampler chain;
- slot/sequence setup;
- cache state and restore path;
- speculative type, target/draft pair, and draft controls.

The required controls are machine-readable in `fixtures/determinism/profile.json`.

## Exact comparison lanes

| Lane | Expected comparison |
|---|---|
| Same build, same CPU controls, greedy | Exact token sequence |
| Same server lane, fixed seed, known deterministic sampler | Exact token sequence |
| Stream vs non-stream | Exact assembled content/tokens and terminal metadata |
| Cold vs in-memory cache vs restored state, greedy | Exact continuation tokens |
| Target-only vs speculative/MTP, greedy | Exact target-verified tokens |
| Cross-fork CPU using upstream-compatible feature set | Exact token sequence after all inputs are identical |
| Repeat on one pinned accelerator lane | Exact within that lane |
| CPU vs accelerator logits | Approved numeric/rank profile; do not assume exact floats |
| Different quant model bytes | Not an engine-equivalence comparison |

A deterministic failure should retain first mismatching token position, reference and candidate token IDs/pieces, preceding accepted tokens, relevant logits if instrumented, cache position, and backend trace.

## Cross-backend nuance

Small floating-point differences can change argmax at a near-tie. The suite therefore separates:

- numerical logit/kernel conformance;
- top-rank requirements;
- end-to-end exact greedy token requirements.

A lane can satisfy a numeric profile but fail an explicitly required top-token invariant. That is a real conformance result, not something to normalize away.

## Nondeterministic lane

Non-greedy outputs are compared over the fixed `seed-schedule-v1` fixture. The schedule is divided before execution:

- even labels: calibration evidence;
- odd labels: validation evidence.

Exact text or token-sequence equality is forbidden as the pass/fail criterion. Preserve exact sequences as diagnostic artifacts.

Possible preregistered statistics include token/category total variation, paired acceptance differences, calibrated rank-frequency metrics, or a reviewed hypothesis test. The selected statistic and threshold stay `null` until approved.

## Distributional controls

- Use identical seed labels in paired target-only/speculative or fork-to-fork comparisons.
- Keep prompt and sampler configuration identical.
- Decide categories before examining candidate results.
- Report sample count, missing/failed runs, and every excluded observation.
- Do not rerun only unfavorable seeds.
- Do not tune a threshold on the validation partition.
- Treat a process error, timeout, or invalid output as a failure/error, not as a missing random sample.

## What not to compare

Do not compare prose similarity, BLEU-like metrics, or embedding similarity as a substitute for engine correctness when exact deterministic execution is available. Do not infer nondeterministic correctness from one plausible response.
