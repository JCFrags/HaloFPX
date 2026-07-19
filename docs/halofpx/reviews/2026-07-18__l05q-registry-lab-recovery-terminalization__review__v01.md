# L05q registry-lab recovery terminalization review v01

- Date: 2026-07-18
- Scope: portable fake-only operation-6 admission, recovered ABORT/CLOSE
  terminalization, and restart projection closure under ADR-0023
- Receipt SHA-256:
  `dfa1ff903168375289909ea1621c0a9c977a5a1241f11d7d6f3aa61e6245ce68`
- Final independent verdict: **ACCEPT**

## Delivered boundary

The existing internal `STATIC EXCLUDE_FROM_ALL` registry-lab target now admits
only two mutation actions after operation 5 independently classifies a complete
compatible snapshot:

- `needs_predecessor_abort` executes operation 6 and publishes one exact
  authenticated recovery-class ABORT; and
- `needs_successor_close` executes operation 6, reasserts the already-published
  successor and HEAD durability projections, authenticates both, and publishes
  one exact recovery-class CLOSE.

Operation 6 privately commits the derived action, slot, attempt, PREPARE,
current HEAD, and operation identity. It revalidates the complete action state,
logical budget, and reserve before latching uncertainty and immediately invoking
the first action operation. Post-latch reserve loss or invariant disagreement
cannot produce a definite no-mutation result; authenticated readback
contradictions quarantine, and other failures remain uncertain.

The fake separates file-data durability from namespace durability. Test-private
restart projection enumerates every admitted absent/prefix/complete terminal
outcome without adding an oracle axis to the operation algebra. Recovery never
invents bytes, names, durability, terminal attribution, or a lost positive
disposition.

No public header, option, install/export rule, filesystem syscall, Linux adapter,
product edge, provider, cache, restore path, inference authority, or persistent
write was introduced. Normal CAS and quarantine publication remain closed.

## Adversarial review and repairs

Promotion was paused after independent review found four fail-closed defects in
the initial candidate. The accepted implementation repairs all of them:

1. confirmed operation-6 `unavailable` is independently derived only from an
   action-critical state or commitment mismatch; a clean oracle script cannot
   manufacture that reserved result;
2. unexpected post-latch derived/script disagreement maps to uncertainty,
   except an authenticated readback contradiction, which quarantines;
3. external reserve loss after the operation-6 latch maps to uncertainty; and
4. ADR and target metadata distinguish the 11/19 injectable primitive entries
   from engine-owned cleanup events in the complete trace.

The static contract now pins both exhaustive modes, exact product/projection/
attack counts, restore/reclassification, every-ordinal secret exclusion, the
late-reserve check, post-latch mapping, and clean-state rejection of reserved
operation-6 `unavailable`.

The final independent reviewer recomputed all seven reviewed-source hashes, all
15 retained-evidence hashes, local Release/Debug and live Linux optimized/
sanitizer executable hashes, and the golden-vector hash. It re-ran the Release
contract and core path, checked the exact base/branch/ancestor and zero-remotes
state, rechecked all four immutable references, and returned ACCEPT with no
blocker.

## Qualification

| Gate | Final-candidate result |
|---|---|
| Windows Release configured suite | Pass, 84/84, twice |
| Windows Debug configured suite | 83/84; one proven inherited exception |
| Windows Debug focused L05q test | Pass |
| Windows Release product exhaustion | Pass, 287 admitted and 1,393 forbidden |
| Windows Release L05q exhaustive | Pass |
| Windows Release core repeats | Pass, 1,000/1,000, eight workers |
| Windows Debug core repeats | Pass, 200/200, four workers |
| Windows Release full ordinary repeats | Pass, 8/8, two workers |
| nimo-2 optimized focused tests | Pass, 2/2 |
| nimo-2 optimized archive audit | Pass, 1/1 |
| nimo-2 optimized product/exhaustive modes | Pass |
| nimo-2 optimized core repeats | Pass, 1,000/1,000, four workers |
| nimo-2 ASan/UBSan focused tests | Pass, 2/2 |
| nimo-2 ASan/UBSan exhaustive mode | Pass |
| nimo-2 ASan/UBSan core repeats | Pass, 1,000/1,000, four workers |
| Immutable reference repositories | Clean at locked commits/trees, 4/4 |
| Configured implementation remote | None |

Each exhaustive process covers 3,072 action/slot/history cases, all 1,680 new
operation products, 91 malformed scripts, 10,335 deterministic restart
projections with restore and reclassification, 712 terminal truncations, 5,696
one-bit mutations, and eight recomputed-tag semantic attacks. The repeated
stability layer contributes 3,208 successful candidate processes beyond the
configured and exhaustive single-process runs.

The first Windows 1,000-process orchestration exceeded a 600-second controller
window and produced no receipt, so it is excluded. The completed rerun retained
eight 125/125 worker records but the inherited L05P runner returned a false
final aggregate because its property sum was blank. Independent integer
aggregation validated 1,000/1,000 with zero failed indices; a corrected
L05q-local runner was used for all later repeats.

## Inherited Debug exception

The only configured Debug failure is `test-halofpx-context-store-authority` at
`authority.valid()`. The current binary fails at line 180. A preserved pre-L05q
Debug binary with SHA-256
`1efe1e8ab986b0a397a85db9d7b4d6f6ed252032865b96182cd34ccc60bdff0e`
fails at the same assertion at its line 172. L05q edits and links neither target,
the L05q Debug focused test passes, and the complete Release suite including the
authority test passes. The exception is retained as a baseline issue rather
than attributed to this milestone.

## Wiki, provenance, rollback, and performance

The implementation agrees with canonical Wiki Section 63: corruption or
uncertainty never enters inference, file and directory durability are separate,
immutable publication is required, and invalid restart state becomes recovery,
miss-like rejection, or quarantine. M63-01 concrete Linux syscall/process/
filesystem qualification remains open, and no durability mode or persistence
claim is made.

The slice is target-native. No donor implementation was imported, no GPL
llama-ai code or separately licensed documentation entered the MIT engine, no
CachyLLama code was copied, the direct-cherry-pick roster remains empty, and no
P3 record is required.

The known-good nimo-2 RPC worker remained on port 50052 and nimo-1 health
remained HTTP 200. A real MiniMax M2.7 UD-Q6_K_XL request returned exact
`L05Q_ROLLBACK_OK`. This is an operational rollback smoke, not a matched
performance baseline. Because the target remains excluded with no product
edge, feature-off is the compatibility and zero-regression control; this
milestone makes no speed claim.

## Reusable follow-up improvements

- After this commit exists, add its exact hash and the accepted L05p/L05q
  history to canonical Wiki Section 63; do not pre-claim an uncommitted hash.
- Root-cause the inherited Debug authority assertion in a separate baseline
  milestone without relabeling it as an L05q regression.
- Future special-mode runners should retain command, exit, stdout/stderr, and
  timing directly rather than relying on a hash-pinned summary JSON.
- The legacy `read-only` target/file name now contains bounded fake mutation;
  consider a later mechanical rename without mixing it into authority work.

Exact commands, counts, source/executable hashes, raw logs, reference identities,
orchestration exclusions, rollback evidence, and the Debug baseline exception
are recorded in
`../evidence/l05q-registry-lab-recovery-terminalization-repeat-receipt.json`.
