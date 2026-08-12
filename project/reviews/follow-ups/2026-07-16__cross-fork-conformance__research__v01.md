---
title: "Cross-fork semantic conformance corpus design"
date: "2026-07-17"
status: "research"
priority: "P1"
scope: "Pinned llama.cpp, ROCmFPX, CachyLLama, and llama-ai snapshots"
artifact_type: "follow-up research design"
---

# Cross-fork semantic conformance corpus design

## 1. Verdict and boundary

**[RECOMMENDATION]** Build one manifest-driven corpus with capability-specific adapters and oracles, not one assumed byte-compatible test suite. The corpus should compare externally observable semantics where the forks share a contract, exercise fork-only features against their own declared contracts, and require safe rejection when an artifact or operation is unsupported.

This document defines the corpus, fixtures, comparators, provenance rules, and failure semantics. It does **not** report conformance results, establish benchmark numbers, assert cross-fork state portability, or promote any claim to `[MEASURED]`. Numeric tolerances remain `CALIBRATION_REQUIRED` until a controlled repeatability study supplies evidence.

`llama-ai` is treated as an orchestration participant around its pinned CachyLLama submodule, not as a fifth independent inference engine. Its conformance surface is profile expansion, binary selection, environment/argument precedence, API behavior, and error propagation.

## 2. Pinned lineage and legal boundary

| Participant | Exact revision | Corpus role | Repository-level license at the pin |
|---|---|---|---|
| `ggml-org/llama.cpp` | `788e07dc91d266ad3162a1ce9037665656269689` | Shared-surface reference candidate and upstream test seed | MIT |
| `charlie12345/ROCmFPX` | `a5605a72768c6562241b248e268e33dc92787394` | ROCmFPX types, MTP/speculation, state, RPC, and shared-surface candidate | MIT |
| `fewtarius/CachyLLama` | `6be745998f568e379ea197fcf827baec73ff9940` | SSD checkpoint/cache semantics plus shared-surface candidate | MIT |
| `fewtarius/llama-ai` | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | Wrapper/profile/API conformance; pinned CachyLLama gitlink resolves to `6be745...` | GPL-3.0-or-later for source; documentation declares CC-BY-NC-SA-4.0 |

**[VERIFIED]** Exact-tree inspection at these revisions shows materially different capability surfaces. ROCmFPX extends the GGML type space and speculative/MTP paths; CachyLLama adds SSD cache/checkpoint paths; the pinned llama-ai tree selects its CachyLLama submodule. These differences are reasons to encode applicability explicitly, not evidence that any implementation passes.

Repository licenses do not automatically license model weights, tokenizer files, chat templates, captured outputs, or third-party fixtures. Every redistributed artifact therefore needs its own provenance and license decision.

## 3. Conformance principles

1. **Semantic conformance is not ABI or serialized-byte compatibility.** Equivalent token streams or state positions may be conformant even when internal layouts differ. A foreign state blob is invalid unless an explicit cross-runtime contract says otherwise.
2. **No universal oracle is assumed.** A reviewed upstream result can be the oracle for a shared contract; fork-only behavior needs a specification-derived oracle, metamorphic invariant, or self-consistency oracle.
3. **Compatibility is fingerprinted.** Model, tokenizer, chat template, build, backend, runtime revision, cache/state format, topology, sampler configuration, and feature flags belong in the result identity.
4. **Unsupported data must fail closed.** A fork-only GGUF type, corrupt checkpoint, incompatible tokenizer, or foreign state must be rejected or cause a documented cache miss/recomputation. It must never be silently reinterpreted.
5. **Deterministic and statistical claims are separated.** Greedy/tokenizer/metadata contracts can be exact. Stochastic sampling needs a preregistered statistical design when exact RNG replay is not part of the contract.
6. **Failures are outputs.** Exit status, signal, HTTP status, structured error, stderr, timeout, partial output, and cleanup behavior are recorded and normalized.
7. **A candidate cannot mint its own golden.** Golden promotion requires an identified oracle, source evidence, independent review, and a signed manifest update.

