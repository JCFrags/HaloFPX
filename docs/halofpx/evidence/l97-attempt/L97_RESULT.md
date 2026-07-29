# L97 terminal result

Status: **NOT PROMOTED; no retry**.

## Proven identities

- accepted base: `8c25e2a160655c4bdebb3fc742301e615b49ad1c`
- source root: `ad299abc42f24fa8ea969de20bbb00caa26d0f18189ac5e4fc491534938876fc`
- build ID: `1aa1b72a6d5ecb0b38efaca6f0622efae0b1cf6595f544d89bd2edb1b96c6d3b`
- controller SHA256: `43a17a8760d501fd99e3705949543f86b4a4372e0d83c6bd3eae024dc6dcb299`
- staged-runtime gate SHA256: `bd061d30dae0b4affa97bdfba924022be23393e19b069deefcd06fea91b899b5`
- worker SHA256: `67917be33faf0108ab7833bbd88f4e04ec630d29e23a356d76b06abc669f44e6`
- canary SHA256: `6bc92bbc2461af6d1abb82ffefb8175ec295bfefbc65c9b398b09f93f0e61201`
- model SHA256: `96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`

The pre-mutation staged-runtime package gate passed. The provenance commands
were the canonical sanitized loader/startup executions; no unsupported
`--help` success contract was used.

## Runtime boundary

The single authorized transition completed authenticated residency-A capture.
The reference token was `21549` and the retained suffix was `alpha`. Four
capture server authorities were retained. A genuinely fresh residency-B worker
then launched, admitted, executed, and published one additional server
authority.

Residency-B emitted token `9283`, but the controller refused before accepting
the result:

`durable and emitted result authority differ`

The durable authenticated record and emitted result line agree on all populated
fields. Their exact difference is that the durable record contains
`"prompt_chunk_sizes": ""`, while the text parser's `([^ ]+)` expression cannot
represent an empty value and omits that field. This is a source-proven
controller result-authority comparison defect, diagnosed read-only after the
terminal boundary. It is not corrected in L97.

The later `halofpx-l48-canary-restore InvocationID changed before evidence
collection` cleanup/evidence failure is separate. Retained evidence does not
prove that it caused the earlier result-authority refusal.

L97 establishes neither restored-token equality nor cache correctness. No
performance claim is made.

