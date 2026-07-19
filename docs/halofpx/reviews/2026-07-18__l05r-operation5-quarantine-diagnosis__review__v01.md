# L05r operation-5 quarantine diagnosis review v01

- Date: 2026-07-18
- Decision authority: ADR-0024 and canonical Wiki Section 63
- Verdict: **ACCEPT** for the excluded fake operation-5 diagnosis slice

## Outcome

Operation 5 now scans the complete fixed snapshot, accumulates every applicable
diagnosis flag, and selects the exact ADR-0024 precedence only after the scan.
Existing quarantine and authenticated initialization remain the two required
early blockers. Codes 3 and 14 are not reused as ordinary snapshot diagnoses.

Publication eligibility is separate from sticky classification. It requires an
exact, durable, authenticated, compatible initialized marker plus one truthful
U0, UH, or P evidence shape admitted by the selected reason. The current
snapshot cannot reconstruct the old predecessor-HEAD bytes after a successor
HEAD becomes current, so this slice never invents S evidence. It also does not
downgrade an otherwise exact successor observation to UH, P, or U0 merely to
make it publishable.

## Independent adversarial review

The first review returned **REJECT**. It found that the original implementation
treated any valid current HEAD as UH when no unique PREPARE existed. That could
mislabel a successor HEAD as the predecessor-only UH shape.

The repair derives `predecessor_head_evidence` independently from the exact
initialized marker, initial envelope, selected bytes, HEAD content digest, and
selector identity. UH now requires that predicate. Integrated tests cover
successor HEAD with zero, one, and multiple valid PREPAREs; all three remain
shape `none` and nonpublishing when exact S evidence is unavailable. The final
independent verdict is **ACCEPT**.

## Qualification

Windows Release passed the complete 40-test HaloFPX label and 100/100 focused
diagnosis repetitions. Nimo-1 passed the current exact-source Release focused
pair and complete 39-test Linux HaloFPX label. Nimo-2 passed the same current
snapshot under ASan/UBSan with leak detection and halt-on-error enabled. Source
hashes matched across Windows, nimo-1, and nimo-2.

The focused contract fixes all 13 selectable single causes, all 78 ordered
reason pairs, 16 integrated diagnoses, the exact initialized-root gate, and the
absence of operations 69-76. Static and negative-link audits confirm the target
remains `STATIC EXCLUDE_FROM_ALL` with no server-product edge.

## Boundaries and rollback

WebUI stayed disabled. No public header, filesystem adapter, persistent write,
provider, cache, restore, inference, or performance path changed. No GPL
llama-ai or CachyLlama implementation entered the MIT engine. The immutable
reference clones remained at their locked commits and trees, and both known-good
node services remained healthy.

Rollback is removal of the private diagnosis enums/view, complete-scan fault
accumulation, fixture accessor, and focused tests. The next safe slice may use
this plan to prepare event-free quarantine encoding, but operation 69 and every
publication operation remain closed until their separate admission gate passes.
