# Runtime Support

**Pinned snapshots:** llama.cpp `6bdd77f13cf11b264b4231d320afc404f48d576e`; ROCmFPX `61f2f2d7bc4955e9bca821095ef69125837133b5`.

| Candidate | GGUF architecture | llama.cpp | ROCmFPX | Runtime risk |
|---|---|---|---|---|
| Qwen3-235B-A22B-Instruct-2507 | qwen3moe | Yes — dedicated qwen3moe implementation at pinned snapshot. | Yes — qwen3moe implementation present. | Low relative to this set; still preflight the exact GGUF and template. |
| Step-3.7-Flash | step35 | Yes — step35 implementation, hybrid SWA cache, and MTP paths present. | Yes — step35 plus a released ROCmFPX Q3 QualityPlus recipe. | Medium: newer architecture and multimodal/MTP paths make revision pinning important. |
| MiMo-V2-Flash | mimo2 | Yes — dedicated mimo2 implementation and hybrid SWA cache. | Yes — mimo2 implementation present. | Medium-low: implementation is current, but the model is newer and MTP tensors are preserved/disabled in the main pass. |
| GLM-4.7 | glm4moe | Yes — dedicated glm4-moe implementation. | Yes — glm4-moe implementation present. | Medium: long context and parser/template semantics deserve an exact-build acceptance test. |
| Llama-3.1-Nemotron-Ultra-253B-v1 | deci | Yes — Deci implementation supports per-layer attention/FFN topology arrays and 253B type. | Yes — Deci implementation present. | Medium: custom NAS topology means the exact GGUF metadata and current Deci implementation must match. |
| DeepSeek-R1-0528 | deepseek2 | Yes — deepseek2 implementation with MLA absorption. | Yes — deepseek2 implementation present. | Medium: large artifact, specialized MLA graph, and reasoning-generation behavior require end-to-end validation. |
| Llama-3.1-Tulu-3-405B | llama | Yes — mature generic Llama implementation. | Yes — generic Llama implementation inherited. | Low architecture risk, high compute and memory-pressure risk. |
| MiniMax-M3 | minimax-m3 | Not production-ready at snapshot: quant card points to a PR and says MSA falls back to dense attention. | Yes — dedicated minimax-m3 implementation and smoke tests present. | High: preliminary mainline status and sparse-attention semantics are not yet a stable deployment baseline. |
| Kimi-K2-Thinking | deepseek2-derived | Architecture family is supported, but the exact community conversion is not qualified here. | Architecture-family support expected; exact artifact not qualified. | High: extreme size, community conversion, and low-bit kernels. |

## What “supported” means here

A source implementation exists at the pinned commit and matches the model's architecture family. It does **not** mean every community GGUF, template, multimodal sidecar, speculative head, quant kernel, or backend combination is qualified.

## Important implementation findings

- MiMo and Step use llama.cpp's interleaved sliding-window/full-attention cache. With one sequence, `ubatch=512`, and `--swa-full` disabled, SWA capacity is padded to 256 cells and is bounded by `window + ubatch` rather than the full context.
- DeepSeek MLA uses the absorption optimization and a V-less cache input: 512 latent values plus a 64-value RoPE key per layer/token. Value vectors are reconstructed through `wv_b`.
- Nemotron Ultra uses the Deci architecture implementation with per-layer attention skip and FFN topology metadata. The table uses a conservative full-attention cache bound because the exact 126-block array is artifact-specific.
- MiniMax-M3 is not treated as mainline-production-ready. Its quant card references an in-flight PR and notes that MSA falls back to dense attention in that path.

## RPC and multi-device

llama.cpp can split layers/tensors across local devices and exposes an RPC backend for remote devices. The RPC documentation calls it proof-of-concept and warns that it has no authentication or encryption. Use it only on an isolated trusted network, and treat all two-node tables as capacity envelopes pending measurement.
