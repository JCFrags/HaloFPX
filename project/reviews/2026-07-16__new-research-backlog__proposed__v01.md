---
type: improvement-proposal
status: proposed
target: research/prompts and wiki/HaloFPX_Wiki
created: 2026-07-16
risk: low
approval_required: human
---

# New research backlog beyond the original 86 prompts

Observed problem:

The original 86 prompts cover the main HaloFPX architecture unusually well, but several cross-cutting risks do not have a dedicated research owner. A broad keyword and section audit found substantial existing coverage for ROCm/`gfx1151`, Linux USB4 and USB4STREAM, distributed execution modes, persistent-cache correctness, generic security, reproducible builds, SBOM/SLSA, and upstream freshness. The gaps below are narrower: they either arise from current primary evidence or require a proof artifact that no existing section explicitly owns.

Evidence:

- The assembled wiki was reviewed across sections 13–17, 19–28, 29–37, 39–55, 56–65, 66, 71–72, 73–81, and the available execution/governance sections. The original prompt index was checked through section 86.
- A read-only `git ls-remote ... HEAD` check on 2026-07-17 observed the same repository heads already pinned by the wiki: `llama.cpp` `788e07dc91d266ad3162a1ce9037665656269689`, ROCmFPX `a5605a72768c6562241b248e268e33dc92787394`, CachyLLama `6be745998f568e379ea197fcf827baec73ff9940`, and `llama-ai` `1017f3dfdce3ca2b06aa9007b23295db3bb35722`. This is a point-in-time observation, not a freshness guarantee.
- Current upstream security evidence is more specific than the existing generic threat model. The llama.cpp advisory [GHSA-j8rj-fmpv-wcxw](https://github.com/ggml-org/llama.cpp/security/advisories/GHSA-j8rj-fmpv-wcxw), published 2026-03-26, describes unauthenticated RCE in the RPC `GRAPH_COMPUTE` path, lists affected versions through `b7991`, and lists no patched version. [GHSA-96jg-mvhq-q7q7](https://github.com/ggml-org/llama.cpp/security/advisories/GHSA-96jg-mvhq-q7q7), published 2026-03-18, describes an integer-overflow heap corruption path in GGUF tensor parsing and identifies `b7824` as patched. Exact reachability in HaloFPX's pins and donor code remains unverified.
- Linux exposes RAS/EDAC mechanisms and error records ([Linux RAS documentation](https://docs.kernel.org/admin-guide/RAS/main.html)); AMD SMI exposes per-GPU-block ECC/RAS queries but explicitly permits `NOT_SUPPORTED` ([AMD SMI error-query API](https://rocm.docs.amd.com/projects/amdsmi/en/docs-6.4.2/doxygen/docBin/html/group__tagErrorQuery.html)). The wiki has no dedicated inventory for what Strix Halo actually exposes or what compensating end-to-end checks are required when hardware RAS is absent.
- TLA+ is designed for precise descriptions of asynchronous/distributed systems and includes the TLC model checker ([Lamport, *Specifying Systems*](https://lamport.azurewebsites.net/tla/book-02-03-04.pdf)). Existing sections recommend properties and fault tests, but no section owns an executable state-machine model spanning epochs, output commit, reconnect, rank-local cache publication, and degraded fallback.
- The kernel fscrypt interface supports key add/remove/status operations, while documenting incomplete removal when files remain busy and other limitations ([Linux fscrypt documentation](https://docs.kernel.org/6.15/filesystems/fscrypt.html)). Section 64 recommends encryption where required, but no prompt resolves key scope, rotation, revocation, deletion, backup, and crash behavior.
- The upstream GGUF format is extensible ([GGUF specification](https://github.com/ggml-org/ggml/blob/master/docs/gguf.md)); llama.cpp's tool behavior depends on model-specific chat templates and overrides ([function-calling documentation](https://github.com/ggml-org/llama.cpp/blob/master/docs/function-calling.md)). The current wiki validates individual models and surfaces, but lacks one cross-fork semantic conformance corpus for donor integration.
- Linux now documents the open `accel/amdxdna` NPU driver, including userspace, DMA, error handling, telemetry, and scheduling ([kernel amdxdna documentation](https://www.kernel.org/doc/html/next/accel/amdxdna/index.html)). In contrast, ONNX Runtime's Vitis AI execution-provider table currently lists Ryzen AI NPU support on Windows, not AMD64 Linux ([Vitis AI EP requirements](https://onnxruntime.ai/docs/execution-providers/Vitis-AI-ExecutionProvider.html)). The project also has historical evidence that disabling IOMMU prevented NPU probing. A bounded role/exclusion decision therefore needs current source and machine evidence.

Proposed change:

Approve the following non-duplicative prompts for addition after section 86. IDs `NR-*` are backlog identifiers only; canonical section numbers should be assigned when the prompt index is amended.

## P0 — release- or architecture-blocking

### NR-01 — Inherited llama.cpp security-advisory delta and exploitability map

- **Trigger/source evidence:** [GHSA-j8rj-fmpv-wcxw](https://github.com/ggml-org/llama.cpp/security/advisories/GHSA-j8rj-fmpv-wcxw), [GHSA-96jg-mvhq-q7q7](https://github.com/ggml-org/llama.cpp/security/advisories/GHSA-96jg-mvhq-q7q7), and the current [llama.cpp security policy](https://github.com/ggml-org/llama.cpp/blob/master/SECURITY.md).
- **Affected existing sections:** 11, 13–16, 49, 53, 66, 71, 72, 80, 81, 83, 85.
- **Why this is not a duplicate:** Section 71 establishes the general trust boundary and warns against unmodified RPC, but it does not map 2026 advisories to exact pinned commits, copied code paths, build flags, listener reachability, or release gates. Section 85 watches upstream change; it does not perform exploitability analysis.
- **Prompt:** Research every upstream llama.cpp advisory applicable to the pinned baseline and planned donor code. For each advisory, record publication/revision, affected and fixed commits or build numbers, vulnerable symbol/path, whether ROCmFPX/CachyLLama inherited or modified it, compile-time inclusion, runtime reachability, privilege/listener boundary, safe reproducer availability, and mitigation status. Diff the exact pin against fixes without running public exploits on a live service. Define a deny-by-default build/listener policy, backport criteria, security regression tests, and a release-blocking advisory ledger. Treat “not reachable” as a claim requiring build and runtime evidence.
- **Required output/exit:** commit-addressed applicability matrix; source-level reachability trace; safe negative tests; proposed release gate; explicit verdict for upstream RPC, HaloFPX transport reuse, GGUF ingestion, and server exposure. No production exploit execution.

### NR-02 — Silent data corruption, hardware RAS coverage, and compensating integrity checks

- **Trigger/source evidence:** [Linux RAS/EDAC documentation](https://docs.kernel.org/admin-guide/RAS/main.html), [AMDGPU RAS documentation](https://docs.kernel.org/gpu/amdgpu/ras.html), and [AMD SMI error-query API](https://rocm.docs.amd.com/projects/amdsmi/en/docs-6.4.2/doxygen/docBin/html/group__tagErrorQuery.html).
- **Affected existing sections:** 17–24, 27, 39, 48, 54, 57–63, 69, 73, 78–81, 83.
- **Why this is not a duplicate:** Existing correctness and fault-injection work covers detected crashes, corruption, wrong results, and invalid cache state. It does not inventory whether Strix Halo LPDDR, GPU blocks, fabric, or NVMe paths expose correctable/uncorrectable error telemetry, nor define an end-to-end strategy when they do not.
- **Prompt:** Determine the error-detection and recovery coverage actually exposed on both target systems: CPU/MCA, EDAC, AER, amdgpu RAS, AMD SMI, NVMe media/integrity logs, firmware events, and USB4 link errors. Separate unsupported, unavailable, disabled, and zero-count states. Trace which inference and cache data paths lack hardware integrity protection. Design proportionate compensating checks—model/page digests, transport authentication/integrity, rank-to-rank result sentinels, periodic golden canaries, cache scrub, and quarantine—without claiming ECC where none is proven. Define alert, drain, recompute, and hardware-service thresholds.
- **Required output/exit:** two-node capability matrix with raw command receipts; data-path fault-coverage diagram; silent-corruption threat model; proposed telemetry schema and release/operations gates; safe injection plan limited to supported non-destructive mechanisms unless separately approved.

### NR-03 — Executable model of epochs, output commit, reconnect, and rank-local cache publication

- **Trigger/source evidence:** [TLA+ / TLC reference](https://lamport.azurewebsites.net/tla/book-02-03-04.pdf). Existing sections 48, 53, 58, and 63 independently define safety-sensitive state transitions.
- **Affected existing sections:** 39, 42–45, 48–49, 52–53, 58–63, 68, 72, 78, 80–81, 83.
- **Why this is not a duplicate:** Property tests and fault matrices exercise implementations, but no current artifact proves that the combined proposed protocol excludes stale output, split brain, double commit, cross-epoch reuse, or a global manifest that references nondurable rank-local state under arbitrary message loss/reorder/duplication and crash/restart.
- **Prompt:** Build a small executable specification for two ranks, coordinator, two rails, session epochs, command/credit counters, visible token commit, rank-local checkpoint durability, global-manifest publication, cancellation, reconnect, coordinator/rank crash, and single-node fallback. State invariants and liveness assumptions explicitly. Model-check bounded loss, duplication, reordering, delay, partial durability, restart, and version mismatch. Produce counterexample traces before proposing protocol corrections. Map every modeled transition and invariant to implementation assertions and section-80 fault tests.
- **Required output/exit:** versioned specification and model configuration; invariant catalogue; retained counterexample traces; protocol-change proposals; traceability table from model actions to code/tests. The model is evidence about the abstraction only, not proof of the future implementation.

## P1 — required before persistent multi-user operation or donor integration

### NR-04 — HaloKV encryption-at-rest, key lifecycle, backup, and cryptographic deletion

- **Trigger/source evidence:** [Linux fscrypt documentation](https://docs.kernel.org/6.15/filesystems/fscrypt.html), especially key removal/status and busy-file limitations; existing open question OQ-08-09 and section-64 privacy recommendation.
- **Affected existing sections:** 08–09, 56–65, 67–72, 77, 80, 83.
- **Why this is not a duplicate:** Section 64 names encryption and per-user namespaces, and section 72 mentions encrypted backups, but neither owns a concrete key hierarchy, rotation/revocation state machine, crash semantics, or proof that deletion does not leave a usable key in processes, backups, old manifests, or a peer.
- **Prompt:** Compare filesystem-level, block-level, and application/object-level encryption for rank-local HaloKV. Define threat boundaries, tenant/project key hierarchy, authenticated encryption and nonce requirements if encryption is above the filesystem, key storage/unlock, boot behavior, peer separation, rotation, revocation, backup/restore, crash recovery, busy-file behavior, secure deletion claims, and failure modes. Preserve corruption-as-miss and content-addressing semantics without exposing plaintext-derived cross-tenant identifiers. Distinguish logical deletion, cryptographic erasure, discard, and physical-media sanitization.
- **Required output/exit:** decision matrix and recommended minimal profile; key/state lifecycle diagram; negative tests for wrong/missing/rotated keys and crash points; backup/restore and deletion runbooks; explicit residual-risk statement.

### NR-05 — Cross-fork GGUF, tokenizer, chat-template, state, and API semantic conformance corpus

- **Trigger/source evidence:** [GGUF specification](https://github.com/ggml-org/ggml/blob/master/docs/gguf.md), llama.cpp [function-calling/chat-template documentation](https://github.com/ggml-org/llama.cpp/blob/master/docs/function-calling.md), and the four exact repository heads observed above.
- **Affected existing sections:** 11–15, 29–36, 56–61, 66–68, 78, 81, 85.
- **Why this is not a duplicate:** Sections 13 and 14 inventory code deltas; sections 29–36 validate models/features; section 78 owns correctness gates. None defines one minimized, versioned differential corpus that is run unchanged against upstream llama.cpp, ROCmFPX, CachyLLama, and each integration candidate to catch semantic drift at merge boundaries.
- **Prompt:** Design a legally redistributable or reproducibly generated corpus of minimal GGUF fixtures, tokenizer edge cases, named/default chat templates, tool-call/structured-output cases, recurrent/MTP/speculative state fixtures, save/restore fixtures, malformed-boundary cases, and streaming API event traces. Define which outputs must match exactly, which need numerical comparators, and which are intentionally fork-specific. Run or plan differential tests against exact commits and reduce every unexpected difference to a stable fixture. Tie corpus changes to upstream-watch triggers and integration-lane gates.
- **Required output/exit:** corpus manifest with provenance/licenses/hashes; oracle/comparator specification; fork-by-fixture result schema; minimized known-difference ledger; CI integration proposal. This does not replace model-quality evaluation or security fuzzing.

## P2 — bounded opportunity/exclusion decision

### NR-06 — Linux XDNA2 NPU suitability for auxiliary HaloFPX work

- **Trigger/source evidence:** [Linux amdxdna documentation](https://www.kernel.org/doc/html/next/accel/amdxdna/index.html), [AMD NPU userspace description](https://docs.kernel.org/accel/amdxdna/amdnpu.html), and [ONNX Runtime Vitis AI EP requirements](https://onnxruntime.ai/docs/execution-providers/Vitis-AI-ExecutionProvider.html).
- **Affected existing sections:** 08, 17, 19, 22–24, 36–39, 47, 67, 74, 76, 83, 85.
- **Why this is not a duplicate:** Sections 17 and 19 correctly avoid treating nominal NPU TOPS as useful LLM capacity and note the IOMMU dependency. They do not determine whether a supported Linux userspace path exists for any bounded auxiliary role, or formally close the NPU lane as out of scope.
- **Prompt:** Research the exact Ryzen AI Max/XDNA2 kernel, firmware, compiler, userspace runtime, model-format, operator/data-type, memory-transfer, scheduling, telemetry, suspend/reset, and licensing support on the project's Linux distribution. Inventory what is upstream versus vendor-only and what requires IOMMU. Evaluate only bounded auxiliary candidates such as embeddings, reranking, moderation, prompt classification, or a small draft model; do not assume transformer offload interoperability with llama.cpp. Define a minimal machine probe and matched end-to-end comparison including package-power contention. If no supportable path exists, record a dated exclusion decision and revalidation trigger.
- **Required output/exit:** support matrix tied to exact versions and target PCI ID; non-invasive two-node probe plan; one bounded candidate/no-go analysis; explicit keep-excluded or experiment recommendation with freshness triggers.

## Deliberately not proposed because existing prompts already own the work

| Candidate topic | Existing owner(s) | Audit disposition |
|---|---|---|
| Generic ROCm/`gfx1151` support matrix and upgrade watch | 17, 23–24, 26, 37, 74, 78, 81, 85 | Duplicate; extend those sections when a concrete release delta appears. |
| Generic Linux USB4/USB4STREAM research or kernel-patch decision | 20, 49–55, 75, 80, 85 | Duplicate; current work already covers topology, ABI, framing, multipath, buffers, benchmarks, fault handling, and watch triggers. |
| Cross-host clock synchronization | 05, 27, 55, 73, 75 | Duplicate; PTP/NTP capability and uncertainty measurement are already explicit open work. |
| Generic parser fuzzing | 53, 57, 59, 66, 71, 80 | Duplicate; NR-01 is advisory applicability, not a replacement fuzzing prompt. |
| Generic persistent-cache atomicity/corruption/endurance | 56–65, 77, 80 | Duplicate; NR-04 is specifically key lifecycle and confidentiality. |
| Generic SBOM, SLSA, signing, or reproducible builds | 16, 26, 78, 81 | Duplicate; current pages already define these release inputs and open tool choices. |
| Generic upstream freshness/backlog | 15, 82, 85 | Duplicate; each proposed prompt above has a concrete trigger and bounded artifact. |

Expected benefit:

The backlog closes six high-value blind spots without diluting the existing 86-section structure. It converts newly visible security evidence into a release decision, makes silent-corruption assumptions explicit, subjects the distributed commit design to state-space exploration before implementation, resolves cache confidentiality semantics, creates a stable donor-integration oracle, and either finds a bounded NPU role or closes it with evidence.

Possible regression:

- Adding all six as full wiki sections immediately could create numbering churn and duplicate facts while sections 82–86 are still settling.
- Formal modeling and crypto design can become unbounded unless the required state variables and threat boundary remain narrow.
- Security reproducer work could create operational risk if performed against a live listener or with public exploit code; NR-01 therefore requires source analysis and safe negative tests by default.
- NPU investigation could distract from the primary HIP/Vulkan path; NR-06 is P2 and must end in a bounded go/no-go decision.

Validation required:

1. Human owner confirms priorities and assigns canonical section IDs or records these as section-85 backlog items.
2. Before dispatch, compare each prompt against the final merged sections 82–86 and remove any newly introduced duplication.
3. Require primary, commit-addressed evidence and the normal seven-file output standard for any promoted prompt.
4. Review each result through `sources -> wiki -> implementation decision`; do not treat a proposal or model-check result as machine validation.
5. For NR-01, prohibit testing against a live service and require explicit approval before any exploit-derived reproducer is executed.
6. For NR-02 and NR-06, distinguish unsupported telemetry/runtime from a zero-error or zero-benefit measurement.
7. For NR-03 and NR-04, obtain design review before their proposed state machines or cryptographic choices become implementation authority.

Decision: pending
