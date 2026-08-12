# Primary vendor and distribution claim capture

**Access date:** 2026-07-18  
**Method:** browser retrieval of public primary documentation; this file is a structured claim record, not a verbatim mirror.  
**Redistribution note:** AMD account-gated packages, firmware binaries, EULAs, and full proprietary documentation are not redistributed in this bundle.

## AMD Ryzen AI Software 1.7.1 — Linux installation

Source: https://ryzenai.docs.amd.com/en/latest/linux.html  
Page version: Ryzen AI Software 1.7.1  
Page last updated: 2026-07-17

Captured facts:

- Linux target platforms are STX and KRK.
- Supported Linux flows listed: CNN INT8, CNN BF16, BERT/encoder-style NLP BF16, and LLM NPU-only.
- Reference environment: Ubuntu 24.04 LTS, kernel >= 6.10, Python 3.12.x, 64 GB RAM recommended.
- Driver package names:
  - xrt_202610.2.21.75_24.04-amd64-base.deb
  - xrt_202610.2.21.75_24.04-amd64-base-dev.deb
  - xrt_202610.2.21.75_24.04-amd64-npu.deb
  - xrt_plugin.2.21.260102.53.release_24.04-amd64-amdxdna.deb
- Software archive: ryzen_ai-1.7.1.tgz.
- Validation entry points: xrt-smi examine and the package quicktest.
- Public download links route through AMD account infrastructure.

## AMD Ryzen AI Software 1.7.1 — release notes

Source: https://ryzenai.docs.amd.com/en/latest/relnotes.html  
Page version: Ryzen AI Software 1.7.1

Captured facts:

- Strix and Strix Halo are grouped as STX.
- Ryzen AI Max 300 Series is listed in the supported processor families.
- STX/KRK compatibility table lists CNN INT8, CNN BF16, NLP BF16, and LLM OGA.
- Version 1.7.1 adds NPU support for SmolLM2-135M-Instruct and SmolLM-135M-Instruct and up to 16K context for NPU models.
- Version 1.6 release notes explicitly record model/runtime incompatibility across OGA release generations.
- Version 1.4 release notes list a DistilBERT text-classification example and a gte-large-en-v1.5 embedding example, but that release-note entry does not establish Linux qualification.

## AMD model format, quantization, and operator coverage

Sources:
- https://ryzenai.docs.amd.com/en/latest/model_quantization.html
- https://ryzenai.docs.amd.com/en/latest/modelrun.html
- https://ryzenai.docs.amd.com/en/1.5/ops_support.html

Captured facts:

- ONNX is the primary CNN/transformer interchange format; opset 17 is recommended.
- FP32 CNN/transformer input can be converted to BF16 by the Vitis AI execution-provider compiler path.
- CNN INT8 modes include XINT8 and A8W8; A16W8 is documented in the release/operator matrix.
- Operator coverage is conditional by operator configuration. A table-level “Y” means broad coverage, not universal coverage.
- Graph partitioning may leave unsupported subgraphs on CPU; operator assignment must be measured on the actual model.

## AMD Linux LLM path

Source: https://ryzenai.docs.amd.com/en/latest/llm_linux.html  
Page last updated: 2026-07-10

Captured facts:

- The reference path uses AMD-prepared, versioned OGA/NPU model assets and AMD deployment libraries.
- The documented Linux flow is NPU-only.
- The model-generation package is version-pinned to model-generate==1.7.1 from an AMD package index.
- Published example measurements are reference-output values, not measurements on the target Ryzen AI MAX+ 395 system.

## llama.cpp and Whisper boundary

Sources:
- https://ryzenai.docs.amd.com/en/latest/
- https://ryzenai.docs.amd.com/en/1.7/whisper_cpp.html

Captured facts:

- AMD's stack overview states llama.cpp is supported for iGPU, not as the NPU execution interface.
- AMD's Whisper page says NPU acceleration is currently Windows-only and Linux support is planned.

## Firmware

Sources:
- https://kernel.googlesource.com/pub/scm/linux/kernel/git/firmware/linux-firmware/+/924d73c9a2501a256d18a26cbe640548c70b3a9a/WHENCE
- https://kernel.googlesource.com/pub/scm/linux/kernel/git/firmware/linux-firmware/+/924d73c9a2501a256d18a26cbe640548c70b3a9a/

Pinned linux-firmware commit: 924d73c9a2501a256d18a26cbe640548c70b3a9a  
WHENCE blob: 3be577a6ba7f0f880176c8c79c64f498df84599a

Captured entries for PCI directory 17f0_11:

- amdnpu/17f0_11/npu.sbin.1.0.0.166; npu.sbin symlink
- amdnpu/17f0_11/npu.sbin.1.1.2.65; npu_7.sbin symlink
- License entry: Redistributable; see LICENSE.amdnpu.

The binary firmware and LICENSE.amdnpu text are not copied into this bundle.

## Capture limitations

- The exact target distribution, kernel package, firmware package, and installed AMD package inventory were not supplied.
- Account-gated AMD package payloads and their package-level hashes were not accessible.
- The AMD EULA and Linux third-party notices are referenced but not interpreted here.
- Public-source commits for XRT, llvm-aie, and mlir-aie are pinned separately. No claim is made that the account-gated 1.7.1 binaries were built from those public HEAD commits.
