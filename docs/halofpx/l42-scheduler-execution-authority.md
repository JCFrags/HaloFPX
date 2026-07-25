# L42 — scheduler execution-authority foundation

Result: **PASS**

Base: `ba0cbd51634cd58496d35cf615dfdae32a367269`

L42 corrects the four material L41 review gaps without opening mutable-input
census, SET_TENSOR roles, RPC redesign, cache semantics, primary testing,
production mutation, or performance work.

## Accepted contract

The ggml scheduler now has an explicit, runtime-default-off authority mode. An
admitted caller supplies a bounded event buffer, attempt nonce, HMAC key,
version, maximum events, and nonzero execution sequence. Enabled execution
exports canonical little-endian authenticated records for:

- pre-mutation graph tensor identities and complete source/view edges;
- per-node backend assignments;
- exact split ordinal, backend, node range, and ordered inputs;
- exact copy identity `(source canonical ID, destination backend, copy slot,
  generation)`, consumer node/source slot, source and generated-destination
  layouts, and view authority;
- ordinary and expert-partial source-before/destination-after execution,
  including allocation ordinal, buffer-relative allocation and transfer range,
  `type/ne/nb`, view edges, synchronized physical/logical digests, logical byte
  count, and separately labeled padding;
- an authenticated trailer and result receipt containing exact counts, root,
  nonce, sequence, and tag.

Every event is bounded and chained. Unknown types/references, malformed or
out-of-order records, duplicate indices, overlapping or out-of-bounds ranges,
unsupported readback/layout, arithmetic overflow, misaligned quantized ranges,
missing pairs, digest mismatch, or export exhaustion fail closed.

Feature-off execution retains the original scheduler branches and performs no
diagnostic allocation, traversal, synchronization, readback, or event export.

## Qualification

One evidence session used two executions because the scheduler's expert
optimization is selected only when the split's first node is `MUL_MAT_ID` with
host-resident weights. Combining it with the ordinary nested-view/CPU-consumer
graph would change first-node/split authority and could suppress the exact
expert branch being qualified.

The session proved:

- ordinary: a real RPC/ROCm-to-CPU split and copy of a nested/view-derived
  range, exact authenticated 16-event transcript, one copy map and verified
  copy, expected deterministic output;
- expert: a real `MUL_MAT_ID` host-weight-to-RPC/ROCm branch, exact
  authenticated 12-event transcript, two non-overlapping partial ranges with
  256 logical and 256 explicitly labeled padding bytes each, exact
  source/destination physical and logical equality, expected deterministic
  output;
- live hash fixtures: nested and strided F32 logical ranges, Q8_0 aligned
  hashing and misalignment refusal, transferred-padding labeling, and
  out-of-bounds refusal;
- independently re-signed semantic refusal records for duplicate, order,
  unknown type, overlap, and out-of-bounds cases, plus malformed and HMAC
  tamper refusal;
- feature-off output equality and absence of an authority result.

Raw transcripts, receipts, logs, and hashes are retained under
`docs/halofpx/evidence/l42-raw/`.

## Closeout

Independent adversarial review returned PASS. The disposable loopback worker
and port `50242` were removed. Production was never mutated. At closeout the
nimo-2 system worker was active on `50052` with MainPID `1535639`; the nimo-1
system coordinator was active on `8081` with MainPID `2356329` and HTTP `200`;
both reported `NRestarts=0`.

L42 does not authorize primary testing, cache promotion, or L43.
