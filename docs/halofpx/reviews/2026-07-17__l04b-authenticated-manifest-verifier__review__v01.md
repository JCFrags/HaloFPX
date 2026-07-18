---
type: implementation-milestone-review
status: accept
date: 2026-07-17
lane: L04b
parent_commit: 4bc730ed30163e5b657d2bcf974e6feaf72c5a3d
base_commit: 61f2f2d7bc4955e9bca821095ef69125837133b5
base_tree: 0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd
---

# L04b authenticated-manifest-verifier review

## Verdict

**Accept as the second L04 implementation milestone.** The offline verifier
authenticates the exact parser-confirmed manifest bytes, checks trusted authority
and protected replay expectations, and rejects internally corrupt or unexpected
compatibility state. Its only positive result is `authenticated_unadmitted`, a
terminal miss with no profile, codec, payload access, candidate, or hit.

The target remains excluded from the production build graph and performs no I/O,
logging, provider call, context mutation, or persistent read/write.

## Cryptographic and provenance boundary

The target-owned HMAC wrapper compiles the exact selected-base public-domain
SHA-256 source already present under `examples/gguf-hash`. Static tests pin the
three source/header hashes and reject OpenSSL, BCrypt, network/server, donor,
filesystem, provider, state, logging, and `memcmp` dependencies. The wrapper
uses checked bounded spans, RFC 2104 long-key handling, exact NUL-terminated
domains, big-endian lengths/generation, fixed-trip tag comparison, and volatile
wiping of wrapper-owned key blocks, pads, digests, and context objects.

The inherited SHA transform retains local working words, and portable C++ does
not formally prove constant-time machine code. Remanence and Release assembly
qualification on MSVC and GCC/Clang therefore remain closed gates before live
protected keys or trusted hits.

No CachyLLama or GPL llama-ai implementation or separately licensed
documentation entered the MIT engine. The direct-cherry-pick roster remains
empty. Reuse of unchanged selected-base public-domain code is recorded and
hash-locked, not treated as donor admission.

## Independent adversarial review

The first review returned `REVISE` on two security-boundary issues:

1. active key-material presence was checked before unknown, revoked, and
   read-disabled lifecycle rejection; and
2. documentation overstated secret erasure inside the inherited SHA transform.

Lifecycle disposition now precedes active-key material validation, including
tests where rejected records contain no master bytes. Documentation now limits
the wipe claim to wrapper-owned storage and explicitly gates live use on the
unwiped transform locals. Re-review returned `ACCEPT` with no remaining framing,
endianness, bound, HMAC, key lifecycle, authority, replay, compatibility,
constant-time-claim, exclusion, provenance, or test blocker.

## Verification

| Check | Result |
|---|---|
| Clean Windows CPU Release build (`build/halofpx-l04b-clean`) | Pass, all configured targets |
| Explicit excluded format/auth libraries | Pass |
| Clean HaloFPX CTests | Pass, 8/8 |
| Clean focused inherited CTests | Pass, 7/7 including fixture dependency |
| Baseline/candidate `llama-server --help` | Byte-identical direct capture, 55,691 characters |
| NIST/RFC primitive vectors | Pass, including padding boundaries and a 131-byte HMAC key |
| Independent Python `hashlib` manifest goldens | Exact compatibility root, KDF, tag, and manifest digest match |
| Static source-lock/no-hook/no-runtime-link contract | Pass |
| Immutable reference clones | Clean with refs byte-equivalent to locked records, 4/4 |
| Independent adversarial review | ACCEPT after two findings were closed |
| `git diff --check` | Pass; autocrlf notices only |

The clean local build does not claim HIP, Vulkan, ROCm, target-node, live key
authority, state continuation, payload corruption, filesystem defense, crash,
or performance qualification. Feature-off remains the compatibility control.

## Rollback and next gate

Reverting L04b leaves the accepted L04a structural parser and L03a inert seam;
there is no persisted data, runtime configuration, or cache invalidation action.

The next reader work must remain offline and default-excluded while it adds the
immutable object-frame parser and exact length/domain/type/digest rejection.
No filesystem path may be opened until authenticated scope and a fixed trusted
root are available. Provider eligibility still waits for an admitted complete
state profile and codec. Transactional publication and all writes remain a
later independently modeled and reviewed gate.
