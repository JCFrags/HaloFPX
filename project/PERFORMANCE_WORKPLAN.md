# HaloFPX Performance Work Plan

Status date: 2026-08-12. This is a living work ledger, not a performance claim.
Accepted Project Lead/source decisions are the work authority. GitHub issues
are subordinate execution trackers; exact experiment records are the
measurement authority.

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

## Ordered work

### 1. Restart cache correctness

Close GitHub issue #14 with a small CPU fixture: process A saves and exits;
process B obtains the exact hit and deterministic continuation; same-size
corruption and compatibility mismatch must miss and recompute. Keep this
correctness qualification separate from target performance. The existing
full-v1 exact-key context-store path is the implementation under qualification;
the run-local `--cache-disk` spill path is not restart-persistent.

### 2. Measurement visibility

Add structured phase measurements for cache capture/synchronization,
serialization, integrity hashing, write/sync/publish, verification, decode, and
install. Extend the matched target harness so every condition binds and hashes
both distributed binaries, retains failures, and reports TTFT, inter-token
latency, prompt/decode rates, cache source, bytes, and avoided tokens.

### 3. First low-risk target screens

- Compare `n_batch/n_ubatch` `512/512` with `2048/512` while holding every
  other variable fixed. The 1,129-token stress prompt currently crosses three
  outer decode calls at `512/512`; the larger outer batch may reduce repeated
  scheduler and RPC setup without changing microbatch size.
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
on one pinned, low-priority CPU thread on each target. This makes an optional
server-local EVP provider the first bounded candidate, but it does not yet
measure the bundled helper or a cache request. See the
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

### 6. Generation

The current dual-node token path is structurally serialized across remote and
local graph splits. Model-free n-gram speculation can amortize those turns only
when drafts are accepted. Repair and qualify it on repetitive code/prose plus a
non-repetitive control, require exact greedy output parity, and retain accepted
draft counts and target-verification latency. MiniMax n-gram acceptance is
currently **[OPEN]**; retained slower n-gram results apply to a Qwen ROCmFP4
workload, not MiniMax. This is workload-dependent and must remain opt-in unless
evidence supports adaptive use.

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
- Add a small ordinary ROCmFPX GGUF fixture registry with source, license,
  converter commit, exact hashes, architecture, and role.
- Record a fresh paired build/toolchain tuple and retain both target binaries.
- Capture per-operation or kernel timing on the actual CachyOS/ROCm stack before
  another broad low-level kernel sweep.