## 4. Proposed corpus layout

```text
conformance/
  README.md
  manifest.yaml
  schemas/
    fixture.schema.json
    result.schema.json
    error.schema.json
  fixtures/
    tokenizer/
    chat/
    gguf-structural/
    model-references/
    logits-sampling/
    state/
    speculation/
    rpc/
    failures/
    wrapper/
  adapters/
    llama-cpp/
    rocmfpx/
    cachyllama/
    llama-ai/
  goldens/
    <fixture-id>/<compatibility-fingerprint>/
  results/
    <run-id>/
  licenses/
  provenance/
```

This is a design target only; this follow-up does not create the corpus or copy third-party fixtures.

### 4.1 Fixture manifest

Every fixture should declare:

- stable `fixture_id`, schema version, capability, purpose, and owner;
- applicability: required, optional, expected-reject, or not-applicable per participant;
- source URI, source revision/path, acquisition date, cryptographic digest, license/SPDX identifier, redistribution permission, and attribution;
- hashes for model, tokenizer, chat template, GGUF, state/checkpoint, grammar, and RPC payload artifacts;
- exact runtime commit, submodule commit, build flags, compiler, linked runtime libraries, backend/device, driver, OS, and topology;
- command, environment allowlist, working directory, API request, seed, sampler chain, context/batch/ubatch/slot settings, cache types, and feature flags;
- oracle type, comparator, invariant set, and tolerance profile;
- timeout, maximum memory/disk/network use, cleanup expectation, and sensitivity classification;
- expected error category and permitted side effects for negative fixtures.

`llama-ai` results additionally record the fully expanded command, selected binary path and digest, resolved profile, CachyLLama submodule SHA, environment precedence, and downstream response.

### 4.2 Result record

A result should be append-only and contain the fixture manifest digest, compatibility fingerprint, timestamps, captured stdout/stderr, raw and normalized errors, API transcript, token IDs/text, requested logits or summaries, state/cache artifact digests, resource observations, adapter revision, and verdict. Large or sensitive artifacts may be stored by content address with access controls, but their digest and retention policy remain in the record.

## 5. Golden lifecycle and comparator classes

### 5.1 Golden lifecycle

Goldens move through four explicit states:

1. `declared`: expected contract and oracle named;
2. `generated`: candidate artifact created with full provenance;
3. `reviewed`: independent source and artifact review completed;
4. `accepted`: manifest pins the digest and compatibility fingerprint.

Regeneration never overwrites an accepted golden. It creates a new version and an explained diff. Goldens are keyed by full compatibility fingerprint, not merely by fork name or model filename.

### 5.2 Comparator classes

| Class | Use | Pass rule |
|---|---|---|
| `E0_EXACT` | Rendered chat bytes, token IDs, GGUF metadata/type identifiers, tensor names/shapes, normalized error category, exit/status class | Exact equality after only fixture-declared canonicalization |
| `S1_SEMANTIC_EXACT` | Continuation after restore, accepted speculative stream, finish reason, logical state position | Same declared semantic sequence/invariants; serialized bytes need not match |
| `N1_NUMERIC_ENVELOPE` | Logits or deterministic numeric intermediates across allowed backend variation | Per element: `abs(candidate-reference) <= atol + rtol*abs(reference)`, plus identical finite/non-finite mask and fixture-declared rank/argmax checks |
| `D1_DISTRIBUTIONAL` | Stochastic samplers when exact RNG replay is outside the contract | Preregistered seed set, statistic, significance level, effect bound, and sample count |
| `M1_METAMORPHIC` | Fork-only behavior without a portable absolute golden | Fixture-declared relation, such as save/restore equivalence or local/RPC equivalence |

`N1` values such as `atol`, `rtol`, top-k stability, or divergence bounds are `CALIBRATION_REQUIRED`. They must be derived from repeated reference runs under controlled matched configurations and reviewed before any pass verdict. `D1` likewise cannot pass until its power analysis and bounds are accepted. One sampled output is never distributional evidence.

