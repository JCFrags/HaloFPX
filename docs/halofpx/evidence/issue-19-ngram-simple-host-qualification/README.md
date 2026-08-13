# Issue #19 ngram-simple host qualification

Status: **HOST QUALIFICATION PASSED; FEATURE REMAINS DEFAULT-OFF**

The first worktree-backed receipt was superseded after independent review. The
authoritative host result below was rebuilt and tested from a Git-free export
of exact post-rebase commit `eb04b9baad514b4cfde60dce2c96725e53ec115d`,
tree `00ad8a7655c2c633392a605fbf400ee507eb04d8`. The evidence summary was added
after that tested source commit and does not imply that later source was tested.

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
The local CPU build used GCC 15.2.0. Its retained 309,340,160-byte source
archive has SHA-256 `afa6aad7933df5ec15bad48b1660846bad38879cc44dbba62ae64c908b55b9d1`.
The extracted 10,622-entry manifest has SHA-256
`141aa600c52acce0db65e9a64adb8a75ea711a7f81893084fbd5fc1aee5ab4af`.
The fresh CMake build directory had no prior cache or build graph; its only
preconfigure entry was the zero-byte source-export log. The raw receipt
retains the export/configure/build commands and zero return codes, configure
and build logs, `CMakeCache.txt`, `build.ninja`, and the source archive. The
cache binds `CMAKE_HOME_DIRECTORY` to that exact export, and the qualified
server is under the same build root.

The build used `LLAMA_BUILD_WEBUI=OFF` and `LLAMA_CURL=OFF`. The parser test
used `LLAMA_TEST_SKIP_NETWORK=1`; its retained log explicitly confirms that
the legacy hardcoded URL tests were skipped. An older Web UI provisioning
attempt has no retained file and is labeled `UNRETAINED_ANECDOTAL`. A later
PowerShell-to-WSL wrapper failure occurred before CMake and is retained with
file hashes and sizes in the summary. Neither has qualification authority.

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
`draft_n=100` and `draft_n_accepted=22`; this is observed natural rejection,
not deterministic rejection injection. The reusable receipt tool records the
exported commit/tree/manifest, server and model hashes, exact process argv,
`LD_LIBRARY_PATH`, every file-backed `/proc/<pid>/maps` entry and SHA-256,
startup-log effective batch values, exact request bytes and hash, disabled
cache state, raw props/responses/logs, and exact-process cleanup. PASS requires
the complete mapped-file set to be identical across all six processes, in
addition to the project DSO set.
The bounded, secret-scanned retained result is
[`receipt-summary.json`](receipt-summary.json); it binds the larger local raw
receipt by SHA-256 without committing verbose process logs or model output.
The local raw receipt is
`/tmp/halofpx-ngram-export-eb04b9ba-final/receipt`; its final 62-entry
manifest has SHA-256
`f1494523010b963847f5fe6d7eced798c1a44a702bfc671497bfa91189a273b6`
and passed `sha256sum -c`. The receipt also retains the tool-authored
47-entry core-matrix manifest before the composition controls were appended.

## Rebase composition controls

The same Git-free source export also passed the canonical World1 live-authority
composition gates: the install-ON build and focused suite passed 9/9, while the
install-OFF boundary/feature-graph control passed 1/1. The native/default ngram
build cache records both World1 options and the sampling-sync canary as `OFF`.
The preserved PR67 native sampling contracts passed 2/2, and its sidecar unit
suite passed 15/15. These are host composition controls only; they add no
performance or target-machine claim. Their configure/build/test logs, CMake
caches, build graphs, and normalized summary are covered by the final receipt
manifest.

Earlier observations are retained but superseded for qualification:

- the pre-rebase clean export receipt tested commit `29f717fb`; it remains
  retained with exact manifest identity but has no authority for the rebased
  source;
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
restored file's Git blob is `84cd77e6f2ed59aef4e29dd739b5df916f3cf3f9`.
The clean receipt runs only the isolated ngram module. The
opt-in gate dynamically compares full token arrays and does not bake a
fixture-specific token. Set `NGRAM_SIMPLE_STRICT_PARITY=1` and optionally
`NGRAM_SIMPLE_STRICT_PARITY_UBATCH=<N>` to run it. The standalone Linux receipt
tool is `scripts/qualify-ngram-simple-host.py`.

## Cleanup receipt

Early manual diagnostics leaked 13 local WSL `llama-server` processes on the
reserved ports 18920–18927, 18931–18934, and 18941–18942. After log custody was
confirmed, each exact process was terminated and both `pgrep` and `ss` showed
the executable/ports clear. That early cleanup remains transcript-only and is
not final qualification evidence. The final pytest and six-run receipt record
their own exact executable/PID checks, `finally` cleanup, absent cache paths,
and clear processes/ports after every run.

## Claim boundary

- No speedup, throughput, latency, power, or memory claim is made.
- No Strix Halo target or RPC worker was contacted.
- The experiment remains default-off.
- No tested `n_ubatch` value is presented as a performance candidate.
- Strict greedy parity remains a promotion requirement for the eventual matched
  two-rank experiment.
