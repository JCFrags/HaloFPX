# L14Q-T01 standard quantized-KV FlashAttention test coverage

L14Q-T01 is a target-native, test-only qualification milestone for standard
Q8_0 and Q4_0 K/V cache types. It expands the existing
`FLASH_ATTN_EXT` backend-operation inventory at Strix Halo head dimensions and
boundaries without changing a kernel, backend selector, build graph, runtime
default, command-line interface, service, or deployed binary.

The exact implementation parent is HaloFPX
`051084fa3ab724cd290f864c093ff67f16e13a90`, tree
`b2fed43eeb5ccba9286638cf024d39452bf697e0`. The source-base authority remains
ROCmFPX `61f2f2d7bc4955e9bca821095ef69125837133b5`, tree
`0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`; this milestone does not advance
that locked base.

## Admitted test inventory

The only modified path is `tests/test-backend-ops.cpp`. Its parent blob is
`5e712d7271f23e4ebff14b60bf234f8b7e4d394a`; the candidate blob is
`543c26432318b243e3412bb1aff2f8874e67a963`, with SHA-256
`daf6a931f1bee54aeaaaf0b9a8002f3a778598211c4849d73def9f3f038bf292`.
The diff adds 30 lines and deletes none.

The positive inventory is the Cartesian product of:

- symmetric K/V types Q8_0/Q8_0 and Q4_0/Q4_0;
- head dimensions 128 and 256; and
- five target-owned shapes: `(nr2=1, kv=255, nb=1)`,
  `(1,256,1)`, `(1,257,1)`, `(8,256,1)`, and `(8,256,9)`.

This yields exactly 20 executable positive cases, including the 255/256/257
boundary, GQA ratio 8, and a multi-batch cell. One additional ROCm-specific
negative case uses head dimension 160, GQA ratio 8, KV length 256, batch 1,
and Q8_0/Q8_0. Its contract is an explicit documented rejection, not an omitted
or silently passing test.

## Provenance boundary

The approved P3 authority is preparation commit
`f6c56a45cbd365d2208bcb5a65c0bd6afce62be7` and capability record
`L14Q-T01`. It derives only a behavioral coverage requirement from Nathan
Wilson's MIT-licensed commit
`6b03608e63f48c9371bf5f00423da413ac0288de` in the preserved
`Nathanw1014/llama.cpp` intake. The donor result blob is
`1cbd27cb4d163fbc9970b57e0886a38adf62542f`.

The implementation is a clean, HaloFPX-owned reimplementation. No donor line,
comment, loop, literal case table, kernel, dependency, patch, or binary entered
the engine. The direct-cherry-pick roster remains empty. The retained source
bundle SHA-256 is
`79c61718bd60ecce3e5ea3919fb18d8c709b3d26032c5fe9d4f24055ade3bc3f`;
the preserved combined patch SHA-256 is
`66ddb0e33301ab7231c736be318483e90cfba813701fbeb5269f547a65c4f42d`.
Neither artifact was applied.

The independent similarity analysis found zero exact or whitespace-normalized
matches across the 28 target nonblank added lines and the donor commit's 14
nonblank additions (15 additions including one blank line). Comment overlap was
zero and the longest common token run was nine. Manual structure review found
no copied or mechanically translated donor comment, name, ordering, literal
case table, or loop structure. The disposition is **APPROVE:
target-native/no-copy**.

## Qualification results

Both Strix Halo nodes completed the focused matrix separately for CPU, ROCm,
and Vulkan: 200/200 positive executions per backend per node, with zero
failures. Nimo-1 also completed an additional combined CPU 200/200 run. The
explicit ROCm negative was observed as unsupported for the intended reason.

The inherited full `FLASH_ATTN_EXT` inventory completed separately on each node
with identical counts and zero failures in every admitted backend class: CPU
reported 5,136 supported cases; ROCm reported 2,919 supported and 2,227
not-supported cases; Vulkan reported 5,132 supported and four not-supported
cases. Unsupported counts are backend capability outcomes, not failed
correctness cases.

Nimo-1 completed the 581-target CPU build and the applicable Strix backend
build, plus its feature-off controls. Nimo-2 completed the same 581-target CPU
build and applicable Strix build. Its promoted feature-off subset passed 79/79:
78 non-tokenizer tests plus the tokenizer test executed separately. A full
nimo-2 CTest launcher attempt reported 79/95 because 16 tokenizer invocations
were corrupted by a CRLF launcher artifact and returned rc 8; those launcher
results are retained but excluded. A separate nimo-1 CRLF launcher artifact is
also retained and excluded.

Raw evidence is retained on each originating node. The exact bundle identities,
binary hashes, service-continuity evidence, excluded artifacts, and
reconciliation fields are pinned in
`evidence/l14q-t01-quantized-kv-fa-test-coverage-receipt.json`.

## Nonclaims and rollback

L14Q-T01 makes no inference-speed, latency, memory, runtime-optimization, or
zero-regression claim. It does not implement or admit the donor HIP tile path or
Vulkan coopmat1 path, does not cover ROCmFPX K/V formats, and does not alter
TurboQuant or ROCmFPX FlashAttention routing. Large MiniMax weights are not
changed or admitted by this test slice.

There is no runtime, kernel, CMake, performance-default, NOTICE, SBOM, service,
deployment, or persistent-state change. Rollback is the deletion/revert of the
single 30-line test insertion, restoring parent blob `5e712d7271f23e4ebff14b60bf234f8b7e4d394a`.
All exact node binary identities are reconciled. The independent promotion
review accepted this test-only milestone with no findings.