The following are unconditional failures regardless of numeric tolerance: unexpected NaN/Inf, tokenization or chat-render mismatch, corrupted state accepted as valid, wrong prefix/state position, crash, hang, timeout without declared timeout semantics, partial durable commit, silent feature downgrade, or unsupported type reinterpreted as another type.

## 6. Suite design

### 6.1 `TOK` — tokenizer and chat-template semantics

Golden fixtures should cover ASCII, multilingual Unicode, combining sequences, normalization-sensitive text, whitespace/newline variants, byte fallback, special-token adjacency, BOS/EOS policy, empty input, long input boundaries, and invalid byte sequences where a byte-oriented API permits them. Record both token IDs and detokenized bytes; require round-trip equality only where the tokenizer contract promises it.

Chat fixtures should cover system/user/assistant roles, repeated roles, empty content, tool calls/results, escaping, generation prompts, BOS/EOS insertion, template selection/fallback, truncation boundary behavior, and malformed role/content structures. The rendered prompt bytes and resulting token IDs are separate `E0` goldens.

Seed candidates may be derived from the pinned repositories' tokenizer/chat tests, but any copied fixture must first pass per-file license and provenance review.

### 6.2 `GGUF` — structural and type semantics

Maintain two classes:

- **Shared structural fixtures:** standard type identifiers, metadata values, tensor names/shapes, alignment, split/shard metadata, unknown metadata keys, empty/small tensors, truncation, invalid offsets, duplicate/conflicting metadata, and malformed headers.
- **Fork-only fixtures:** ROCmFPX-specific GGML/GGUF types and feature metadata, with explicit positive applicability for the pinned ROCmFPX build and `expected-reject` for implementations that do not declare support.

The oracle is parsed structure and declared acceptance/rejection, not identical model output for arbitrary quantizations. No implementation may map an unknown type to a familiar type. CachyLLama SSD checkpoints are not GGUF and must be tested under `STATE`, not smuggled into this suite.

### 6.3 `LOG` and `SAMP` — logits and sampling

Use two layers:

1. **Sampler-unit fixtures** feed a small declared logits vector and candidate set directly into the sampler chain. Cover greedy selection, ties, sorting, temperature, top-k, top-p, min-p, penalties, grammar constraints, seed/reset/clone behavior, empty/degenerate candidate sets, and NaN/Inf handling.
2. **Model fixtures** run a small, redistributable, exactly hashed model/tokenizer with matched prompt and runtime settings, then capture selected logits and generated tokens.

Greedy and deterministic sampler-unit fixtures should use `E0`. Cross-backend model logits use `N1` only after calibration. Stochastic paths use exact replay only if the runtime contract fixes the RNG algorithm and state; otherwise use `D1` with preregistration.

### 6.4 `STATE` — KV, recurrent, sampler, and durable restore

Required fixture families:

- context get/set and sequence get/set within one runtime/build;
- save after prefix, destroy the context/process as applicable, restore, then submit **only the suffix** and compare continuation with an uninterrupted reference;
- fragmented sequence state and recurrent-state rollback where supported;
- sampler, grammar, draft/speculative, and recurrent state captured independently from KV state so missing state ownership is visible;
- multi-sequence isolation and sequence copy/remove operations;
- CachyLLama SSD cold/warm/restart restore, including target/draft/speculative portions;
- invalid/corrupt/truncated payload, changed model/tokenizer/template, changed cache type/context/topology/version, wrong sequence or prefix position, bit flip, and interrupted write;
- cross-fork import matrix, defaulting to `expected-reject` unless an explicit, versioned compatibility contract is proven.

The primary positive oracle is `S1`: suffix-only restored continuation equals the uninterrupted continuation for the declared deterministic configuration, and logical positions match. Full-prompt replay is a separate fallback test and cannot prove checkpoint restoration. Corruption or incompatibility must produce rejection, cache miss, or recomputation; accepting invalid state is a hard failure. Durable checkpoint success must not be described as zero downtime.

### 6.5 `SPEC` — MTP and speculative decoding

