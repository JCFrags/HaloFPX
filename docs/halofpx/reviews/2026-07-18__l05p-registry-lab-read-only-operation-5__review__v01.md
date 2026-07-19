# L05p registry-lab read-only operation 5 review v01

- Date: 2026-07-18
- Scope: portable fake-only operation-5 decoder, authenticated binder, and
  recovery/request classifier under ADR-0022
- Receipt SHA-256:
  `eda5217418090a3fa1be174ef1329daee794743f45b60430977fff4248b5c4e2`
- Final independent verdict: **ACCEPT**

## Delivered boundary

The existing internal `STATIC EXCLUDE_FROM_ALL` target now executes operation
5 against the immutable operation-4 snapshot. Five private kind-specific
decoders enforce the normative deterministic-CBOR and authentication contract,
then a complete fixed-layout scan derives the eleven-class precedence. A clean
request reaches only private event 201, the operation-6 test boundary. No
ordinary authority result, mutation, filesystem adapter, provider, server,
cache hit, restore, or persistent write is introduced.

The implementation remains linked only to the target-native registry-lab wire
archive. It adds no public header, option, install/export rule, product edge,
donor format, or runtime dependency. Feature-off product behavior therefore
remains the compatibility control.

## Adversarial review history and repairs

Promotion was held across three independent rejection rounds. The accepted
candidate repairs every finding:

1. the HMAC comparison is a dedicated fixed-32-byte routine with an exact
   Windows COMDAT-to-disassembly proof and a Linux symbol-bound optimized
   proof, each protected by synthetic early-branch, helper-call, and wrong-
   bound negative controls;
2. authenticated carriers are privately constructible and immutable outside
   successful decoding;
3. registry IDs stop at the normative 128-byte maximum and fixed request
   buffers require canonical zero tails before entry;
4. integrated precedence, every staging/envelope position, authenticated
   structural/semantic mutations, and dependent one-CLOSE history faults are
   exercised rather than inferred from invalid-tag rejection;
5. the receipt and exact-candidate Windows/Linux evidence were recollected
   after every source correction; and
6. both normative terminal classes are admitted for both CLOSE and ABORT while
   their kind-specific phases remain exact. Authenticated positive histories
   explicitly cover recovered CLOSE and normal ABORT; invalid-phase cases
   remain fail-closed.

The final independent rereview directly recomputed every reviewed-source and
raw-evidence hash, inspected the exact executable identities and logs, checked
the four immutable reference clones, and returned ACCEPT with no blockers.

## Qualification

| Gate | Corrected-candidate result |
|---|---|
| Windows Release registry-lab matrix | Pass, 7/7 |
| Windows Debug registry-lab matrix | Pass, 7/7 |
| Windows full configured Release suite | Pass, 84/84 |
| Windows Release core repeats | Pass, 1,000/1,000, 8 workers |
| Windows Debug core repeats | Pass, 200/200, 4 workers |
| Windows Release full hostile-corpus repeats | Pass, 8/8, 2 workers |
| nimo-2 optimized registry-lab matrix | Pass, 6/6 |
| nimo-2 ASan/UBSan registry-lab matrix | Pass, 6/6 |
| nimo-2 ASan/UBSan core repeats | Pass, 1,000/1,000, 4 workers |
| nimo-2 ASan/UBSan full hostile-corpus repeats | Pass, 10/10 |
| Immutable reference repositories | Clean at locked commits/trees, 4/4 |
| Configured implementation remote | None |

The functional matrix covers all eleven classifications, twelve admitted
primitive products, 55 pure pairwise and 11 integrated precedence overlaps,
all 512 replay and late-corruption positions, 2,048 staging projections, 1,536
immutable-envelope positions, 178 projection/namespace faults, 53 recomputed-
tag hostile semantic/structural/history cases, 2,085 truncations, and 16,680
one-bit mutations per full-corpus process. Sanitizer and optimized comparator
qualification are intentionally separate so instrumentation cannot weaken the
compiled-control-flow proof.

Exact paths, counts, source/executable hashes, commands, retained raw logs,
reference identities, rollback evidence, and the excluded orchestration
attempt are recorded in
`../evidence/l05p-registry-lab-read-only-operation5-repeat-receipt.json`.

## Wiki, provenance, rollback, and non-claims

The candidate matches ADR-0022, the normative
`../contracts/context-store-registry-lab-v1.cddl`, and the accepted target-native
wire contract. Unsupported, malformed, unauthenticated, incomplete,
incompatible, contradictory, or ambiguous state fails closed to a miss-like
result or sticky-quarantine classification; no partial restore is possible.

No donor implementation was used. No GPL llama-ai implementation or
separately licensed documentation entered the MIT engine; no CachyLLama code
was admitted; the direct-cherry-pick roster remains empty and no P3 record is
required for this target-native slice.

The known-good nimo-2 RPC worker and nimo-1 model server remained available,
health returned HTTP 200, and a real completion returned exact
`HALOFPX_ROLLBACK_OK`. Its observed generation rate is only an operational
smoke signal, not a matched baseline or performance claim.

Acceptance proves only the portable fake-only read path through operation 5.
Operation 6, recovery or quarantine mutation, Linux filesystem behavior,
persistent writes, cache reuse, production credential custody, runtime
compatibility, and performance non-regression remain closed behind their
documented gates.
