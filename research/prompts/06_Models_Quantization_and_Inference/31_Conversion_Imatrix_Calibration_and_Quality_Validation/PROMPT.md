# Research Agent Prompt — 31: Conversion, Imatrix, Calibration, and Quality Validation

The project is a custom local LLM inference stack for two matched AMD Strix Halo systems connected by two tuned, low-latency USB4 host-to-host links. The intended source base is a fork of `charlie12345/ROCmFPX`, selectively incorporating the persistent SSD-backed KV-cache and agent-serving ideas from `fewtarius/CachyLLama` and `fewtarius/llama-ai`, while tracking `ggml-org/llama.cpp`. The product must choose among replication, remote speculative decoding, two-way tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution, with a rank-local persistent cache and a specialized dual-link transport. The wiki will be consumed by local Codex and human engineers before and during implementation.

Research and build the wiki section **31: Conversion, Imatrix, Calibration, and Quality Validation**.

**Required coverage:** Document end-to-end conversion from source checkpoints to BF16/F16 GGUF and ROCmFPX quants, including tokenizer metadata, architecture-specific conversion, imatrix creation and use, calibration corpora, tensor routing, checksums, failure recovery, disk requirements, and quality comparisons against source models. Include reproducible commands and acceptance thresholds.

Use current primary sources and exact repository commits, releases, document revisions, and dates. Separate verified facts, inferences, assumptions, recommendations, open questions, and actual measurements. Explain the implications for HaloFPX. Include concrete Internet follow-up and on-machine validation tasks; never invent benchmark results.

Return a downloadable folder named:

```text
06_Models_Quantization_and_Inference/31_Conversion_Imatrix_Calibration_and_Quality_Validation/
```

Follow the included `OUTPUT_STANDARD.md`. The folder must contain `README.md`, `facts_and_constraints.md`, `design_implications.md`, `procedures_and_checks.md`, `open_questions.md`, `sources.md`, and `section.yaml`, plus diagrams, scripts, data, or evidence only when useful. Use relative cross-links and stable section IDs, keep pages retrieval-friendly, and identify any conclusions that depend on other wiki sections.