For each supported implementation, compare the speculative result to a target-only baseline under a deterministic/greedy configuration. Record proposed tokens, accepted/rejected counts, fallback reason, target verification, and final token stream. ROCmFPX strict-MTP paths should be tested against their declared exact-output mode; unsupported combinations remain explicit.

Fixtures should include vocabulary/BOS/EOS/content incompatibility, draft-context failure, target failure, zero accepted tokens, maximum draft length, early EOS, state save/restore, cancellation, and resource exhaustion. Every stateful speculative implementation needs its own restore fixture. If multiple stateful components cannot be composed, the expected outcome is an explicit rejection—not silent omission of one component's state.

For stochastic speculation, final-distribution equivalence uses `D1`, not equality of one trace. Performance and acceptance-rate comparisons are experiments, not conformance gates, unless a separate requirement specifies them.

### 6.6 `RPC` — remote execution semantics

Run RPC only on an isolated, trusted test network. The suite does not treat the pinned proof-of-concept RPC implementations as authenticated or safe for untrusted exposure.

Compare local and RPC execution using the same pinned binary, model, inputs, and backend assignment. Fixtures should cover tensor metadata transfer, graph execution, cancellation, disconnect before/during/after execution, reconnect, remote process death, malformed/oversized frames, runtime/build mismatch, unsupported tensor type, timeout, duplicate request behavior if applicable, and partial-result cleanup.

The oracle combines `M1` local/RPC equivalence with an exact error category. A transport failure must not be reported as successful inference, reuse a partial result, or leave a state/checkpoint falsely marked valid.

### 6.7 `FAIL` — normalized failure and safety semantics

Adapters should map raw outcomes into this initial taxonomy while preserving all raw data:

| Category | Meaning |
|---|---|
| `INVALID_ARGUMENT` | Malformed or contradictory request/configuration |
| `UNSUPPORTED_TYPE` | GGUF/tensor/cache/type is not implemented |
| `INCOMPATIBLE_MODEL` | Model architecture, metadata, or digest mismatch |
| `INCOMPATIBLE_TOKENIZER` | Vocabulary/template/special-token contract mismatch |
| `STATE_MISMATCH` | State/checkpoint belongs to a different compatibility fingerprint or position |
| `CORRUPT_ARTIFACT` | Digest, bounds, structure, or payload validation fails |
| `RESOURCE_EXHAUSTED` | Memory, disk, slot, or declared quota exhausted |
| `TRANSPORT_LOST` | RPC/network peer disappeared or connection failed |
| `TIMEOUT` | Declared deadline exceeded |
| `CANCELLED` | Caller-requested cancellation completed |
| `INTERNAL` | Unclassified implementation failure |

Exact human-readable wording is not a cross-fork golden. Compare normalized category, status/exit class, and safety properties: no success marker, no invalid durable artifact, no unrelated state mutation, bounded cleanup, and no silent retry that changes semantics. Unknown failures remain `INTERNAL` and require triage; adapters must not guess a more favorable category.

### 6.8 `WRAP` — llama-ai orchestration semantics

Test profile resolution, defaults, explicit-argument precedence, environment precedence, backend selection, selected binary/submodule identity, cache switches, API routes, request translation, downstream exit/status propagation, cancellation, and missing/incompatible binary behavior. Each wrapper fixture should also invoke the same downstream CachyLLama operation directly so the wrapper delta is visible.

The wrapper passes when it invokes the declared pinned engine with the expected expanded configuration and faithfully propagates results/failures. It does not receive an independent inference golden simply because it has a separate repository.

## 7. Applicability matrix

