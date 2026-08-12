---
type: follow-up-research
status: proposed
priority: bounded-feasibility
target: HaloFPX Linux XDNA2 NPU role decision
created: 2026-07-16
last_researched: 2026-07-17
risk: medium
approval_required: human-and-machine
scope: Strix Halo NPU hardware, Linux driver and userspace maturity, model fit, memory movement, role selection, and stop criteria
wiki_changes: none
---

# Linux XDNA2 suitability for HaloFPX

## Executive decision

**Decision: DEFER production integration; AUTHORIZE only a time-boxed feasibility spike.**

**[VERIFIED]** AMD Ryzen AI Max+ 395 is the product formerly codenamed Strix Halo and advertises an integrated Ryzen AI NPU rated at up to 50 TOPS. Its memory is 256-bit LPDDR5x with a 128 GB product maximum. These are product specifications, not application performance measurements. [XDNA-01]

**[VERIFIED]** Ryzen AI Software 1.7.1 names Strix Halo within its supported STX family. Its Linux instructions support Ubuntu 24.04, kernel 6.10 or newer, Python 3.12, XRT/NPU packages, CNN INT8/BF16, encoder-style NLP BF16, and NPU-only LLM execution. Linux LLM execution uses prepared ONNX Runtime GenAI (OGA) artifacts; AMD's example is a prequantized, postprocessed Phi-3.5-mini model. [XDNA-02][XDNA-03][XDNA-04]

**[VERIFIED]** The upstream `amdxdna` kernel driver exposes an inference accelerator whose workloads are compiled into an XDNA overlay and `ctrlcode`. Its documented execution path explicitly moves data between host DDR and software-managed on-chip memory through DMA. The XRT shim remains a separate userspace component. [XDNA-05][XDNA-06]

**[INFERENCE]** XDNA2 is therefore a model- and toolchain-specific accelerator, not a transparent extension of the pinned HaloFPX `llama.cpp`/GGUF runtime. The reviewed primary sources do not establish a llama.cpp XDNA backend, direct ROCm/XDNA buffer sharing, shared KV state, or a supported way to split one decode graph between the existing GPU ranks and the NPU.

**[RECOMMENDATION]** Keep the main model, distributed decode, KV ownership, coordinator, deterministic service logic, and ordinary telemetry on their existing CPU/ROCm paths. Permit one isolated NPU service experiment for (1) a prepared small embedding/reranking model and (2) only if protocol-compatible with the chosen speculative design, a prepared 135M-to-low-single-digit-billion draft model. Do not budget production implementation until exact-machine evidence passes every gate in this report.

No wiki pages were changed by this follow-up.

## What “supported” must mean

| Level | Required evidence | What it does not prove |
|---|---|---|
| product-present | exact processor SKU lists Ryzen AI NPU | OS can enumerate or execute it |
| kernel-enumerated | correct PCI function binds to `amdxdna`; `/dev/accel` exists | firmware/XRT compatibility |
| runtime-ready | exact driver, firmware, XRT shim, and Ryzen AI package pass official validation | target graph compiles or is correct |
| graph-supported | exact immutable model artifact compiles/loads with no unintended CPU fallback | output quality or latency benefit |
| output-validated | outputs pass task-specific oracle against a pinned reference | end-to-end role benefit under contention |
| role-suitable | matched end-to-end workload improves the declared objective without harming the main service | portability to another model, release, or machine |
| production-ready | recovery, observability, reproducibility, upgrades, and rollback pass | future releases remain compatible |

An `xrt-smi` GEMM TOPS result is a full-array INT8 synthetic test. It is useful for hardware sanity, not evidence for LLM decode, embedding latency, or HaloFPX throughput. [XDNA-07]

## Hardware and execution constraints

### Exact product boundary

- **[VERIFIED]** Ryzen AI Max+ 395 is Strix Halo, with up to 50 advertised NPU TOPS, up to 128 GB LPDDR5x-8000, and a 256-bit memory interface. [XDNA-01]
- **[OPEN]** The exact two project nodes, OEM firmware revisions, NPU PCI revisions, enabled firmware, power/thermal policy, and memory reservation have not been inventoried in this report.
- **[RECOMMENDATION]** Do not extrapolate from AMD's peak TOPS or documentation sample output. Record application-level wall time, quality, energy if available, and main-service interference on both exact nodes.

### NPU architecture and memory movement

