# L14Q-T01 quantized-KV FlashAttention test-coverage independent review

**Result: ACCEPT. Independent review found no correctness, clarity, rollback,
license, provenance, or evidence-reconciliation finding.**

## Scope reviewed

The candidate is anchored to parent
`051084fa3ab724cd290f864c093ff67f16e13a90`, tree
`b2fed43eeb5ccba9286638cf024d39452bf697e0`, and preserves the locked ROCmFPX
base. Its only source change is a 30-line insertion in
`tests/test-backend-ops.cpp`, from blob `5e712d7271f23e4ebff14b60bf234f8b7e4d394a`
to `543c26432318b243e3412bb1aff2f8874e67a963`. No runtime, kernel, CMake,
feature default, command-line, service, NOTICE, or SBOM path changed.

The target-owned inventory contains 20 positive cases: Q8_0/Q8_0 and
Q4_0/Q4_0, head dimensions 128 and 256, KV lengths 255/256/257, GQA ratio 8,
and single- and multi-batch shapes. One separate ROCm-only negative at head
dimension 160 requires an explicit unsupported result.

## Evidence reconciliation

- The candidate path SHA-256 is
  `daf6a931f1bee54aeaaaf0b9a8002f3a778598211c4849d73def9f3f038bf292`;
  Git reports one file, 30 insertions, zero deletions.
- Each node passed 200/200 focused positive executions on CPU, ROCm, and Vulkan,
  for 1,200 required executions with zero failures. Nimo-1 also passed an
  additional combined CPU 200/200 run. The explicit ROCm negative produced the
  intended unsupported classification.
- The inherited full FlashAttention inventory reported zero failed cases: CPU
  5,136 supported; ROCm 2,919 supported plus 2,227 not supported; Vulkan 5,132
  supported plus four not supported. Backend not-supported classifications are
  not promoted as passes or failures.
- Both nodes completed the 581-target CPU build and the applicable Strix
  backend build. Nimo-1's feature-off controls passed. Nimo-2's admitted subset
  passed 79/79: 78 non-tokenizer tests and the tokenizer test separately.
- Nimo-2's 79/95 full CTest launcher result is excluded because 16 tokenizer
  invocations were corrupted by a CRLF launcher artifact and returned rc 8.
  The corresponding nimo-1 CRLF launcher artifact is likewise retained but
  excluded. Neither is represented as a product/test failure.
- The nimo-1 evidence bundle is 24,578,905 bytes, SHA-256
  `f03ba3e2ad6fb61e00de788362e653ee166c7b1c7ba4e43d49a372ef795a1e2f`,
  and reproduced byte-for-byte. The nimo-2 evidence tar is 4,904,960 bytes,
  SHA-256
  `5e424f1e11312f00c10b40467ee1d9c25b12aa3746a74459714045144619de21`.
- Both nodes' exact CPU test, Strix test, HIP library, and Vulkan library hashes
  are pinned in the receipt. Nimo-2's help output is byte-identical to nimo-1,
  its help stderr is empty, and its evidence bundle reproduced byte-for-byte.
- The nimo-1 inference server remained healthy at PID 3783057 on port 8081.
  The nimo-2 RPC service remained at PID 3562775 on port 50052. Neither service
  was restarted or redeployed.

## Provenance and similarity review

Preparation commit `f6c56a45cbd365d2208bcb5a65c0bd6afce62be7` independently approved the exact P3 test-only
clean-reimplementation treatment. The requirement authority is Nathan Wilson's
MIT commit `6b03608e63f48c9371bf5f00423da413ac0288de`; donor result blob
`1cbd27cb4d163fbc9970b57e0886a38adf62542f`. The preserved bundle and combined
patch remain intake evidence and were not applied. No donor code, comment,
literal table, dependency, kernel, or build change was imported, and the
direct-cherry-pick roster remains empty.

Similarity disposition is **APPROVE**: zero exact or whitespace-normalized line
matches across 28 target nonblank additions and 14 donor nonblank additions,
zero comment matches, and longest common token run nine. The donor Git delta is
15 insertions and two deletions. Manual structure review found no copied or
mechanically translated donor comment, name, ordering, literal case table, or
loop structure.

## Promotion decision

**ACCEPT for promotion as L14Q-T01.** The one-file test-only implementation,
20-positive-plus-one-negative inventory, independent P3/no-copy treatment, and
retained nimo-1/nimo-2 qualification evidence reconcile without findings. The
unsupported backend classifications remain capability outcomes rather than
passes, and the excluded CRLF launcher artifacts remain excluded rather than
being recast as product failures.

Rollback restores the parent test blob by reverting this single test-only
insertion. This milestone makes no performance claim and admits neither the HIP
tile dequant-on-load runtime path nor the Vulkan coopmat1 runtime path. It does
not modify ROCmFPX/TurboQuant routing, persistent state, models, services, or
deployed inference behavior.
