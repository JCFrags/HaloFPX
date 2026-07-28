# L73 focused pre-runtime source review

Verdict: **PASS**

No P1/P2 was found in the exact two-file diff at base
`11f4b50fc48cdb52570d55d4d054c5470d95f13e`.

The reviewer confirmed that, within each destination backend, REGISTER now
sorts before EXCLUDE and the remaining ordering keys stay pointer-independent
and unchanged. The immutable canonical vector remains the sole source for
prepared-admission counts/root, exported iteration, and runtime L44
register/exclude calls. Grammar v1, APIs, duplicate collapse, and conflict
refusal are unchanged.

The intentionally interleaved self-test proves a contiguous register block
followed by a contiguous exclude block; existing duplicate/conflict bits
remain, and the exact mask is `0xfffff`. Required Linux targets built.

The full scheduler test binary also reported an unrelated
`expert_transcript=0` after refusing an unclassified expert graph leaf. Every
L73-relevant ordering, feature-off, composition, refusal, split, hash, and
evidence gate passed. The reviewer classified the expert-only result as
pre-existing test debt outside the two-file L73 change and approved the single
authorized feature-on runtime attempt.