| Capability | llama.cpp | ROCmFPX | CachyLLama | llama-ai |
|---|---|---|---|---|
| Shared tokenizer/chat fixtures | Required where API exists | Required where API exists | Required where API exists | Required through resolved CachyLLama path and wrapper translation |
| Standard GGUF structural/types | Required | Required | Required | Downstream result plus wrapper propagation |
| ROCmFPX-only GGUF types | Expected reject unless support is explicitly present | Required positive path | Expected reject unless support is explicitly present | Expected downstream reject unless selected engine supports it |
| Shared sampler-unit contracts | Required | Required | Required | Downstream identity/translation, not a new engine oracle |
| In-process/sequence state | Required where exposed | Required where exposed | Required where exposed | Downstream result plus wrapper behavior |
| Durable SSD cache/checkpoint | Not applicable unless separately implemented | Not applicable unless separately implemented | Required | Required when the selected profile exposes it |
| MTP/speculative modes | Per declared pinned capability | Per declared pinned capability, including fork modes | Per declared pinned capability | Only modes actually exposed by selected CachyLLama/profile |
| RPC | Per declared pinned capability | Per declared pinned capability | Per declared pinned capability | Only if wrapper/profile exposes it |
| Wrapper/profile semantics | Not applicable | Not applicable | Direct-engine baseline | Required |

Applicability is resolved from the exact pinned source/build and recorded in the fixture manifest. It must not be inferred from a similarly named current branch.

## 8. Execution and promotion plan

### Phase 0 — source and schema review

- freeze source mirrors/commits, submodules, licenses, and fixture schemas;
- map every fixture to a source contract or explicitly labeled invariant;
- implement adapter dry-run output and compatibility fingerprinting;
- review corpus artifacts for secrets and redistribution restrictions.

### Phase 1 — portable CPU baseline

- select a tiny, legally redistributable, exactly hashed model/tokenizer or generate a project-owned synthetic fixture;
- run tokenizer, chat, GGUF structural, sampler-unit, greedy model, and same-runtime state tests;
- calibrate numeric comparators using repeated controlled runs; keep `N1/D1` blocked until accepted.

### Phase 2 — matched accelerated backends

- run HIP/Vulkan/other declared builds using exact drivers and runtime libraries;
- distinguish backend variation from fork variation by matched controls;
- store environment and raw numeric evidence before making `[MEASURED]` claims.

### Phase 3 — fork features

- add ROCmFPX custom types and MTP/speculative modes;
- add CachyLLama SSD checkpoint/cache cold/warm/restart and fault injection;
- add llama-ai profile/wrapper parity;
- add isolated-network RPC local/remote equivalence and failure injection.

### Phase 4 — adversarial compatibility matrix

- cross-load foreign versions/forks only as negative fixtures unless a contract exists;
- mutate/truncate artifacts, change one fingerprint component at a time, interrupt writes/transports, and test resource limits;
- require safe rejection/miss/recomputation and verify no partial durable state is accepted.

No phase is complete merely because a command exits zero. Completion requires preserved raw results, manifest validation, reviewed comparator output, and an explained disposition for every required fixture.

## 9. Provenance, licensing, and sensitive data

1. Maintain `provenance/<artifact-id>.yaml` with source URI, revision, path, retrieval method/date, SHA-256, transformation history, author/owner, license evidence, redistribution flag, and reviewer.
2. Maintain `licenses/<artifact-id>.md` for model/tokenizer/template/test-data terms. Do not infer those terms from the engine repository license.
3. Record an SPDX-style software/build manifest for every adapter and tested binary, including submodules and linked runtime libraries.
4. Prefer synthetic, non-sensitive prompts. Treat state/checkpoint blobs as potentially user-derived sensitive data. Public fixtures may contain them only when generated exclusively from approved synthetic inputs.
5. Do not redistribute weights, tokenizer data, or captured artifacts without affirmative permission. A private fixture may be referenced by digest and controlled acquisition instructions when redistribution is unavailable.
6. **[RECOMMENDATION]** License new project-authored fixture text and schemas under a permissive, explicitly recorded license after project/legal review; do not apply a license retroactively to imported material.
7. Preserve original files and transformation scripts so a derived golden can be reproduced and audited.

## 10. Acceptance criteria

The corpus design is ready for implementation only when:

