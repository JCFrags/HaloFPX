# ROCmFPX operator evidence matrix

| Requirement | Static evidence at `61f2f2d…` | Claim boundary |
|---|---|---|
| HIP build path | `GGML_USE_HIP` branch in `ggml-cuda.cu` | Does not prove target GPU build succeeds |
| MoE selection/matmul | `topk-moe.cuh`, `GGML_OP_MUL_MAT_ID` source presence | Does not prove every selected quant tensor has a HIP kernel |
| SSM/Mamba state | `ssm-conv.cuh`, `ssm-scan.cuh` and implementation files | Does not prove Nemotron state correctness or no fallback |
| Qwen DeltaNet | `gated_delta_net.cuh/.cu` included | Does not prove recurrent sequencing or numerical equivalence |
| Attention | flash-attention and matrix source included | Does not prove MLA/long-context support for exact shapes |
| Speculative/MTP | Model-side graph paths vary | No candidate MTP run was performed |

Status: `KERNEL_MATRIX_INCOMPLETE_STATIC_REVIEW`; local backend tracing is mandatory.
