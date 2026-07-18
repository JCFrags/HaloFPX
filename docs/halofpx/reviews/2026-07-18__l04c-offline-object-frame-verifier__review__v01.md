---
type: implementation-milestone-review
status: accept
date: 2026-07-18
lane: L04c
parent_commit: 2d2c58e5be2f6bd482661d4d6fd39caf2b23aea0
base_commit: 61f2f2d7bc4955e9bca821095ef69125837133b5
base_tree: 0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd
---

# L04c offline object-frame-verifier review

## Verdict

**Accept as the third L04 implementation milestone.** The excluded offline
target verifies a caller-owned immutable object frame only after an L04b result
authenticates the exact descriptor. Its only positive disposition is
`object_verified_unadmitted`; there is still no codec, state restore, candidate,
hit, filesystem read, writer, or production-server hook.

The exact frame has a fixed magic and domain, bounded ASCII stream type, u64be
payload length, exact EOF, and whole-frame SHA-256 identity. Caller-supplied
positive frame and payload limits remain mandatory. A zero-byte payload is
structurally valid but remains unregistered and unadmitted.

## Independent adversarial review

The first review returned `REVISE` on two high-severity boundary defects and two
medium evidence/test defects:

1. zero-length payloads were rejected despite the accepted ADR permitting them;
2. the verifier accepted a publicly forgeable aggregate authentication result;
3. L04b provenance text still described the pre-split SHA target; and
4. Release fixture assertions were disabled by include order.

The implementation now accepts and independently hashes the 38-byte empty
frame. The L04b success result is non-aggregate and retains its authenticated
carrier, object count, and references privately; only const getters expose
references, and only the friend verifier can populate success state. Positive
and malformed-frame tests traverse genuine signed L04b manifests with assertions
active in Release. The SHA documentation and target contracts were corrected.

Final re-review returned `ACCEPT` with no remaining correctness,
authentication-binding, framing, bounds, provenance, default-off, regression,
or test-integrity blocker.

## Verification

| Check | Result |
|---|---|
| Clean Windows CPU Release build (`build/halofpx-l04c-clean`) | Pass, all configured targets |
| Clean HaloFPX CTests | Pass, 10/10 |
| Clean focused inherited CTests | Pass, 7/7 including fixture dependency |
| Baseline/candidate `llama-server --help` | Exact-equal direct capture, 55,691 characters; SHA-256 `37f76df4a726e5ee065ae32fb256fcd78f8705980f0d6cecfffdafdca0e4f742` |
| Empty and one-byte independent object goldens | Pass |
| Structural, bound, corruption, mismatch, truncation, concurrency tests | Pass |
| Static source-lock/no-hook/no-runtime-link contract | Pass |
| Immutable reference clones | Clean with line-exact refs equal to locked records, 4/4 |
| Independent adversarial review | ACCEPT after all four findings were closed |
| `git diff --check` | Pass; autocrlf notices only |

The clean local build does not claim HIP, Vulkan, ROCm, target-node,
filesystem-reader, partial-I/O, codec, state-continuation, crash, durability, or
performance qualification. Feature-off remains the compatibility control.

## Rollback and next gate

Reverting L04c leaves the accepted L04b authenticated manifest verifier and L04a
structural parser. No persisted object, runtime configuration, or invalidation
action exists.

Before any writer implementation, the L02 publication state machine and crash
boundaries require the documented model check and a separate adversarial review.
A filesystem reader must first add trusted-root opens, file-identity checks,
bounded streaming, short/late-I/O handling, and object-set completeness while
remaining offline and default-excluded.
