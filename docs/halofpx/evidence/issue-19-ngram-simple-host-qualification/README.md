# Issue #19 ngram-simple host qualification

Status: **REVIEW REOPENED; PRIOR RECEIPT SUPERSEDED**

The prior worktree-backed receipt has no promotion authority after independent
review. A new receipt must be produced from a clean exported commit tree and
must close every review finding before this status can return to passed.

This is a host-only correctness and observability gate for the default-off
`ngram-simple` experiment in issue #19. It is not Strix Halo, distributed, or
performance evidence. No target machine was contacted.

## Retained source fix

The server configuration seam previously made an explicit `--spec-type` append
to the default `none`, and repeated parsing appended again. The repaired
assignment contract:

- preserves the default singleton `none` when the option is absent;
- replaces it on CLI or environment assignment and makes reparse idempotent;
- preserves ordered, distinct combinations of non-`none` types;
- rejects empty, unknown, duplicate, and `none`-plus-active combinations;
- follows the existing repeated-argument rule that the last value wins.

`/props.default_generation_settings.params` now reports startup-backed task
defaults as typed `speculative.types`, `speculative.n_min`, and
`speculative.n_max` fields. They are server startup/task-default metadata, not
live request metrics. Draft activity remains authoritative only in response
`timings.draft_n` and `timings.draft_n_accepted`.

One observability gap remains: `/props` does not report effective `n_batch` or
`n_ubatch`, even though physical batch shape can change near-tied greedy output.

## Host matrix and parity boundary

The pinned fixture is `stories15M_MOE-F16.gguf`, 73,466,432 bytes, SHA-256
`1240dfc1957df9f3550dd6c1d9e64b466fc2f452d8bc34bd4e45e1a1e2ca6055`.
The local CPU build used GCC 15.2.0 and the exact frozen base
`0ee1217a2dccddc91ea03a1160c18025326a6f33` plus the changes under review.
An earlier build attempt entered the network-dependent Web UI download path and
failed provisioning; it is not qualification evidence. The clean retry used a
new build directory and `LLAMA_BUILD_WEBUI=OFF`, and the final receipt binds
only that retry's executable and mapped libraries.

The standard host tests use exact request identity and memoryless greedy
sampling (`temperature=0`, sampler list `temperature`, seed 42). They cover:

- exact target-only versus ngram-simple token and text parity on a stable
  corpus;
- explicit startup values and exact feature-off/default behavior;
- nonzero generated and accepted drafts plus observed natural rejection
  (`0 <= accepted < drafted`); no rejection is injected;
- context shift;
- two sequential requests; and
- two concurrent requests on two slots.

The adversarial prompt `I believe the meaning of life is` contains a near-tied
greedy decision. That makes it useful for detecting an unmatched physical
batch configuration, but it does not make exact parity portable across
backends or runtime identities. The server documentation already warns that
different batch sizes need not produce bit-identical logits.

The final clean-build qualification therefore used the same explicit
`--batch-size 256` and each same explicit `--ubatch-size` in both target-only
and speculative modes. For `n_predict=96`, the full token arrays and content
were identical at `n_ubatch={1,2,256}`. Each speculative run reported
`draft_n=100` and `draft_n_accepted=22`. The reusable receipt tool records the
source HEAD/tree/status, server and model hashes, exact process argv,
`LD_LIBRARY_PATH`, every file-backed `/proc/<pid>/maps` entry and SHA-256,
startup-log effective batch values, exact request bytes and hash, disabled
cache state, raw props/responses/logs, and exact-process cleanup.
The bounded, secret-scanned retained result is
[`receipt-summary.json`](receipt-summary.json); it binds the larger local raw
receipt by SHA-256 without committing verbose process logs or model output.

Earlier observations are retained but superseded for qualification:

- a default-ubatch target was compared with an explicit-ubatch-1 speculative
  run and differed at generated token 44; this was an unmatched comparison;
- a separate provisional report that `n_ubatch>=2` differed did not retain a
  complete executable/DSO/source identity and was not reproduced by the clean,
  matched matrix.

Neither provisional observation is evidence of a speculative rollback or
sampler defect. The identity-complete matched host matrix is authoritative for
this pinned build and fixture only.

The isolated ngram pytest module is
`tools/server/tests/unit/test_ngram_simple_qualification.py`; the inherited
external-draft `test_speculative.py` remains byte-for-byte unchanged. The
opt-in gate dynamically compares full token arrays and does not bake a
fixture-specific token. Set `NGRAM_SIMPLE_STRICT_PARITY=1` and optionally
`NGRAM_SIMPLE_STRICT_PARITY_UBATCH=<N>` to run it. The standalone Linux receipt
tool is `scripts/qualify-ngram-simple-host.py`.

## Cleanup receipt

Early manual diagnostics leaked 13 local WSL `llama-server` processes on the
reserved ports 18920–18927, 18931–18934, and 18941–18942. After log custody was
confirmed, each exact process was terminated and both `pgrep` and `ss` showed
the executable/ports clear. The retained pytest harness now uses `finally`
cleanup plus an autouse cleanup fixture, including focused `--confcutdir` runs.

## Claim boundary

- No speedup, throughput, latency, power, or memory claim is made.
- No Strix Halo target or RPC worker was contacted.
- The experiment remains default-off.
- No tested `n_ubatch` value is presented as a performance candidate.
- Strict greedy parity remains a promotion requirement for the eventual matched
  two-rank experiment.