**[VERIFIED]** The kernel documentation describes XDNA as a partitionable two-dimensional array of compute and memory tiles. Each column has DMA engines that move data between host DDR and a memory tile; the memory tiles form software-managed on-chip L2. Workloads require a compiler-produced overlay and `ctrlcode`. Each workload context also uses a host-resident 64 MB instruction buffer mapped for its execution-runtime context. [XDNA-05]

**[VERIFIED]** Userspace submits a command buffer containing input, output, and instruction-buffer pointers. Firmware executes `ctrlcode`, which starts DMA transfers between host DDR and on-chip memory. Suspend powers the NPU off; resume reloads firmware and repeats the handshake. [XDNA-05]

**[INFERENCE]** “Unified memory” at the product level does not imply zero-copy interoperability between ROCm allocations, llama.cpp tensors/KV, OGA objects, and XRT buffer objects. The reviewed sources establish host-DDR DMA, not a shared execution graph or coherence contract with the GPU runtime.

**[RECOMMENDATION]** Treat every CPU/GPU-to-NPU boundary as a serialization, synchronization, and possible copy until a pinned implementation plus machine trace proves otherwise. Measure prompt/token IDs, embeddings, logits, and state transfers separately. Do not place latency-critical per-token work across that boundary without a demonstrated end-to-end win.

## Linux stack maturity and reproducibility

| Layer | Current primary evidence | Assessment for HaloFPX |
|---|---|---|
| kernel driver | `amdxdna` is documented in the upstream kernel accelerator subsystem; AMD's driver repository says Ubuntu 25.04 kernel 6.14 includes it [XDNA-05][XDNA-06] | usable foundation, but exact distro kernel is not sufficient evidence |
| firmware/driver pairing | AMD's repository warns stale or mixed firmware can cause command aborts/mailbox timeouts [XDNA-06] | must be pinned as one tested set; no mix-and-match |
| XRT shim | still required; Ryzen AI 1.7.1 Linux installs XRT 2.21.75 packages and `xrt_plugin.2.21.260102.53...amdxdna.deb` [XDNA-02] | separate lifecycle from kernel; record package hashes |
| upstream versus packaged driver | AMD's current repository builds an upstream/staging driver and a legacy out-of-tree driver, and warns an older in-tree driver may lack ioctls required by a newer shim [XDNA-06] | production must identify the actually loaded module, not only the installed package |
| compiler/runtime | kernel docs identify Peano/IRON compilation and XRT; Ryzen AI supplies higher-level compilation/runtime packages [XDNA-05][XDNA-02] | capable but specialized; cache all compiled artifacts with source/model/toolchain identity |
| Linux model flow | Ryzen AI 1.7.1 Linux supports NPU-only LLM flow and prepared OGA models; hybrid NPU+iGPU is not the documented Linux path [XDNA-03] | isolate from the ROCm main service; do not assume Windows feature parity |
| management | `xrt-smi` can report versions/device state, JSON inventory, partitions, contexts, and validation tests; estimated power is currently unavailable on Linux [XDNA-07] | sufficient for readiness inventory, incomplete for energy and all failure modes |

The AMD driver repository was inspected at `319aa5e50f8ee51cdfb8447c38e9c4d34a6bbfeb` (`main`, resolved 2026-07-17). Its release tag `2.21.75` resolves to `beb9e450fe123ecdf395453971576179cedcf1dd`, while the Ryzen AI Linux package names also carry XRT 2.21.75. This correspondence is useful provenance, not proof that the untested repository head should replace the packaged stack. [XDNA-06]

## Model and operator fit

**[VERIFIED]** Ryzen AI 1.7.1 reports STX/KRK support for CNN INT8, CNN BF16, NLP BF16, and OGA LLMs. The Linux page narrows Linux LLM use to the NPU-only flow. [XDNA-02][XDNA-03][XDNA-04]

**[VERIFIED]** The documented general CNN/NLP operator table provides broad, not universal, coverage and explicitly warns that some configurations of listed operators may not be supported. The LLM flow has a much smaller named OGA operator set: simplified layer-normalization variants, `MatMulNBits` in W4A-BF16/FP16 forms, `Add`, rotary embedding, group-query attention, `Sigmoid`, and `Mul`. [XDNA-08]

**[VERIFIED]** Linux 1.7.1 selects prequantized and postprocessed ready-to-run artifacts from AMD's NPU model collection. AMD's reference flow uses `Phi-3.5-mini-instruct_rai_1.7.1_npu_4K`; custom preparation installs `model-generate==1.7.1` from AMD's Linux package index. [XDNA-03]

