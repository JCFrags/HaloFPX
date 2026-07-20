# P06b MiniMax-M2 Q8 exact-shape kernel canary

Status: **gfx1151 kernel shapes qualified; partition equivalence remains closed**

Correction from P06d exact-artifact intake: the pinned 160 GB primary model's
gate, down, and up expert tensors are `Q6_0_ROCMFPX`, not
`Q8_0_ROCMFPX`. P06b is a synthetic Q8 backend-variant qualification at the
primary model's exact expert geometry. It does not qualify the primary
artifact's expert tensor type.

P06b qualifies the inherited ROCm `MUL_MAT_ID` path at the exact expert
projection geometry needed by the pinned MiniMax-M2.7 primary workload. The
cases exist only when `HALOFPX_MINIMAX_M2_EXPERT_PARTITION_CANARY=ON`; the
option still defaults `OFF`, and this milestone does not link the P06a seam or
new code into `llama` or `llama-server`.

The gated roster covers 192 experts, top-8 selection, Q8_0_ROCMFPX weights,
F32 activations, hidden width 3072, intermediate width 1536, and decode/batch
widths one and eight:

| Projection | Batch | Mean microseconds | Sample SD | Mean GFLOPS | Sample SD |
|---|---:|---:|---:|---:|---:|
| 3072 to 1536 | 1 | 157.22 | 13.20 | 482.51 | 41.14 |
| 1536 to 3072 | 1 | 158.61 | 5.33 | 476.35 | 15.99 |
| 3072 to 1536 | 8 | 1411.31 | 21.02 | 428.02 | 6.39 |
| 1536 to 3072 | 8 | 1411.71 | 20.29 | 427.89 | 6.10 |

These are three retained microkernel runs on nimo-1. They are not end-to-end
prompt-processing or token-generation results and do not establish a speedup.

## Focused qualification

Nimo-1 built `test-backend-ops` in Release mode with GCC 16.1.1, CMake 4.3.4,
HIP enabled, and gfx1151. RPC, Vulkan, dynamic backend loading, and the ROCWMMA
FlashAttention gate were disabled in this isolated build. The four gated cases
were filtered by Q8_0_ROCMFPX, 192 experts, top-8, and the exact projection and
batch dimensions.

The ROCm support probe admitted all four cases. Correctness mode compared all
four ROCm results with the CPU backend and reported `supported=1` with no error.
Three performance-mode runs then completed successfully. The test executable
SHA-256 is
`faf382d85555e82a153ab3c8b928db3e15d709deefbaacce341d26992f0ea893`.
The retained correctness CSV SHA-256 is
`71ec55f861976e1941da55e609edd6bcc7734acf2d188d12855058e41ef59163`.

The selected evidence manifest SHA-256 is
`94860a73b149a9a42fb027b00d2832e0afed90da6dfc9ca757c210b5a9ecd76c`.
The 89,012-byte bundle SHA-256 is
`b8afb826631117d630f0101a9a355be6906bcfdbe19ae928cd9f20cabc201a9c`;
all manifest entries verify.

## Boundary and next step

P06b establishes that the inherited gfx1151 backend supports and correctly
executes full 192-expert Q8_0_ROCMFPX projection tensors at the primary
model's exact shapes. It does not establish that the pinned artifact uses Q8,
split experts, compare full versus
rank-partitioned output, prove local/remote overlap, load the pinned 160 GB
artifact, change KV ownership, or claim server performance. The broader P06a
gate was deliberately split: P06b qualified the synthetic Q8 kernels, P06c
subsequently proved Q8 partition equivalence, and P06d qualified exact-artifact
Q6 peer-data placement. Q6 partition compute remains closed.

The known-good nimo-2 worker and nimo-1 server were stopped in dependency order
for the isolated GPU run and restarted worker-first afterward. Both returned
active with zero restarts. After the rollback model finished loading, the
nimo-1 HTTP health endpoint returned `200 {"status":"ok"}`.

No donor code, GPL llama-ai code, CachyLLama code, dependency, WebUI,
persistent write, model mutation, remote, deployment replacement, or
reference-clone change entered this milestone. Generation above 30 tok/s
remains a stretch objective.
