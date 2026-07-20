# L05z fault-controller checkpoint independent review

**Result: ACCEPT_FOR_CHECKPOINT_CONTINUATION_NOT_PROMOTION.** The frozen
test-only controller checkpoint, both 350-cell crash matrices, and the listed
checkpoint regressions are accepted as an intermediate qualification result.
L05z promotion remains closed.

## Frozen boundary

The checkpoint extends committed L05z focused core
`6c00a05cd03f5c05819c2791f494e28b6a7d0855` / tree
`e9ac265d7530e5256cff8d59db31f05d8e802c0d`. The receipt pins the exact
SHA-256 identities of the ptrace controller (`26ffe6bc...`), returned-fault
controller (`8bf8f48f...`), hostile manifest (`3c80ec79...`), CMake boundary
(`da5839b1...`), anchor audit test (`3977a7ea...`), initializer anchor
(`a94c08e5...`), and seam contract (`fcb9889e...`). These are Linux x86-64,
test-only, excluded targets. They add no product, install, export, default CTest,
persistent-write, production fault-hook, or dependency edge.

## Accepted two-node crash evidence

nimo-1 passed 350/350 exact-production crash cells: 25 repetitions of every
entry and exit boundary for each of seven mutation families. The retained root
is
`/var/tmp/halofpx-qualification/l05z-ptrace-scale25-26ffe6-final-20260719-nimo1`.
It contains 350 unique run identifiers and 350 unique Btrfs UUIDs, 801,550
JSONL events, and 122,165,668 receipt bytes. The frozen controller binary is
`f4aeae2e...` and the target binary is `db5a7eeb...`.

The top A/B evidence bundle is 26,870,152 bytes with SHA-256
`0d7c52e664d39acd420108aa86db70a612a2cf469ecfb72e8b6a6be58d75f862`.
Deterministic reconstruction matched 5,756/5,756 entries with zero mismatch.
No qualification residue remained. `llama-server` remained healthy as PID 971
on port 8081 with HTTP 200 and reported `NRestarts=0`.

nimo-2 independently passed the same 350/350 matrix at
`/var/tmp/halofpx-qualification/l05z-ptrace-scale25-26ffe6-final2-20260719-nimo2`.
It retained 801,550 JSONL events and 122,917,454 receipt bytes. Its 45,931,231
byte top A/B bundle is
`19f25e64a6b5c8cda3bb74057918867300bc6d21623efbbe654492ddf1d8349d`;
deterministic reconstruction matched 7,122/7,122 entries, and the evidence seal
is `ddff038eb27801c9e168e03c3fe5266d9f3b88997e50529ec80afb164b4c126e`.
No residue remained. `ggml-rpc-server` remained continuously reachable as PID
3562775 on port 50052.

The accepted two-node crash total is therefore 700/700:
`2 nodes * 7 families * 2 phases * 25 repetitions`.

## Checkpoint regressions

nimo-1 passed a clean 520/520 build, the full 48/48 HaloFPX suite, the focused
6/6 L05z suite, seven of seven inherited controls, and the one-of-one
feature-off contract. Their retained log hashes are respectively represented
by the build result and SHA-256 values
`97739d7d24819be6567e4c8df75ca1ba5b9756e227b46b1edd635239d8a86b70`,
`3c4cb8bc4b71a1001da89dd3b5e5a6e9cd2a91cad9a5c45f2cbab0e41b872133`,
`8ee7285db43344d0597aecc31aa0a7f5e0c651c0abffb31626b9583dd2e0863b`,
and
`fef5acdfad04e4c11dabbf01673a3c77ffafe5e2bc723b959147e3afa40859dc`;
the evidence bundle is
`fa7a9b9be8c2a31a9517a34c81f0427b293b46b2805ffcb8fcb23d217abd24d1`.

Windows passed 3/3 distinct context-store-authority contracts. The one
deliberate CRT assertion probe preserved assertion text on stderr and a
nonzero exit, while no WerFault UI appeared. The log is
`29364a129d14f4e777b0e83a4b8e5c2dae3e02d23c16f57d10c9fc04a7470266`
and the bundle is
`f9bcbc48ad6e57cda6120f2687b5ce75d4ebd28c3567b6914f40aecc12508d12`.
This verifies the Windows CRT test boundary without weakening the failing
assertion.

All four immutable reference clones remained clean at their locked commit and
tree identities recorded in the receipt.

## Returned-fault checkpoint

The returned-fault source is frozen, but execution is not accepted. Its
canonical hostile manifest contains exactly 1,899 cases. The sorted case-ID
set hash is
`1028ac1be238cd418e48699c509f27a0dfcded792869c4014a9f5f6d9a8e8698`,
and the canonical manifest hash is
`5c7a84a0bcf7e2bad3ad98e988d88c898137bee97c1c66023e8bea8a1c15d213`.
The controller closes its arithmetic at 8,468 cases: 2,087 inherited syscall
compatibility cases, 4,482 L05z syscall-role profiles, and 1,899 hostile-input
cases. This is manifest and oracle closure only; zero returned-fault execution
cases are promoted by this checkpoint.

## Remaining gates and nonclaims

Both nodes must execute the complete 8,468-case returned-fault matrix in the
admitted Release and sanitizer lanes, including full target-plus-controller
ASan/UBSan coverage on nimo-2. Storage, response, cleanup, audit, retry, and
scale evidence remain pending. The final frozen source must then receive final
inherited and feature-off Windows/Linux regression reconciliation, provenance
and immutable-reference reconciliation, service and rollback checks,
evidence-bundle reconstruction, and a separate independent L05z promotion
review before Wiki promotion.

This review makes no returned-fault execution, sanitizer, final full-regression
reconciliation, final-provenance, persistent-write, initialization-completion,
HEAD-publication, cache-hit, restore, distributed, inference-performance,
zero-regression, or final L05z acceptance claim.
