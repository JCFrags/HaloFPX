# CI recommendations

## Pipeline model

Use separate correctness, hardware, fault, statistical, and promotion workflows. Do not place every case in one monolithic job.

| Lane | Trigger | Typical scope | Gate |
|---|---|---|---|
| Suite validation | every change | schemas, manifest, harness unit tests | required |
| PR CPU native | every fork PR | fast CTest/native server smoke | required |
| PR sanitizer | parser/state/API changes; otherwise scheduled | malformed GGUF/state/HTTP, cancellation smoke | required where selected |
| Cross-fork CPU | nightly and integration PR | exact upstream-compatible tests | required |
| Server compatibility | nightly | native, OpenAI, Responses, Anthropic, schemas, streams | required |
| GPU backend | nightly on named self-hosted lanes | backend ops, logits, kernels, deterministic repeat | required per supported backend |
| ROCmFPX | ROCm/Vulkan lane | type matrix, TurboQuant, CPU reference vs accelerator | required for integration feature |
| Cache persistence | nightly | save/restore, restart, tiers, rejection, isolation | required for integration feature |
| Long context | nightly/weekly | boundaries, shift, passkey, long-prefix restore | required on release branches |
| Speculative/MTP | nightly | target parity, MTP capability, cache, cancellation | required for supported methods |
| RPC | nightly isolated network | local/remote parity, disconnect, mismatch, cancel | required if shipped |
| Statistical | weekly | preregistered non-greedy seed schedule | only with approved profile |
| Performance | weekly | throughput/latency/memory telemetry | record-only until approved |
| Fault lab | weekly/manual | OOM, ENOSPC/EIO, device loss, interrupted writes | required before major release |
| Reference promotion | manual protected environment | independent reference/calibration/validation | never run from candidate PR |

## Fast PR selection

A normal CPU PR should run:

- suite integrity and harness unit tests;
- upstream native `main` tests relevant to changed paths;
- GGUF parser negatives;
- tokenizer and chat template tests;
- sampler and grammar tests;
- tiny-model state and deterministic smoke;
- server health, tokenize, completion stream/non-stream, schema/error smoke;
- changed-feature matrix cases selected by path mapping.

Keep a generated case-selection artifact so reviewers see what was and was not run.

## Hardware lanes

Name lanes by semantics, not generic `gpu`:

```text
linux-x86_64-cpu-avx2
linux-x86_64-rocm-<gpu-family>-<rocm-version>
linux-x86_64-vulkan-<gpu-family>-<driver-version>
linux-x86_64-cuda-<gpu-family>-<cuda-version>
macos-arm64-metal-<soc>-<os-version>
```

A hardware lane records device IDs, driver/runtime, firmware where relevant, memory, clocks/power policy if performance is gated, and dynamic backend library hashes.

## Model and network policy

- Mirror approved model fixtures internally.
- Verify SHA-256 before every run.
- Do not let a release gate follow a mutable URL or branch.
- Keep normal conformance jobs offline after fixture preparation.
- Run RPC only on loopback or an isolated CI network.
- Never expose llama.cpp RPC/server test instances to untrusted networks.

## Artifacts

Retain:

- source locks and merge-base report;
- CMake cache/build flags and compiler version;
- binary/backend-library digests;
- model/fixture lock;
- capability probe;
- raw observation per case;
- stdout/stderr and server logs;
- normalized comparison report;
- sanitizer output and crash metadata;
- hardware/driver inventory;
- summary with explicit pass/fail/error/skip/not-applicable/uncalibrated counts.

Tar evidence before artifact upload when executable bits, case-sensitive paths, or symlinks matter.

## Reference protection

Approved references live in a read-only store or protected branch/environment. Candidate workflows receive read access only. Promotion requires manual dispatch, immutable source/model locks, independent runs, disjoint validation evidence, and reviewer approval.

## Flake policy

A retry cannot turn a failure into a pass silently. Report initial and retry results. Fix infrastructure flakes at the harness layer; do not widen numerical tolerances or skip semantic assertions to improve pass rate.

## Upstream drift automation

A scheduled job should compare the pinned upstream commit with the selected update candidate and report:

- changed test inventory;
- changed GGUF/state/tokenizer/server APIs;
- changed default parameters;
- new quant types/backends;
- new security advisories;
- renamed server endpoints/fields;
- new speculative/RPC behavior.

The update becomes a reviewed pin change, not an automatic baseline replacement.

Example workflows are under `ci/examples/`. Replace action tags with immutable commit SHAs under your supply-chain policy.
