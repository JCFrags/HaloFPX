# HaloFPX Glossary

Status: preferred project terms

Last verified: 2026-07-29

Use one preferred term for each concept.
Define an abbreviation at first use in every standalone page.

| Preferred term | Meaning |
|---|---|
| accepted baseline | A named configuration that passed its stated gate |
| Advanced Micro Devices (AMD) | The vendor of the Strix Halo system-on-chip |
| architecture decision record (ADR) | An accepted or rejected architecture choice with provenance |
| cache hit | A compatible authenticated state object that the runtime accepts |
| cache miss | A state lookup that causes cold computation or recomputation |
| canonical path | The stable project location for one authoritative artifact |
| commit | A full Git commit identifier when the identifier affects authority |
| continuous integration (CI) | Automated build, test, and validation work |
| coordinator | The process that owns request control and coordinates ranks |
| directed acyclic graph (DAG) | A graph with directed edges and no directed cycle |
| dynamic random-access memory (DRAM) | Volatile main memory |
| evidence | Preserved source, measurement, receipt, or review that supports a claim |
| feature-off | The execution condition with the candidate feature disabled |
| feature-on | The execution condition with the candidate feature enabled |
| generation performance | The rate for tokens produced after prompt processing |
| HaloFPX | The project integration fork and inference stack |
| HaloKV | The project persistent-cache design |
| HIP | The AMD GPU portability programming model |
| immutable receipt | A retained record that later work must not rewrite |
| inference | A labeled conclusion derived from evidence |
| mixture of experts (MoE) | A model architecture that routes work to selected experts |
| multi-token prediction (MTP) | A model feature that predicts more than one future token |
| nimo-1 | The current coordinator host in the Project Lead production authority |
| nimo-2 | The current remote procedure call worker host in the Project Lead production authority |
| out of memory (OOM) | A failure caused by insufficient available memory |
| prompt performance | The rate for processing input prompt tokens |
| rank | One distributed execution participant with explicit ownership |
| rank worker | A distributed process that owns one rank |
| recomputation | Cold execution used after a cache miss or invalid state |
| remote direct memory access (RDMA) | Direct access to memory on another host |
| remote procedure call (RPC) | A request and response across a process or host boundary |
| research pin | An exact source commit used for research, not a release selection |
| random number generator (RNG) | A component that produces the sampling random sequence |
| Simplified Technical English (STE) | The project writing rules in the documentation task specification |
| Strix Halo | The AMD hardware platform used by the two project nodes |
| system-on-chip (SoC) | A device that integrates processors and platform functions |
| task worker | A person or agent that performs an assigned project task |
| time to first token (TTFT) | Time from accepted request to the first generated token |

The project uses a Simplified Technical English style.
The project does not claim formal ASD-STE100 certification.
