# HaloFPX Performance Work Plan

Status date: 2026-08-12. This is a living work ledger, not a performance claim.
Accepted Project Lead/source decisions are the work authority. GitHub issues
are subordinate execution trackers; exact experiment records are the
measurement authority.

**P0 safety gate:** [issue #41](https://github.com/JCFrags/HaloFPX/issues/41)
blocks target builds, quantization, disposable inference, and benchmarks while
production or any unaccounted KFD/render/HMM owner is active. Ordinary
`MemAvailable` and RSS readings are insufficient under high `gpu_active` HMM
ownership. The [2026-08-12 incident](../docs/halofpx/evidence/2026-08-12-target-hmm-oom-incident/README.md)
is a safety record with no benchmark or performance result.
The repository's
[offline maintenance controller](../docs/halofpx/strix-maintenance-admission-controller.md)
qualifies only the fake transaction model. It keeps target execution literal
false. Owner-signed authorization, exact reviewed-source binding, atomic
two-node receipt consumption, and an independent recovery watchdog remain
mandatory before any real maintenance-controller proposal.

## Outcomes

Optimize ROCmFPX-family GGUF inference on the two CachyOS Strix Halo machines
across supported model architectures. Keep three measurements separate:

1. cache lookup, validation, restore, and avoided prompt work;
2. cold prompt processing and time to first token; and
3. token generation throughput and inter-token latency.

MiniMax Q6 Agent remains the largest stress fixture. At least one ordinary,
smaller ROCmFPX model is required for daily iteration and model-general claims.
Conventional GGUF quants are controls unless a task explicitly admits them as
a separate product lane.

## Cache reference authority

The cache-saving behavior donor is the pinned MIT engine fork
[`fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940).
Use its checkpoint persistence, tiering, prefix matching, restart reuse, and
operational behavior as the reference. Wiki Section 56's retain/redesign/reject
decisions remain authoritative for HaloFPX storage, compatibility, integrity,
and durability. Invalid or incompatible state must miss and recompute. Port or
adapt only after capability-level provenance and license review; do not copy
donor code merely because the behavior is authoritative.

## Current measured baseline and cache result

- **[MEASURED] Historical:** The matched dual-node ROCmFPX stress baseline is
  about 203.7 prompt tokens/s and 16.64 generation tokens/s under the exact P07
  conditions. It is not a current-main result. See the
  [P07 receipt](../docs/halofpx/p07-current-head-feature-off-matched-baseline.md).
- **[MEASURED] Diagnostic:** P08 recorded prompt GPU busy averages of 42.69%
  and 35.15% on the two ranks. **[INFERENCE]** Its low observed traffic does
  not support an aggregate-bandwidth-saturation explanation. This motivates a
  scheduling investigation but does not prove prompt-phase causality. See the
  [P08 profile](../docs/halofpx/p08-exact-model-critical-path-profile.md).
- **[MEASURED] Diagnostic:** The correct L101 rank-local restore saved 1,128
  prompt tokens and reduced the recorded cold prompt phase from 6,507.388 ms to
  a 1,168.730 ms restore, with exact token and logits equality. The timing and
  saved-boundary fields are in the
  [capture journal](../docs/halofpx/evidence/l101-attempt-final/child/halofpx-l48-canary-capture-journal.txt)
  and [restore journal](../docs/halofpx/evidence/l101-attempt-final/child/halofpx-l48-canary-restore-journal.txt);
  the [result](../docs/halofpx/evidence/l101-attempt-final/L101_RESULT.md) binds
  correctness and byte coverage. **[INFERENCE]** This demonstrates large reuse
  potential, not an accepted end-user persistent-cache product.
- **[VERIFIED]** PR #20 hardened the run-local SSD prompt-cache spill path
  against same-size corruption. It did not make that path restart-persistent.
- **[VERIFIED]** PR #23 merged at
  `aee627bd46de21327c9082f7915818430d38f453` and closed issue #14. A
  default-off Linux CPU fixture proved exact-key reuse across fresh server
  processes plus compatibility/corruption cold recomputation. Its profile is
  world size 1, rank 0, ordinary transformer memory, and greedy memoryless
  sampling. It is not a distributed or prefix-cache result.
- **[VERIFIED]** PR #27 merged at
  `bf420e9f1db4ea4ba1d7c87771b6a4d662b5be67`. It provides optional
  OpenSSL EVP SHA-256 for the separate run-local SSD prompt cache while
  retaining every exact-length and full-file integrity gate. Scalar and EVP
  hosted tests passed; no target end-to-end speedup is accepted.
- **[VERIFIED]** The issue-#32 L10e slice implements a default-off,
  non-product exact longest-prefix selector around the authenticated catalog.
  It returns explicit suffix-replay and cache-attribution telemetry and fails
  closed on corrupt, ambiguous, or incompatible state. Local Linux CPU tests
  passed; product wiring, two-rank restore, and target performance remain open.
  See the [L10e record](../docs/halofpx/l10e-default-off-exact-longest-prefix-selector.md).
- **[VERIFIED]** The first issue-#33 standalone authority slice now hashes
  exact model/runtime bytes and typed model, tokenizer, template, build/state
  ABI, and fixed two-rank plan facts into the closed compatibility root. It is
  default-off and product-isolated; loader/server adaptation and the remaining
  request-resolved component encoders are still open.
- **[VERIFIED]** The dependent L10f slice adds a separately gated world-1
  server product shell, authenticated manifest-only boundary discovery,
  one-shot state installation, residual accounting, and cache-source telemetry.
  It deliberately remains cold because no trusted live-loader authority
  provider exists. Local CPU tests cover the coordinator and fail-closed server
  fallback; model-backed suffix replay and any speed claim remain open. See the
  [L10f record](../docs/halofpx/l10f-default-off-world1-prefix-product-shell.md).

## Current execution trackers

| Tracker | State | Boundary |
|---|---|---|
| [#14](https://github.com/JCFrags/HaloFPX/issues/14) | closed | bounded world-1 exact-key restart qualification |
| [#25](https://github.com/JCFrags/HaloFPX/issues/25) | active P0 | PR #30 merged default-off and compile-qualified; GPU correctness/parity/performance remain open |
| [#15](https://github.com/JCFrags/HaloFPX/issues/15) | open | model-general Strix Halo prefill matrix |
| [#16](https://github.com/JCFrags/HaloFPX/issues/16) | closed | PR #31 model-general frozen-plan/schedule/evidence core |
| [#37](https://github.com/JCFrags/HaloFPX/issues/37) | open | safe CachyOS process adapter for the A/B core |
| [#41](https://github.com/JCFrags/HaloFPX/issues/41) | open P0 | reject all target work during production or foreign KFD/render/HMM ownership; prove real distributed recovery after identity change |
| [#18](https://github.com/JCFrags/HaloFPX/issues/18) | open | cache source and restored-work metrics |
| [#26](https://github.com/JCFrags/HaloFPX/issues/26) | open | coordinated restart-safe state across two RPC ranks |
| [#28](https://github.com/JCFrags/HaloFPX/issues/28) | open | PR #35 fixed raw/sampled row fallback; coherent snapshot and synchronization reduction remain |
| [#29](https://github.com/JCFrags/HaloFPX/issues/29) | merged source / runtime open | PR #45 default-off dense FFN gate/up activation-conversion reuse; target compile qualified, runtime performance open |
| [#32](https://github.com/JCFrags/HaloFPX/issues/32) | active | exact authenticated selector plus default-off world-1 server shell; positive model-backed restore remains blocked on #33 |
| [#33](https://github.com/JCFrags/HaloFPX/issues/33) | active | standalone typed authority slice complete; loader capture, resolved request/context semantics, reload invalidation, and server adapter remain |
| [#42](https://github.com/JCFrags/HaloFPX/issues/42) | active | prompt-side reuse merged and compile-qualified; strict n=1 local-HIP MMVQ generation slice implemented, target runtime/performance blocked by #41 |
| [#43](https://github.com/JCFrags/HaloFPX/issues/43) | active | portable small Qwen3-0.6B pure ROCmFPX registry/recipe; target qualification remains open |
| [#58](https://github.com/JCFrags/HaloFPX/issues/58) | next P1 | retain/rehydrate authenticated scheduler plans, then versioned prior-lineage RPC recompute; no current product or performance claim |

## Ordered work

### 1. Restart cache correctness — bounded lane complete

PR #23 completed the exact issue #14 fixture. Keep that result separate from
the run-local `--cache-disk` spill path, which is not restart-persistent.
Product/server prefix restore, recurrent/hybrid state, and coordinated
rank-local objects remain unqualified; the bounded exact catalog itself is a
separate qualified laboratory seam. Issue #26 owns the two-rank coordination
lane. Its merged test-only coordinator contract specifies fake-provider
capture/stage/commit ordering and failure semantics, but no real rank-local
cache object, RPC, filesystem, or server adapter exists yet. Any real failure
must miss and recompute without accepting partial rank state.

Issue #32 now has a locally CPU-qualified exact longest-prefix selector and a
separately default-off world-1 server shell. The shell discovers boundaries
from authenticated manifests, installs selected state through a deterministic
test seam, and reports restored/residual work, but the real server cannot take
a positive hit until issue #33 supplies trusted live model/plan authority.
Two-rank composition remains independently owned by issue #26. Target
qualification must measure restored tokens and suffix prefill separately; the
hosted tests are not a performance result.

The issue-#33 standalone authority slice supplies a reusable fail-closed
identity builder, but it remains outside the product route until typed loader
capture, request composition, and lifecycle invalidation are complete.

### 2. Measurement visibility

Add structured phase measurements for cache capture/synchronization,
serialization, integrity hashing, write/sync/publish, verification, decode, and
install. Extend the matched target harness so every condition binds and hashes
both distributed binaries, retains failures, and reports TTFT, inter-token
latency, prompt/decode rates, cache source, bytes, and avoided tokens.
Issues #18, #15, and #37 own the cache-attribution, prefill-matrix, and
CachyOS A/B-adapter parts of this work. Closed issue #16 supplies the
model-general evidence core used by #37.

### 3. First low-risk target screens

Do not begin these screens through the offline controller. They remain blocked
until issue #41 is resolved in a real isolated maintenance window. Any future
real controller must first implement every ADR-0057 promotion gate, including
cryptographic owner approval, atomic authorization consumption across both
nodes, and an independent worker-first recovery watchdog.

- Compare `n_batch/n_ubatch` `512/512` with `2048/512` while holding every
  other variable fixed. The 1,129-token stress prompt currently crosses three
  outer decode calls at `512/512`; the larger outer batch may reduce repeated
  scheduler and RPC setup without changing microbatch size. The offline-
  qualified plan-v2 contract now enforces identical commits and binaries and
  generates the sole typed batch flag. Target execution remains blocked by
  issues #37 and #41, so no direction or speed result exists yet.
- Screen backend greedy sampling as a generation diagnostic. Do not promote it
  if cache eligibility or exact output parity changes.
- Use one warmup and three interleaved pairs for direction. Require five
  interleaved pairs and retained raw records before labeling a gain
  `[MEASURED]`.

### 4. Cache-path performance

The reachable SSD cache performs scalar byte-at-a-time SHA-256 and whole-file
verification traversals on save, load, and touch. The first implementation
candidate is a faster SHA-256 provider with identical-digest golden tests while
preserving every current full-file pre-rename and pre-apply verification.
Hashing only intended serialization bytes would weaken the current verification
of actual temporary-file contents; applying state while hashing would mutate
the context before integrity succeeds. Traversal elimination therefore waits
for an explicit verified-file or staged-apply design. Target measurements must
separate page-cache-hot CPU traversal from physical NVMe reads.

A 2026-08-12 capability screen measured OpenSSL EVP SHA-256 at about 2.42 GB/s
on one pinned, low-priority CPU thread on each target. PR #27 now implements
the optional server-local provider and preserves scalar parity, but it still
does not measure a matched cache request or end-to-end avoided work. Measure
that through issue #18 before making a speed claim. See the
[`SHA-256 screen`](../docs/halofpx/evidence/2026-08-12-strix-halo-sha256-screen/README.md).

### 5. Prompt-processing implementation

RPC currently advertises no asynchronous compute or events, so llama.cpp's
existing multi-copy prompt pipeline is disabled. Basic RPC graph submission is
already send-only; correct completion is the missing boundary. After the batch
screen, implement a negotiated, default-off ordered completion barrier and
remote event lifecycle that acknowledge only after prior backend work is
complete. Advertise asynchronous/event capabilities only for negotiated peers.
Feature-off, single-node, authenticated-graph, failure, reconnect, buffer
lifetime, and KV ordering behavior must remain correct. The authenticated graph
path remains synchronous and excluded until it receives separate integration.

The ROCmFP quantized-KV flash-attention path also falls back for query batches
above two. Extending the existing default-off HIP TILE path to admitted ROCmFP
types is a later prompt/long-context candidate after reference parity and
gfx1151 build qualification. Model weight format and K/V-cache type must remain
separate experiment dimensions.

PR #45 merged issue #29's default-off dense gate/up Q8_1 activation-reuse
source and target compile qualification. Runtime parity, profiler, and matched
performance evidence remain open.

Issue #42 is the next bounded prompt-side candidate: reuse one exact Q8_1
activation conversion across three separate ROCmFPX Q/K/V MMQs. Its source is
built directly on merged PR #45, reorders only proven Q/K/V nodes before
allocation, supports unequal GQA projection widths, and falls back on every
failed predicate. Host contracts and the backend-operation test compile do not
establish target reachability or speed. Issue #41 blocks all target execution.
When the maintenance gate opens, hold the FFN reuse option identically `OFF`
in Q/K/V OFF/ON comparisons so the measured delta has one cause. Report
cache behavior, prompt/TTFT, and generation separately; the Q/K/V candidate's
`ne1 > 8` gate excludes ordinary one-token generation.

### 6. Generation

Issue #25 is the active P0 slice. **[VERIFIED]** The HIP MMVQ consumers for
Q2/Q3/Q6/Q8 ROCmFPX use the Q8_1 activation scale and all quantized bytes but
not its sum lane. PR #30 therefore skips only that sum reduction for the exact
admitted whitelist under a default-off HIP option, preserves the 36-byte block
ABI, and leaves ROCmFP4 and stock formats on the legacy path. Both CachyOS
Strix Halo nodes have compile-qualified the feature-on and feature-off source,
but no target runtime speed result is accepted. Compilation and host contracts
cannot substitute for the focused correctness and counterbalanced A/B recipe
in [`P15`](../docs/halofpx/p15-rocmfpx-mmvq-sum-free-candidate.md).

Issue #42 also owns a bounded generation-side follow-up. The default-off
strict n=1 local-HIP slice recognizes exact generic separate Q/K/V
projections, converts their shared F32 activation to Q8_1 once, and submits
the three unchanged MMVQ consumers with independent GQA widths. Host OFF/ON
and source contracts cover production pre-allocation optimizer ownership and
feature-off graph identity. **[OPEN]** No `gfx1151` runtime, correctness,
real-model reachability, or speed result is accepted; issue #41 blocks target
access. RPC splits also lack this optimizer, so dual-node reachability remains
a separate protocol-aware design. See [ADR-0055](../docs/halofpx/decisions/0055-rocmfpx-strict-n1-mmvq-qkv-q8-reuse.md).

Issue #28 records the subsequent sampling-output synchronization lane. Its
raw/sampled row-count crash is fixed; the remaining snapshot/coalescing work
must bind output provenance coherently and add scheduler-sync instrumentation
before consolidation is promoted.

The current dual-node token path is structurally serialized across remote and
local graph splits. Model-free n-gram speculation can amortize those turns only
when drafts are accepted. Repair and qualify it on repetitive code/prose plus a
non-repetitive control, require exact greedy output parity, and retain accepted
draft counts and target-verification latency. MiniMax n-gram acceptance is
currently **[OPEN]**; retained slower n-gram results apply to a Qwen ROCmFP4
workload, not MiniMax. This is workload-dependent and must remain opt-in unless
evidence supports adaptive use.

The exact feature-off RPC path has no redundant compute-completion response to
remove: graph compute/recompute is send-only, and the following ordered
`GET_TENSOR` response is already the completion boundary. Its steady recompute
frame is only 13 bytes; the GET request is 321 bytes and the output response
remains mandatory. P05's stronger small-command coalescing screen removed 252
send calls without a positive generation point estimate. Do not build a fused
legacy recompute/GET opcode or compact-handle protocol without a new mechanism.

The [source audit](../docs/halofpx/rpc-decode-roundtrip-source-audit.md) found a
larger default-off authenticated-composed omission: that lane disables llama
graph reuse and retransmits `G = 12 + 8*N_nodes + 296*N_tensors` bytes per RPC
split. A direct reuse toggle is P0-invalid because fresh composed authority
changes the transcript while current recompute requires prior-transcript
equality, and the scheduler retains no canonical pre-rewrite authority plan.
Issue [#58](https://github.com/JCFrags/HaloFPX/issues/58) therefore owns two
stages: first retain/rehydrate the scheduler plan while explicitly forcing
fresh full authenticated compute; then add a versioned prior-lineage recompute
record that can honestly omit `G` only for splits admitted by an exact
endpoint/device cardinality policy. The current server has one stored-graph
slot per endpoint/device, so phase 2 must either require one reusable split or
add a negotiated bounded multi-UID table. Both stages keep L40's preparation
receipt, client-side validation, and separate execute. Neither has a current
product or performance claim.

Model-general rank-parallel fork/join execution remains the higher-ceiling
generation architecture candidate once its dependency and whole-join failure
semantics are explicit.

## Kill gates

- Any output, logits, KV/state, corruption, restart, or feature-off mismatch:
  stop and localize before measuring speed.
- Less than a clear 5% directional prompt/TTFT gain for the RPC-pipeline
  prototype: close or redesign before a broad matrix.
- No accepted drafts or no end-to-end generation gain: leave speculation off.
- A microbenchmark win with no matched end-to-end gain is retained as evidence,
  not promoted as project performance.
- Production recovery failure ends the experiment and restores the last
  accepted worker-first/coordinator-second service state.

## Evidence gaps to close

- Recover and import the P01-P14 node-local raw bundles referenced by receipts
  but absent from Git, where still available.
- **[VERIFIED]** The small ordinary Qwen3-0.6B pure ROCmFPX fixture registry
  records source, license, exact producer/consumer commits, hashes, tensor
  census, architecture, and role. **[MEASURED]** Off-target WSL2 conversion and
  CPU smoke evidence is retained. Publish/reconstruct the external bytes, then
  run quality and target HIP/Vulkan qualification through issue #43.
- Record a fresh paired build/toolchain tuple and retain both target binaries.
- Capture per-operation or kernel timing on the actual CachyOS/ROCm stack before
  another broad low-level kernel sweep.
