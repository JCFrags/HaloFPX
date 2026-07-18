# Static runtime-source review

## Pinned revisions

- llama.cpp: `86a9c79f866799eb0e7e89c03578ccfbcc5d808e`
- ROCmFPX: `61f2f2d7bc4955e9bca821095ef69125837133b5`

## What static support means

A registered architecture and concrete graph builder show that source code exists for the model family. They do not prove that the selected GGUF’s exact tensor schema loads, that every operator is dispatched to ROCm, that there is no CPU fallback, that recurrent state is correct, or that outputs meet quality targets.

## Candidate observations

| Family | Concrete llama.cpp graph | Special state | MTP/NextN static observation | ROCmFPX static observation |
|---|---|---|---|---|
| DeepSeek2 | Yes | MLA + MoE | No explicit decoder-MTP graph identified | Generic attention/MoE source; not run |
| GLM4-MoE | Yes | KV + MoE | NextN tensors skipped and not executed | Generic attention/MoE source; not run |
| Qwen3.5-MoE | Yes | DeltaNet recurrent + KV + MoE | Explicit decoder-MTP path | SSM/gated-delta source files included; not run |
| Nemotron-H-MoE | Yes | Mamba2 recurrent + KV + LatentMoE | No explicit decoder-MTP graph identified | SSM conv/scan source files included; not run |
| MiniMax-M2 | Yes | KV + MoE | No explicit decoder-MTP graph identified | Generic attention/MoE source; not run |

Machine validation status: **NOT_RUN**.