- each fixture has a capability owner, applicability matrix, pinned evidence source, oracle, comparator class, and license/provenance record;
- a compatibility fingerprint schema is accepted for model/tokenizer/template/runtime/build/backend/state/topology settings;
- no numeric or statistical comparator remains an unlabeled arbitrary constant;
- negative fixtures specify both the normalized error and forbidden side effects;
- state fixtures prove suffix-only restoration and separately classify full-prompt replay;
- llama-ai fixtures record the actual downstream binary and CachyLLama submodule identity;
- RPC fixtures are constrained to an isolated trusted network;
- accepted goldens are immutable, content-addressed, independently reviewed, and regenerable;
- all results preserve raw evidence and no unexecuted design claim is labeled `[MEASURED]`.

## 11. Open decisions and research backlog

1. **[OPEN]** Which small model/tokenizer pair has a license suitable for redistribution and exercises all required tokenizer/GGUF/state paths?
2. **[OPEN]** Which shared behavior is normative upstream behavior versus merely current behavior at `788e07...`?
3. **[OPEN]** What repeatability study and hardware matrix will calibrate `N1` tolerances without conflating backend and fork changes?
4. **[OPEN]** Which sampler implementations promise exact RNG/state replay, and which require distributional tests?
5. **[OPEN]** Does any explicit, versioned cross-fork state/checkpoint portability contract exist? Default remains no.
6. **[OPEN]** For every speculative mode, which component owns sampler, grammar, recurrent, draft, and acceptance state, and can all be restored atomically?
7. **[OPEN]** What integrity/durability guarantees do CachyLLama checkpoint records provide under payload corruption and interrupted writes?
8. **[OPEN]** What RPC protocol/version negotiation exists at the pins, and what failure/retry behavior is contractual?
9. **[OPEN]** Which llama-ai profiles expose RPC, speculation, and SSD cache features, and what exact argument/environment precedence is intended?
10. **[OPEN]** Should project-authored schemas/fixtures use CC0, Apache-2.0, MIT, or another approved license?

Suggested next artifacts are: a schema-only corpus manifest; a source-to-fixture traceability table; a model/tokenizer licensing decision; and a calibration experiment plan. None should be promoted to implementation decisions until reviewed against the pinned source and project requirements.

## 12. Primary-source register

All source references below are pinned; branch-head documentation is not used as revision evidence.

- **CF-01:** llama.cpp exact tree, `788e07dc91d266ad3162a1ce9037665656269689`: <https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689>
- **CF-02:** llama.cpp public API/state/sampler declarations: <https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/include/llama.h>
- **CF-03:** llama.cpp sampler implementation: <https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-sampler.cpp>
- **CF-04:** llama.cpp state save/load test: <https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tests/test-save-load-state.cpp>
- **CF-05:** llama.cpp RPC documentation: <https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/rpc/README.md>
- **CF-06:** ROCmFPX exact tree, `a5605a72768c6562241b248e268e33dc92787394`: <https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394>
- **CF-07:** ROCmFPX GGML type declarations: <https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h>
- **CF-08:** ROCmFPX speculative implementation surface: <https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/common/speculative.cpp>
- **CF-09:** ROCmFPX context/API declarations: <https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/include/llama.h>
- **CF-10:** CachyLLama exact tree, `6be745998f568e379ea197fcf827baec73ff9940`: <https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940>
- **CF-11:** CachyLLama SSD cache implementation: <https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp>
- **CF-12:** CachyLLama server SSD-cache context: <https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-ssd-cache.cpp>
- **CF-13:** llama-ai exact tree, `1017f3dfdce3ca2b06aa9007b23295db3bb35722`: <https://github.com/fewtarius/llama-ai/tree/1017f3dfdce3ca2b06aa9007b23295db3bb35722>
- **CF-14:** llama-ai CachyLLama gitlink at the pin: <https://github.com/fewtarius/llama-ai/tree/1017f3dfdce3ca2b06aa9007b23295db3bb35722/CachyLLama>
- **CF-15:** llama-ai license: <https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/LICENSE>

## 13. Review note

This design intentionally separates executable evidence from expectation. The first useful improvement is not a large test run; it is a small schema-valid fixture set with pinned legal provenance and exact failure expectations. That foundation prevents later output from being mistaken for a portable golden merely because two forks happened to agree once.
