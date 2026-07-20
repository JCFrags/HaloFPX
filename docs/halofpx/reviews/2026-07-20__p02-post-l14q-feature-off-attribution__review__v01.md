# P02 post-L14Q feature-off attribution independent review

Date: 2026-07-20

Verdict: **ACCEPT after statistics correction; final G9/G10 remains open**

## Scope and evidence

The review independently inspected the P02 milestone page, machine-readable
receipt, README routing, both node evidence roots and compressed bundles,
matched responses, profile counters, restored services, and immutable reference
repositories. It recomputed representative hashes, all retained summary
statistics, interface deltas, and MPTCP attribution without expanding the test
matrix.

The retained bundles reproduce exactly:

- nimo-1: 43 manifested files, 18,645-byte bundle, SHA-256
  `b2223c697fc9de4d2872f955f0555764abf046e9dc396e23ddb6263e5422b5e2`;
- nimo-2: 84 manifested files, 27,790-byte bundle, SHA-256
  `c2516aadc78ba46926b02baf51dd30dc62f65cd7b37f3f84b00d315c3202a9bd`.

All 16 matched requests returned HTTP 200 with 1129 prompt and 128 generated
tokens. Decoded output is byte-identical. The GPU-use, power, rail-counter, and
prompt-only values agree with raw files. Experimental MPTCP reports one total
subflow and zero thunderbolt1 traffic; the restored deployment reports two
total subflows and one TCP subflow on each rail. Restored service state, binary
hashes, direct health, and clean reference commits/trees are supported.

## Required correction and closure

The initial draft labeled intervals as Welch intervals but used a common
approximately t(10) critical value. Review returned REVISE. The corrected
record uses the Welch-Satterthwaite degrees of freedom independently for each
metric and full-precision JSON values:

- prompt: df 5.664619, critical value 2.482462, interval -0.316180 to
  +0.479305 tok/s;
- generation: df 9.297343, critical value 2.251177, interval -0.031083 to
  +0.041461 tok/s.

The output-hash language was also made representation-exact: `jq -j` hashes
decoded UTF-8 bytes without a terminator, while `jq -r` adds one LF. JSON parses
and `git diff --check` passes after correction. Both corrected intervals still
cross zero, so no performance conclusion changed.

## Promotion boundary

P02 is accepted as a bounded feature-off attribution milestone. The evidence
supports deterministic equivalence, favorable point estimates, and a concrete
single-subflow optimization hypothesis. It does not prove speedup, close final
non-inferiority, establish dual-rail benefit, or authorize transport
replacement. The SIGABRT produced while stopping the pre-existing deployment
is correctly retained as a separate nonblocking operational defect because
the service released memory, subsequently reloaded, and passed direct health.

No correctness, security, rollback, provenance, licensing, or claim-discipline
blocker remains for this documentation commit. Additional trials belong to the
final G9/G10 gate, not P02.
