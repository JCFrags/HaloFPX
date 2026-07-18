# ADR-0001: complete-state admission

- Status: accepted for L02
- Date: 2026-07-17
- Resolves: `OPEN-STATE-01`

## Decision

A persistent hit is valid only under an explicitly admitted, versioned state
profile. A profile is an allowlist that identifies every required stream,
codec, producer ABI, and logical restore position for one bounded behavior
domain. L02 admits no profiles or codecs.

Every future profile, without exception, requires the exact canonical token
sequence, sequence/logical position, output-commit boundary, compatibility
root, and logical ownership. Architecture- or mode-specific target, draft,
recurrent, MTP, speculative, sampler, grammar/tool, and RNG streams must be
profile-required unless a named reconstruction rule has independently proven
behavioral equivalence for that exact mode.

The registry can describe tokens, sequence metadata, target attention/MLA/
sliding-window/recurrent memory, draft or MTP memory, speculative controller,
sampler and logits processors, grammar/parser/tool state, RNG state/counter and
seed provenance, adapters/projectors/multimodal state, request/session/output
boundary, and distributed ownership. A descriptor binds type, schema, producer,
required status, exact length, SHA-256, logical position, rank/layer ownership,
and compatibility root.

Restore validates the complete manifest and every required stream before any
mutation of a live context. Missing, extra, unknown, stale, ambiguous,
partially decoded, corrupt, or incompatible state is a miss and recomputation.
Tensor shape or token-prefix similarity never substitutes for exact admission.

A later profile may explicitly classify a stream as reconstructible only after
tests prove behavioral equivalence for the named mode. It must not call a
restart exact when sampler, RNG, grammar, tool, recurrent, MTP, or speculative
state that can affect output is absent. Arbitrary mid-generation stochastic
restart remains unsupported until independently qualified.

## Admission gate

Each codec/profile needs a target-owned specification, golden vectors, bounded
parser tests, cold-versus-restored exact-continuation tests, malformed/corrupt
tests, upgrade and rollback tests, ownership, and independent review. ROCmFPX
native state is the authority. Donor structures are behavioral evidence only
unless an approved P3 record permits a particular unit.

## Consequence

The complete required-state question is closed by a fail-closed registry
contract rather than an unsupported claim that all runtime state is already
known. Useful subsets can be added one at a time without accepting partial
state as a continuation hit.