**[INFERENCE]** A GGUF file or a model supported by llama.cpp is not thereby supported on XDNA2. Architecture, exact operator shapes, quantization, context length, tokenizer/chat behavior, and prepared runtime artifacts all form part of compatibility.

**[RECOMMENDATION]** Admit a model only when its exact immutable artifact is in AMD's compatible NPU flow or successfully passes a reproducible preparation/compile process, objective task-quality comparison, and negative tests for unsupported shapes. Do not count silent CPU fallback as NPU success.

## Role-by-role suitability

| Candidate role | Decision | Evidence-backed rationale | Promotion gate |
|---|---|---|---|
| main HaloFPX decode / GGUF layers | **REJECT now** | existing path is llama.cpp/ROCm; reviewed XDNA path is OGA with prepared models and its own state/runtime | a maintained backend or explicit service boundary plus matched end-to-end superiority and state correctness |
| distributed rank or KV-cache owner | **REJECT** | no reviewed primary evidence for shared ROCm/XDNA tensor or KV ownership; extra boundaries threaten latency and recovery semantics | explicit ABI/coherence contract, generation fencing, fault recovery, and measured win |
| coordinator / scheduler | **REJECT** | control logic is branchy, low-volume, and CPU-native; NPU requires compiled workloads and buffer submission | none unless coordinator becomes a supported ML inference graph with measured value |
| speculative draft model | **CANDIDATE, narrow** | AMD 1.7.1 adds small NPU-only LLM support and preserves GPU availability, but speculation depends on exact tokenizer/vocabulary, token distribution, context/state, acceptance rate, and per-step transfer cost | exact target/draft protocol compatibility; quality parity; acceptance and end-to-end latency gain under load |
| embedding model | **CANDIDATE** | encoder-style NLP BF16 is explicitly supported on Linux and the result is a compact service boundary | exact model compiles; vector parity/retrieval quality; batch/latency win; no main-service regression |
| reranker/classifier | **CANDIDATE** | bounded transformer/CNN graphs may fit documented formats and avoid per-token coupling | exact graph/shape/operator coverage and task-metric parity |
| telemetry anomaly model | **DEFER** | a batched ML anomaly detector could fit, but deterministic collection, aggregation, alerting, and policy belong on CPU | sustained batch volume plus demonstrated detection benefit exceeding stack/transfer cost |
| telemetry collection/control | **REJECT** | XDNA is an inference accelerator, not a replacement for kernel counters, logging, or service policy | none |
| ASR sidecar | **OUT OF CURRENT SCOPE** | AMD documents Whisper.cpp support, but HaloFPX has no stated ASR requirement | explicit product requirement and separate bounded review |

### Speculative decoding-specific constraint

**[RECOMMENDATION]** Do not call an independently served NPU LLM a “draft model” until the target runtime can consume its token proposals with exact tokenizer/vocabulary alignment and a validated acceptance algorithm. Measure the complete loop: target context transfer or reconstruction, draft generation, token transport, target verification, rejection recovery, and scheduling. Draft tokens per second alone is not the objective.

## Bounded feasibility experiments

All evidence belongs under `experiments/` with exact node, BIOS, kernel, loaded module path/hash, firmware, XRT, Ryzen AI package, model repository revision/content digest, compiler options, environment, and timestamps. These are proposed experiments, not commands already run.

### XDNA-A — Read-only inventory and readiness

1. Capture processor/OEM/BIOS and NPU PCI identity and revision with `lscpu`, `dmidecode`, and `lspci -nnk`.
2. Capture kernel, `CONFIG_DRM_ACCEL`, `CONFIG_AMD_IOMMU`, loaded `amdxdna` module path/version/hash, firmware filenames/hashes, `/dev/accel`, IOMMU/SVA state, groups, and memlock limits.
3. Capture `xrt-smi --version` and `xrt-smi examine -f JSON`, plus installed package versions and hashes.
4. Run only the official quicktest and `xrt-smi validate` in an idle window; preserve failures and journal output.

Pass: both nodes enumerate the same intended stack, the loaded driver and firmware are a documented compatible pair, official validation succeeds, and no hand-copied untracked firmware or ambiguous duplicate module is active.

### XDNA-B — Prepared-model correctness

