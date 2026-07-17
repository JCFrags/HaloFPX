# Output Correctness Program

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


## Correctness tiers

| Tier | Purpose | Default acceptance |
|---|---|---|
| C0 Identity | Prove correct model, tokenizer, template, and quantization | All hashes and metadata match |
| C1 Protocol | API fields, streaming order, UTF-8, stop behavior, usage counts, cancel semantics | 100% pass |
| C2 Deterministic canary | Fixed seed/greedy prompts, cache off and on, single versus dual | 100% approved equivalence across at least 100 prompts |
| C3 Numerical drift | Top-token/top-k/logit comparison where engine exposes logits | Within model/backend-specific tolerance; no top-1 change on critical canaries |
| C4 Task quality | Domain suite such as reasoning, code, retrieval, structured output | No more than 0.5 percentage-point absolute or 1% relative drop versus matched single-node, whichever is stricter |
| C5 Long-context fidelity | Retrieval/needle and multi-turn state retention | 100% critical canaries; configured suite threshold otherwise |

## Equivalence policy

Bitwise output is preferred but not universally expected across different floating-point reduction orders or prompt-cache batch shapes. The correctness owner must preapprove one of:

- exact token-ID equality;
- exact structured fields plus bounded token divergence on noncritical free text;
- logit tolerance with top-1 agreement;
- task-score equivalence.

Changing the equivalence rule after seeing results is prohibited without invalidating the comparison block.

## Nondeterminism controls

- Greedy decoding or fixed seed and complete sampler configuration.
- Same prompt token IDs, not merely the same source string.
- Same output length and stop set.
- Separate cache-on/cache-off baselines.
- Repeated identical requests to quantify intrinsic divergence.

`llama-server` documents that cache reuse may produce non-bit-identical logits because prompt and generation batch sizes differ, which is why cache correctness is a distinct experiment rather than an assumed optimization. [[SRC-009]](../references/Sources.md#src-009)

## Silent-corruption rule

Any malformed output, wrong model identity, missing/duplicated token, invalid UTF-8, broken JSON/tool schema, or response that continues after a detected worker fault is a hard failure. It is not waivable as “minor quality drift.”
