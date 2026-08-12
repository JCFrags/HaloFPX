# 5. Model formats, operators, and data types

## Model interchange

[VENDOR-ONLY] ONNX is the documented high-level interchange format.

[VENDOR-ONLY] AMD recommends ONNX opset 17 for captured CNN/transformer flows.

[VENDOR-ONLY] The public DistilBERT classifier converts the Hugging Face checkpoint to ONNX opset 17, then uses BF16 and the Vitis AI execution provider.

[UNSUPPORTED] Generic GGUF or `llama.cpp` model loading is not a documented NPU path.

## Numeric formats

The captured AMD support boundary includes:

| Family | Captured use |
|---|---|
| BF16 | NLP/transformers and CNN paths |
| XINT8 | supported quantization family in AMD operator documentation |
| A8W8 | activation 8-bit / weight 8-bit family |
| A16W8 | activation 16-bit / weight 8-bit family |
| FP32 | model input/baseline; transformer flow may compile to BF16 |

[VENDOR-ONLY] Data-type support is conditional on model, operator, shape, and compiler/runtime version.

## Operator coverage

[VENDOR-ONLY] AMD publishes a broad operator table, but the captured table is labeled Ryzen AI 1.5.0 rather than an exact 1.7.1 guarantee.

[VENDOR-ONLY] The documentation warns that particular configurations can remain unsupported even when the operator family appears in the table.

[VENDOR-ONLY] The execution provider may partition unsupported subgraphs to CPU.

[DECISION] A successful session creation does not pass the gate. The actual provider assignment/partition report must show the expensive subgraph on NPU.

## Captured examples relevant to bounded roles

### DistilBERT classifier

[VENDOR-ONLY] Exact public example:

- model: `distilbert-base-uncased-finetuned-sst-2-english`;
- ONNX opset 17;
- BF16;
- batch 1;
- input IDs and attention mask as `int64`;
- fixed maximum length 128;
- Vitis AI execution provider;
- compiled cache.

Sources:
- [`../sources/raw/ryzenai-sw/DistilBERT_text_classification_bf16_README.md`](../sources/raw/ryzenai-sw/DistilBERT_text_classification_bf16_README.md)
- [`../sources/raw/ryzenai-sw/run_inference.py`](../sources/raw/ryzenai-sw/run_inference.py)
- [`../sources/raw/ryzenai-sw/vitisai_config.json`](../sources/raw/ryzenai-sw/vitisai_config.json)

### BGE embedding

[UNKNOWN] AMD publishes a BGE embedding implementation using `VitisAIExecutionProvider`, fixed maximum length 512, and pooler output.

[UNKNOWN] The captured public RAG instructions are pinned to a Ryzen AI 1.7.0 hybrid/Conda flow and do not establish Linux qualification for the exact 1.7.1 target stack.

Source: [`../sources/raw/ryzenai-sw/custom_embedding.py`](../sources/raw/ryzenai-sw/custom_embedding.py).

### Small OGA models

[VENDOR-ONLY] AMD's 1.7.1 release notes list SmolLM and SmolLM2 135M instruction models for the NPU stack.

[INFERENCE] Model availability does not establish a draft-model API, speculative-decoding integration, KV-cache sharing, acceptable token latency, or useful system-level speedup.

## Required model manifest

Any experiment must preserve:

- original model and tokenizer IDs and licenses;
- original and converted model hashes;
- ONNX opset and graph hash;
- static shapes and all tensor dtypes;
- quantization/compiler commands;
- provider configuration;
- compiled cache hashes;
- NPU/CPU assignment report;
- release/package versions.