1. Freeze a supported AMD 1.7.1 NPU-only model artifact and its license/digest.
2. Run deterministic prompts/inputs against the pinned OGA CPU/reference implementation and NPU implementation.
3. Compare tokenization, logits where exposed, generated tokens under deterministic sampling, context boundaries, error behavior, and memory use.
4. Exercise invalid shapes, over-context inputs, allocation failure, process kill, and restart.

Pass: no unintended fallback, objective output criteria pass, resource use is bounded, and every failure is observable and recoverable without affecting the main runtime.

### XDNA-C — Embedding/reranking sidecar

Use the exact candidate production model, dimensions, sequence-length distribution, batch sizes, and retrieval/reranking corpus. Compare CPU, NPU cold/warm, and concurrent GPU-load cases. Measure request queueing, preprocessing, host-to-NPU submission, inference, output materialization, tail latency, task-quality metrics, and main-service tokens/s/latency.

Pass: task quality meets the pinned baseline; end-to-end latency or CPU capacity improves materially at the intended load; and the ROCm/USB4/NVMe service remains within its declared SLO.

### XDNA-D — Speculative draft feasibility

Begin only after the exact speculation protocol and tokenizer contract are defined. Test one supported small NPU model without modifying the main decoder. Record draft/target model identities, proposed tokens per step, acceptance distribution, target verification cost, context synchronization cost, RPC/IPC copies, rejection recovery, TTFT, inter-token latency, throughput, and output equivalence.

Compare at minimum: target alone, CPU draft, NPU draft, and any existing GPU draft under matched target settings. Include short/long prompts, recurrent turns, low/high acceptance prompts, concurrency, and both nodes.

Pass: deterministic correctness holds and the NPU draft improves the declared end-to-end metric with confidence while preserving target throughput and recovery behavior. An isolated NPU tokens/s result cannot pass this gate.

### XDNA-E — Contention and shared-resource effects

Run the accepted candidate with the intended ROCm model, dual USB4/MPTCP traffic, NVMe cache traffic, and CPU preprocessing. Sweep NPU concurrency and power mode. Record memory bandwidth counters available to the platform, CPU/GPU/NPU utilization, thermal/power state where observable, OOM/memlock failures, tail latency, and service errors.

Pass: no material regression in the primary inference SLO, no unstable thermal/power oscillation, and bounded memory/locked-memory use. Missing Linux NPU power telemetry remains a limitation, not zero energy.

### XDNA-F — Failure, suspend, upgrade, and rollback

In an isolated window, kill NPU clients, force allocation/shape errors, suspend/resume, restart the driver/runtime through supported mechanisms, and test one pinned upgrade/rollback pair. Inject a deliberately mismatched firmware/driver pair only on a recoverable test installation if separately approved.

Pass: the main inference service remains available or fails according to its declared dependency policy; the NPU sidecar detects loss, rejects stale state, restarts cleanly, and rollback restores the last known-good tuple.

## Hard stop criteria

Stop the feasibility spike and leave XDNA2 unintegrated if any condition is true:

1. Either exact node does not bind reliably to the intended `amdxdna` driver/firmware tuple, or requires an untracked manual firmware/module workaround.
2. The official Linux quicktest or validation fails after one clean, pinned installation attempt and one evidence-led remediation.
3. The exact candidate model, operator shapes, quantization, context, or tokenizer cannot be reproduced with the pinned Linux 1.7.1 flow.
4. Correctness, embedding/retrieval quality, reranking quality, or speculative output equivalence misses the predeclared threshold.
5. NPU execution silently falls back, exposes an opaque mixed execution path, or cannot report enough identity to prove where the graph ran.
6. CPU/GPU/NPU state exchange needs per-token copying or serialization whose end-to-end cost removes the benefit.
7. No documented and tested ownership/recovery rule exists for draft context, KV/state, in-flight requests, and failures.
8. The candidate does not materially improve the predeclared end-to-end objective over CPU/GPU alternatives with matched settings and confidence intervals.
9. Primary ROCm inference latency/throughput, USB4 transport, NVMe cache behavior, or system stability regresses beyond its declared budget.
10. Firmware, driver, XRT, Ryzen AI, prepared model, and compiler artifacts cannot be pinned, archived where licensing allows, and rolled back reproducibly.
11. NPU hangs, suspend/resume, process death, or upgrades can poison the main service or require a machine reboot for routine recovery.
12. Maintaining the separate OGA/XRT/compiler/model-conversion stack exceeds the value of the bounded role.

