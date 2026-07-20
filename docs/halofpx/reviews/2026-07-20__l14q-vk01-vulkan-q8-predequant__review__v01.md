# L14Q-VK-01 Vulkan Q8_0 pre-dequant independent review

Date: 2026-07-20

Verdict: **ACCEPT FOR A DEFAULT-OFF EXPERIMENTAL MILESTONE.**

The initial review found one P2: candidate scratch growth used the inherited
preallocator, which destroys an old buffer before a throwing allocation and
could terminate instead of falling back. The corrected implementation allocates
both replacements before changing context state, catches Vulkan and host
allocation failures, destroys only unpublished temporaries, synchronizes the
active context directly, and swaps buffers only after dual success. Candidate
types and F16 tuning are applied only after reservation succeeds. Qualification
sentinels proved both successful admission and recoverable forced-OOM fallback,
and neither remains in the final diff.

The final review found no P1/P2 blocker. Eligibility is narrow and default-off;
checked arithmetic covers K/V, masks, descriptor ranges, and conservative split
scratch; canonical F16 destination strides and dense-permuted conversion are
consistent; synchronization fences conversion before FA and scratch reuse after
dispatch; and TurboQuant/ROCmFPX authority is preserved.

The no-copy comparison found no donor shader addressing, comment, identifier,
or control structure in the target-native implementation. The preserved donor
clone remained clean at `a18067a85e986f7798f43d98345ed5b86b55cf88`, tree
`130e9cac828f8d8ef877d87ea9c192e24b07c9af`.

The feature-off and focused evidence is sufficient for this bounded commit, not
for runtime promotion or a speed claim. Matched primary-model performance,
broader telemetry, and final non-inferiority remain open. Rollback is the
already-default OFF build or coherent revert.
