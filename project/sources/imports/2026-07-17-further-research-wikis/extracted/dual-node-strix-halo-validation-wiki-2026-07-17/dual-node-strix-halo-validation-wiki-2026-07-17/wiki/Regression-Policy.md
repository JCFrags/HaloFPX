# Regression Thresholds and Baseline Policy

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


## Default stable thresholds

| Metric/statistic | Warn | Fail |
|---|---:|---:|
| Prefill tokens/sec, decode tokens/sec, goodput | −3% | −5% |
| p50/p95 TTFT and ITL | +5% | +10% |
| p99 TTFT and ITL | +8% | +15% |
| Cold/warm load p95 | +5% | +10% |
| Long-context latency/slope | +8% | +15% |
| Eligible prefix token hit rate | −1 percentage point | −2 percentage points or <95% |
| USB4 reference goodput | −3% | −5% |
| Cold disk read amplification | +10% | >1.25× logical bytes |
| Warm disk read amplification | +5% | >0.10× logical bytes |
| Energy per token versus prior dual release | +4% | +7% |
| Repeatability: MAD/median | >3% | >5% (cold start >10%) |

Absolute SLO failure is a failure even when relative performance improves.

## Confidence rule

A fail-level performance regression is confirmed when the paired bootstrap 95% confidence interval lies beyond the fail threshold. If the point estimate breaches but the interval overlaps the threshold, classify `WARN_RETEST`. Hard events such as crash, corruption, throttle, reset, or wrong output do not require statistical confirmation.

## Baseline eligibility

- Baseline age ≤30 days unless the SUT is immutable and freshness is approved.
- Exact model/tokenizer/data hashes.
- Same operating profile and client placement.
- No unresolved upstream security or critical runtime change between baseline and candidate.
- At least one independent baseline reproduction.

## Baseline updates

Do not replace a baseline because a candidate regressed. Promote a candidate to baseline only after stable acceptance, then preserve the old baseline and decision. Changes in hardware, firmware, kernel, driver, runtime, model, quantization, or cache semantics start a new baseline lineage.