Two consecutive release cycles with no passing candidate should close the backlog item until a specific new primary-source capability or product requirement reopens it. “More TOPS” alone is not reopening evidence.

## Recommended backlog disposition

| Priority | Work | Exit condition |
|---|---|---|
| P1 | XDNA-A exact-node inventory/readiness | reproducible driver/firmware/XRT tuple on both nodes |
| P1 | XDNA-C embedding or reranker spike | exact production graph passes quality and end-to-end contention gates |
| P2 | XDNA-D draft spike | only after speculation protocol/tokenizer contract exists; matched end-to-end win |
| defer | production sidecar, lifecycle, observability | one candidate passes correctness, performance, contention, and recovery |
| reject | coordinator, telemetry control, direct HaloFPX main-decode/KV integration | reopen only on a specific maintained integration contract and machine evidence |

**Review disposition: ACCEPT the bounded research plan; DEFER implementation.** The current official Linux stack is sufficiently real to justify a small experiment and insufficiently interchangeable with HaloFPX's existing runtime to justify architectural coupling.

## Primary source register

| ID | Primary source, pinned revision/date | Supports | Limitation |
|---|---|---|---|
| XDNA-01 | AMD, [Ryzen AI Max+ 395 product specification](https://www.amd.com/en/products/processors/laptop/ryzen/ai-300-series/amd-ryzen-ai-max-plus-395.html), accessed 2026-07-17 | Strix Halo identity, NPU rating, memory interface/capacity | peak product specifications; no HaloFPX performance |
| XDNA-02 | AMD, [Ryzen AI 1.7.1 Linux installation](https://ryzenai.docs.amd.com/en/latest/linux.html), updated 2026-07-10, accessed 2026-07-17 | Linux prerequisites, supported formats, package filenames, quicktest | does not validate either project node |
| XDNA-03 | AMD, [Running LLM on Linux, Ryzen AI 1.7.1](https://ryzenai.docs.amd.com/en/latest/llm_linux.html), updated 2026-07-10, accessed 2026-07-17 | NPU-only Linux flow, prepared artifacts, reference model, model-generation package | sample output is not portable performance evidence |
| XDNA-04 | AMD, [Ryzen AI 1.7.1 release notes](https://ryzenai.docs.amd.com/en/latest/relnotes.html), accessed 2026-07-17 | Strix Halo/STX support and model-format compatibility | broad release scope includes Windows features; Linux page is narrower |
| XDNA-05 | Linux kernel project, [AMD NPU driver documentation](https://docs.kernel.org/7.0/accel/amdxdna/amdnpu.html), accessed 2026-07-17 | architecture, compilation, buffers, DMA, execution, suspend, userspace stack | describes Phoenix/Hawk Point/Strix Point details; does not separately quantify Strix Halo array resources |
| XDNA-06 | AMD, [`amd/xdna-driver` README](https://github.com/amd/xdna-driver/blob/319aa5e50f8ee51cdfb8447c38e9c4d34a6bbfeb/README.md), commit `319aa5e50f8ee51cdfb8447c38e9c4d34a6bbfeb`, inspected 2026-07-17; release tag `2.21.75` at `beb9e450fe123ecdf395453971576179cedcf1dd` | kernel/XRT shim boundary, supported kernel baseline, packaged drivers/firmware, mismatch failure modes | repository head is not the approved deployment baseline |
| XDNA-07 | AMD, [NPU Management Interface, Ryzen AI 1.7.1](https://ryzenai.docs.amd.com/en/latest/xrt_smi.html), updated 2026-07-10, accessed 2026-07-17 | JSON inventory, versions, contexts, validation-test semantics, Linux telemetry limitation | sample metrics are not project measurements |
| XDNA-08 | AMD, [Supported Operators, Ryzen AI 1.7.1](https://ryzenai.docs.amd.com/en/latest/ops_support.html), updated 2026-07-10, accessed 2026-07-17 | broad CNN/NLP operator caveat and named OGA LLM operators | listed operator does not guarantee every configuration |

## Review of this review

This report separates product capability, upstream driver presence, userspace readiness, graph compatibility, correctness, and end-to-end suitability. It does not promote AMD sample numbers or peak TOPS into HaloFPX performance claims. Linux-only limitations override broader cross-platform release notes, and unknown ROCm/XDNA interoperability remains explicitly open. The recommendation is reversible: no wiki or implementation change is proposed, each experiment has an exit gate, and the stop criteria prevent an attractive secondary accelerator from silently expanding into a second unbounded inference platform.
