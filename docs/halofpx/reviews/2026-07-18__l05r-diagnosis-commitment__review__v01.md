# L05r quarantine diagnosis commitment review v01

- Date: 2026-07-18
- Decision authority: ADR-0024 and canonical Wiki Section 63
- Verdict: **ACCEPT** for the excluded event-free diagnosis-commitment slice

## Outcome

Every publishable fake operation-5 diagnosis now retains its exact authenticated
root, path-policy, registry ID, and registry epoch revalidation scope. It derives
the ADR-0024 diagnosis commitment over the exact ordered 12-key deterministic-
CBOR map and the domain string including its terminating NUL. The commitment is
bound to the immutable invocation ID. Unsupported reason/shape pairs, incomplete
scope, malformed optional fields, stale hidden values, or an invalid slot fail
closed and leave a zero commitment.

This slice deliberately stops before event acquisition. The current engine
still admits only its exact 5-, 11-, and 19-operation scripts, none containing
operations 69 through 76. The commitment, diagnosis plan, registry scope, and
scratch remain private to the excluded fake registry-lab target and do not enter
restart images, traces, results, public headers, or server/product links.

## Independent adversarial review

The initial live snapshot did not compile because registry-ID validation was a
local lambda outside the new commitment helper's scope. It was repaired by
promoting the identical validation to one private helper used by both preflight
and commitment validation. The reviewer then rebuilt and reran the focused
contract and returned **ACCEPT** with no remaining actionable finding.

The independent review confirmed the exact map size and key order, shortest
integer and length encoding, explicit nulls, domain-plus-NUL hashing, invocation
binding, authenticated initialized-root gate, and the complete 25-admitted /
39-rejected reason-shape matrix. It also confirmed truthful integrated U0, UH,
and P evidence, pure serializer coverage of S, and continued script-level
unreachability of every publication operation.

## Qualification

Windows Release passed the complete 40-test HaloFPX label and 100/100 focused
diagnosis repetitions. Nimo-1 and nimo-2 were qualified from isolated trees
containing the exact same four source/contract files; nimo-1 used a clean Linux
Release build and nimo-2 used ASan/UBSan with leak detection and halt-on-error.
The exact pass counts, elapsed times, hashes, and service probes are retained in
the paired evidence receipt.

The independent oracle covers all 64 reason/shape combinations, malformed
scope and optional-field cases, pure S serialization, ten field-sensitivity
axes, and fixed integrated UH/U0/P golden commitments. Static contract checks
pin the map, domain, matrix, vectors, and absence of operations 69 through 76.

## Boundaries and rollback

WebUI stayed disabled. No filesystem adapter, persistent write, provider, cache
hit, restore, inference, or performance path changed. No GPL llama-ai or
CachyLlama implementation entered the MIT engine, so no P3 admission record is
required. The immutable reference clones remain at their locked commits and
trees, and the known-good node services were not restarted or redeployed.

Rollback is removal of the retained diagnosis scope, deterministic commitment
encoder/validator, private test accessor, oracle, and focused contract markers.
The next milestone may open operation 69 only if it simultaneously supplies the
move-only witness, authenticated encoding and self-verification, action
commitment, operation-6 witness consumption and exact state revalidation, and
the immediate operation-70 transition required by ADR-0024.
