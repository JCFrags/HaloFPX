# Scope and method

## Research cut and source discipline

The review is current to **2026-07-17** and pins each repository to an exact commit. Source paths and hashes are recorded in [`research/source-lock.json`](../research/source-lock.json) and [Source matrix](source-matrix.md). Claims are not based solely on README language; implementation files and public APIs are treated as primary evidence.

## Evidence classes

| Class | Standard used here |
|---|---|
| **OBSERVED** | A source path at the pinned commit directly performs or defines the stated behavior. |
| **INFERENCE** | The conclusion follows from observed source plus an explicit platform assumption, such as POSIX rename/fsync semantics or native C ABI layout. |
| **HALOFPX RECOMMENDATION** | Normative architecture proposed by this report. It is not attributed to an audited implementation. |
| **NOT TESTED** | No claim that the upstream engine was compiled, benchmarked, fuzzed, power-cut tested, or run against real SSD hardware. |

## Audit dimensions

Every design is evaluated across the same dimensions:

1. key composition and collision domain;
2. tenant/trust namespaces;
3. object and index format;
4. persisted metadata and compatibility fingerprint;
5. atomic visibility and durable publication;
6. checksums, authentication, and encryption;
7. byte and retention eviction;
8. thread/process/distributed concurrency;
9. restart and crash recovery;
10. corruption, partial reads, and invalid-state handling;
11. schema/version migration;
12. write amplification and SSD endurance.

## Platform assumptions

CachyLLama writes `kv_ssd_record`, `kv_ssd_index_header`, and `kv_ssd_system_record` by copying their native in-memory representation. The executable parser and Kaitai schemas in this Wiki assume:

- little-endian byte order;
- LP64 data model (`uint64_t` aligned to 8 bytes);
- the field order and implicit padding produced by a conventional GCC/Clang x86-64 ABI;
- no compiler packing pragmas.

The expected checkpoint-header size is 16,480 bytes; index header 120 bytes; system-prompt header 16,440 bytes. A different ABI can produce a different format. This dependence is itself an observed design risk, not a guarantee that these sizes apply universally.

## What was not exhaustively audited

- Every SGLang distributed L3 plugin and its external storage service.
- Every LMCache remote backend or controller deployment mode.
- Firmware-level SSD power-loss protection and vendor-specific write amplification.
- Confidentiality of KV tensors against a party already controlling the inference process.
- Semantic equivalence of every llama.cpp internal state field across every architecture.

The redesign is intentionally stricter than the audited code: where an implementation detail is unknown, the state is ineligible for a hit.
