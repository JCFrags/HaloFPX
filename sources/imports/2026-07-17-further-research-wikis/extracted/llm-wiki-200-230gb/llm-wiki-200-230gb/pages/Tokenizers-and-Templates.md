# Tokenizers and Chat Templates

| Candidate | Tokenizer | Template / launch rule |
|---|---|---|
| Qwen3-235B-A22B-Instruct-2507 | Embedded Qwen tokenizer in GGUF | Embedded Qwen Jinja; launch with --jinja and preserve tool-call tokens. |
| Step-3.7-Flash | Embedded tokenizer; verify BOS/EOS and multimodal special tokens. | Embedded Step Jinja; vision requires the matching mmproj sidecar. |
| MiMo-V2-Flash | Embedded tokenizer in GGUF. | Embedded MiMo Jinja; use --jinja and validate reasoning/tool modes against upstream examples. |
| GLM-4.7 | Embedded GLM tokenizer in GGUF. | Embedded GLM Jinja; reasoning/tool parser behavior is revision-sensitive. |
| Llama-3.1-Nemotron-Ultra-253B-v1 | Llama 3 tokenizer embedded in GGUF. | Use the NVIDIA-provided template/system instructions; reasoning mode is controlled by the documented system prompt. |
| DeepSeek-R1-0528 | Embedded DeepSeek tokenizer. | R1-0528 template; system prompts are supported in this release. Do not force a <think> prefix. |
| Llama-3.1-Tulu-3-405B | Llama 3 tokenizer embedded in GGUF. | Tulu template: <\|system\|>, <\|user\|>, <\|assistant\|>; prefer embedded Jinja. |
| MiniMax-M3 | Embedded tokenizer expected; verify exact converter build. | MiniMax thinking/non-thinking template; exact GGUF/template pair must be validated. |
| Kimi-K2-Thinking | Community conversion; verify tokenizer and special tokens. | Community conversion; template provenance must be checked against upstream. |

## Preflight gate

Before benchmarking, use the exact GGUF and runtime commit to verify:

1. BOS/EOS are not duplicated.
2. System, user, assistant, tool, and reasoning roles render as expected.
3. Stop tokens terminate generation without leaking control tokens.
4. The embedded `tokenizer.chat_template` is present and accepted with `--jinja`.
5. Tool calls round-trip through the server parser used in production.
6. Multimodal models load the matching projector and use the same special-token map.

## Candidate-specific cautions

- **DeepSeek R1-0528:** system prompts are supported; do not force a `<think>` prefix or reuse older R1 prompt guidance blindly.
- **Nemotron Ultra:** reasoning behavior depends on NVIDIA's documented system prompt; do not substitute a generic Llama template.
- **Tulu 3:** its `<|system|>`, `<|user|>`, `<|assistant|>` format is not Meta Llama's stock instruct template.
- **Step/MiniMax:** thinking and multimodal/tool paths add parser and sidecar dependencies.

Tokenizer correctness is a binary acceptance criterion. A model that “loads” but tokenizes the template incorrectly is not a viable candidate.
