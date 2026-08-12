## Introduction
Welcome to this repository, a showcase of an **ONNX Runtime GenAI(OGA)‑based RAG LLM sample application** running on a **Ryzen AI processor**.

## What You’ll Find Here

- **Retrieval-Augmented Generation (RAG) pipeline** powered by:
  - A **hybrid LLM** enables disaggregated inference in which the compute-heavy prefill phase runs on the NPU, while the decode phase executes on the GPU.
  - An **embedding model** compiled with **Vitis AI Execution Provider**
- Built using the widely adopted **LangChain** orchestration framework

### 1.2  Activate Ryzen AI Environment

To ensure compatibility with ONNX-based Llama model, activate the ryzen-ai-1.7.0 Conda environment.

### 2.1 Retrieval-Augmented Generation (RAG) Pipeline

The following models are deployed using Ryzen AI 1.7.0:

- **Embedding Model**: [BGE (BAAI General Embedding)](https://huggingface.co/BAAI/bge-large-en-v1.5), compiled using Vitis AI Execution Provider.

- **Hybrid LLM**: [Llama3.2-3B-Instruct](https://huggingface.co/amd/Llama-3.2-3B-Instruct-onnx-ryzenai-1.7-hybrid), a quantized ONNX model, running using the OGA(OnnxRuntime GenAI) framework on Ryzen AI 1.7.0.

#### 🔹 ONNX Inference on AMD NPU

The embedding model is executed using ONNX Runtime on the NPU (Ryzen AI).

### 2.3 Download, Export to ONNX, and Compile the Embedding Model.

Run the following command to perform download, export and compile steps:

```bash
python custom_embedding/export_bge_onnx.py
```

This script generates a static‑shape, non‑quantized FP32 ONNX model that serves as the baseline for further deployment.
The compiled BGE (BAAI General Embedding) ONNX model will be stored in the cache folder named ``modelcachekey_bge``.
