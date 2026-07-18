---
type: research-prompt-backlog
status: dispatch-ready
created: 2026-07-17
scope: pre-fork external research only
canonical_wiki_edited: false
---

# Pre-fork further Internet research prompts

## Purpose and dispatch rule

This is the deduplicated external-research backlog for preparing the ROCmFPX-based HaloFPX fork. It consolidates the reviews of the twelve imported Wiki packages, canonical Section 85, fork-plan reviews, the donor patch map, the ROCmFPX pin-to-head delta, and the 2026-07-17 live two-node evidence.

Dispatch only the prompts below to an Internet research agent, as one independently downloadable LLM-Wiki folder per prompt. Do **not** substitute external summaries for the local source archaeology or machine experiments listed at the end. Imported research remains candidate evidence and must follow `sources -> wiki -> implementation decision`; no returned Wiki approves a baseline by itself. The accepted fork plan requires no additional Internet research before local/read-only Phase 0A (L00A); these priorities apply to later named gates.

Send each complete prompt section, including its **Priority**, research instructions, **Expected output**, and **Decision unblocked** field. `Decision unblocked` gives the research agent the project context and evidence target; it does not authorize the agent or its returned Wiki to make, approve, or implement that decision.

Priority meanings:

- **P0:** external evidence needed before donor/test-asset promotion, a security-sensitive exposure decision, or locking the first implementation source/toolchain candidate; it does not block L00A.
- **P1:** external evidence for later persistence, multi-user operation, transport graduation, firmware qualification, or Phase 2 model work.
- **P2:** bounded optimization or exclusion decision; it must not delay Phase 0A.

## Prompts

### PF-IR-01 — Current llama.cpp/RPC/GGUF security applicability and backport ledger

**Priority:** P0.

Research every current official security advisory and security-relevant fix that could affect the HaloFPX source and deployment boundaries: `ggml-org/llama.cpp`, `charlie12345/ROCmFPX`, `fewtarius/CachyLLama`, the deployed predecessor `charlie12345/rocmfp4-llama`, GGUF/model ingestion, the RPC server/client, HTTP server, state/cache parsing, and relevant bundled dependencies. Start from the exact project snapshots `ROCmFPX@a5605a72768c6562241b248e268e33dc92787394`, candidate `ROCmFPX@61f2f2d7bc4955e9bca821095ef69125837133b5`, `CachyLLama@6be745998f568e379ea197fcf827baec73ff9940`, `llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722`, `llama.cpp@788e07dc91d266ad3162a1ce9037665656269689`, comparison snapshot `llama.cpp@86d86ed4396b4130922f7b9af26e3d9fc11a591b`, and deployed source `rocmfp4-llama@4860505ee322091f0f61eba77d6ad49be88cf4ea`. For each advisory, identify affected/fixed commits, vulnerable symbols and preconditions, fork source equivalence or divergence, compile-time inclusion, runtime reachability, listener/trust boundary, and a safe negative-regression test. Include all security changes after the known RPC fix, not only the previously reviewed advisory. Do not run exploit code or probe a live service.

**Expected output:** a downloadable folder containing everything styled like an LLM Wiki. The folder must preserve raw primary-source captures and manifests, use exact commits/tags/blob IDs and access dates, label every claim `[VERIFIED]`, `[MEASURED]`, `[INFERENCE]`, `[ASSUMPTION]`, `[RECOMMENDATION]`, or `[OPEN]`, record licenses and reuse limits, include hashes and a source-to-claim manifest, and distinguish source-equivalence evidence from deployed-binary proof.

**Decision unblocked:** the external advisory/applicability ledger for the deny-by-default RPC/server build and listener policy, required security-backport candidates, safe-lab prerequisites, and the security release gate. Deployed-binary equivalence, loaded-library provenance, and negative reachability remain local evidence.

### PF-IR-02 — Linux 7.2 USB4STREAM, thunderbolt-net, and MPTCP deployment-source dossier

**Priority:** P1.

Trace USB4STREAM from its introduction commits through the current stable/mainline state and any distro/CachyOS packaging or backports. Pin the exact `thunderbolt-stream` source, UAPI/configfs ABI, `/dev/tbstreamX` semantics, stream lifecycle, buffer ownership, blocking/poll/error behavior, teardown/reconnect behavior, and compatibility changes. At the same selected-kernel candidates, audit `thunderbolt-net` framing, MTU, rings/queues, NAPI, error paths, teardown, and statistics, plus Linux MPTCP path-manager/scheduler behavior and the exact meanings/defaults of `close_timeout`, `stale_loss_cnt`, subflow fallback, and related controls. Map publicly accessible USB4, Thunderbolt/USB4NET, PCIe-tunneling, and MPTCP specification revisions or errata to the released Linux implementation, covering link directionality/full duplex, tunnel resource allocation, flow control, reset/retrain, error reporting, simultaneous paths, peer trust, cable/controller discovery, and interoperability. Follow primary kernel documentation, commits, release notes, distro source/package records, standards/errata, and relevant maintainer mailing-list threads; record member-only or inaccessible normative material as an evidence gap. Explicitly determine whether registered buffers, `mmap`, `splice`, zero-copy, directional stream preference, or non-Linux interoperability are implemented, proposed, unsupported, or merely speculative. Do not infer application goodput or GPU-direct behavior from link rate or source presence, and do not use generic marketing throughput claims as proof.

**Expected output:** a downloadable folder containing everything styled like an LLM Wiki. The folder must preserve raw primary-source captures and manifests, use exact kernel/distro commits, tags, configs, package revisions, public specification/errata revisions and access dates, apply the project claim labels literally, record licenses/access constraints and patch provenance, include hashes and a source-to-claim manifest, keep unmerged mailing-list proposals clearly separate from released behavior, and include a standards-to-Linux-interface map with unresolved proprietary gaps.

**Decision unblocked:** the externally supportable kernel/package candidate roster, ABI target, TCP/MPTCP fallback boundary, normative-versus-machine-tested transport assumptions, and whether a source/backport lane is sufficiently evidenced to enter reversible machine qualification. Installation or selection still requires the local compatibility and rollback experiments.

### PF-IR-03 — Official gfx1151 compute-stack and signed-artifact compatibility tuple

**Priority:** P0.

Build a lane-aware, component-level dossier for AMD Strix Halo/gfx1151 covering the current official Ryzen/Linux support matrix, ROCm/HIP/HSA/rocBLAS/hipBLASLt/rocWMMA/LLVM components, Mesa/RADV, amdgpu/KFD and firmware requirements, RCCL packaging, compiler resource directories, known issues, and the separate ROCm 7.2.x versus Core SDK/TheRock 7.14.x version lanes. Capture the exact ROCm 7.14.0 gfx1151 artifact URL, checksum/signature, contents, SBOM/licenses, maturity/preview statement, supported OS/kernel bounds, and rollback/removal guidance. Treat the installed two-node tuple (ROCm packages 7.2.4, Mesa 26.1.4, kernel 7.1.3 at the 2026-07-17 capture) only as a local comparison point, not official qualification. Identify authoritative package and source provenance needed to reproduce a build without mixing incompatible lanes.

**Expected output:** a downloadable folder containing everything styled like an LLM Wiki. The folder must preserve raw primary-source captures and manifests, pin exact versions/commits/tarball hashes/signatures and access dates, use literal claim labels, inventory every applicable license and notice, include a component dependency/source manifest, and clearly distinguish documented support, preview availability, known issues, and unverified combinations.

**Decision unblocked:** the official component/source inputs for a control tuple and separately testable candidate tuple, the external portion of the OPEN-BASE-01 build manifest, and whether Linux 7.2 USB4STREAM has a plausible candidate lane without silently redefining the gfx1151 compute baseline. Both-node builds and runtime qualification remain local.

### PF-IR-04 — Donor, test-asset, and release-artifact licensing evidence

**Priority:** P0.

Produce a primary-source licensing and provenance dossier for the exact public components: `ROCmFPX@a5605a72768c6562241b248e268e33dc92787394` and `61f2f2d7bc4955e9bca821095ef69125837133b5`; `CachyLLama@6be745998f568e379ea197fcf827baec73ff9940`; `llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722`; and their vendored/generated assets and submodule/gitlink boundary. Also research authoritative public license evidence for upstream fixtures, model/tokenizer/template artifacts, schemas, scripts, Web UI assets, and documentation types that could be proposed for promotion. Resolve repository-level and file-level license/SPDX/notice gaps where authoritative public evidence exists. Distinguish MIT code adaptation, GPL operational behavior/specification, separately licensed documentation, facts/ideas, generated files, models/test corpora, and distribution dependencies. Define the evidence a human maintainer/legal reviewer needs for a concrete binary/source release, including SBOM, source-to-binary mapping, corresponding-source obligations, notices, and clean-room role separation. Do not assume access to the local twelve-package intake, present the research as legal advice, approve copying where the license remains unknown, or claim to replace the local full-tree/proposed-artifact scan.

**Expected output:** a downloadable folder containing everything styled like an LLM Wiki. The folder must contain raw authoritative license/source captures, exact commits/blob IDs and access dates, literal claim labels, per-file/per-component license and attribution manifests, hashes, unresolved-license entries, and a machine-readable provenance chain. It must state which artifacts may be referenced only, which may be independently regenerated, and which remain blocked pending human permission.

**Decision unblocked:** the external-evidence portion of OPEN-LIC-01: candidate donor/test-asset license records, proposed GPL separation evidence, and notice/SBOM obligations. Final admissibility, clean-room roles, and the release/distribution model remain human decisions over the exact locally proposed artifact tree.

### PF-IR-05 — Exact 200–230 GB model artifact shortlist and immutable identity

**Priority:** P1.

Research publisher-authoritative candidate models whose **stored selected quantization** is approximately 200–230 GB and that plausibly match the user's large-model workload. Start from the imported candidate catalog but refresh every candidate at an immutable publisher revision. For each serious candidate, record architecture and state requirements (including MoE/MLA/recurrent/MTP/speculative features), parameter and active-parameter counts, context limits, tokenizer and chat template, tool-use behavior, license/gating/acceptable-use terms, official weight shard list and exact bytes/hashes where published, available GGUF/conversion lineage, quantization options, calibration/imatrix provenance, and evidence of support or missing operators in relevant exact llama.cpp/ROCmFPX source revisions. Separate publisher facts, third-party conversions, static source support, and machine validation. Do not claim that a model fits, loads, runs correctly, or meets quality targets without local evidence.

**Expected output:** a downloadable folder containing everything styled like an LLM Wiki. The folder must preserve raw publisher and primary runtime-source captures, exact model/source commits and access dates, literal claim labels, model and artifact licenses, shard/file manifests with hashes or explicit unavailable fields, a source-to-claim manifest, and reproducible refresh instructions that do not silently follow moving `main` or `latest` references.

**Decision unblocked:** an immutable Phase 2 artifact/quantization shortlist, model identity and cache-invalidation manifest inputs, license/gating evidence, and the required local backend/conversion/quality preflights. Final target selection still requires a human workload/quality decision and local qualification.

### PF-IR-06 — Persistent-cache filesystem, io_uring, and crash-durability contract

**Priority:** P1.

Research the authoritative crash-consistency and I/O semantics needed for HaloKV and the separate RPC model-tensor cache on Linux. Cover the deployed filesystem first, then ext4, XFS, and btrfs only where they are realistic alternatives: temporary-file creation, `O_TMPFILE`, exact-length writes, `fdatasync`/`fsync`, directory sync, atomic rename/exchange, overwrite behavior, delayed allocation, direct-I/O alignment, reflink/copy-on-write hazards, ENOSPC/EIO/short-write handling, fallocate/discard, mount-option effects, file locking and multi-process fencing. Trace current kernel and liburing semantics for registered files/buffers, direct I/O, asynchronous cancellation, late CQEs, linked operations, resource teardown, and crash boundaries. Identify portable minimum guarantees versus filesystem-specific optimizations and include Windows/macOS/network filesystems only as explicit non-target portability notes. Do not turn documentation into a claim that the future implementation is durable.

**Expected output:** a downloadable folder containing everything styled like an LLM Wiki. The folder must preserve raw official documentation/source/standards captures, exact kernel/liburing commits and filesystem documentation versions, access dates, literal claim labels, licenses, hashes and raw manifests, and a table mapping each required guarantee to an authoritative source and a local fault test.

**Decision unblocked:** source-backed options for OPEN-FMT-01 and OPEN-STORAGE-01 durability primitives, publication ordering, supported-filesystem profile, multi-process writer policy, and the minimum kill/ENOSPC/EIO experiment matrix before persistent writes. The future implementation must still prove the selected contract on the deployed filesystem.

### PF-IR-07 — HaloKV encryption, tenant identity, key lifecycle, backup, and deletion

**Priority:** P1.

Compare current authoritative options for rank-local persistent cache protection: fscrypt, dm-crypt/LUKS, and application/object-level authenticated encryption. Define threat boundaries, tenant/project/system-prefix key hierarchy, principal binding, AEAD/nonce and associated-data requirements, key storage/unlock, boot behavior, peer/rank separation, rotation, revocation, busy-file behavior, process key retention, backup/restore, crash recovery, export, quarantine, and incident response. Explain how encryption interacts with content addressing, deduplication scope, privacy-preserving identifiers, manifests, HMAC/digests, cross-tenant timing leakage, cryptographic erasure, logical deletion, discard, and physical-media sanitization. Preserve the invariant that any unknown, corrupt, unauthenticated, wrong-key, or mismatched object is a miss/recompute. Use standards and official OS/kernel cryptographic documentation; do not invent compliance or secure-deletion guarantees.

**Expected output:** a downloadable folder containing everything styled like an LLM Wiki. The folder must preserve raw standards and primary platform documentation, exact revisions and access dates, literal claim labels, licenses, hashes and raw manifests, a threat-model-to-control matrix, and explicit residual risks and unsupported claims.

**Decision unblocked:** the standards-backed option set and residual-risk ledger for OPEN-SCOPE-01 encryption/key policy, tenant and public/system-prefix deduplication scope, backup/deletion runbooks, and the minimum multi-user persistent-cache security profile. Principal authority, sharing policy, and key ownership remain human decisions.

### PF-IR-08 — RCCL/two-host collective and network-plugin support boundary

**Priority:** P1.

Audit the active `ROCm/rocm-systems/projects/rccl` authority and the exact RCCL component associated with candidate ROCm lanes. Determine documented and source-defined support for two ranks on separate Strix Halo hosts over socket interfaces, interface selection, topology discovery, pointer/buffer registration, DMA-BUF or network-plugin paths, communicator creation, timeouts, abort, asynchronous errors, process/rank failure, reconnect/reinitialization, and version/plugin ABI rules. Compare the real available mechanisms with the needs of two-way tensor parallelism; clearly separate standard collectives, custom ggml RPC, pipeline/layer split, and a hypothetical custom USB4STREAM transport. Identify upstream tests and maintainer statements relevant to a two-host Ethernet-over-USB4 socket topology. Do not claim RCCL works on this APU pair or that it provides GPU-direct over USB4 without machine evidence.

**Expected output:** a downloadable folder containing everything styled like an LLM Wiki. The folder must preserve raw AMD/ROCm source, documentation, issues/PRs or maintainer statements, exact commits/tags and access dates, literal claim labels, licenses, hashes and source manifests, and distinguish normative APIs from issue commentary and inferred applicability.

**Decision unblocked:** whether RCCL belongs in the local experiment matrix, which documented collective/transport abstractions merit adapters, what failure semantics may be tested, and whether custom transport work appears necessary for Phase 2. Only two-node machine evidence can establish actual suitability.

### PF-IR-09 — Exact hardware firmware, security bulletin, errata, and RAS authority map

**Priority:** P1.

Using the measured target identity as the search key—Nimo Direct MME3L / NIMO Mini PC board v1.0, BIOS 3.05 dated 2025-10-11, AMD Ryzen AI MAX+ 395/gfx1151, AMD PCI device `1002:1586` revision c1, Crucial P310 1 TB firmware VACR001, and the observed USB4 router/controller identities where authoritative mapping is possible—locate official OEM, AMD Product Security, LVFS/fwupd, linux-firmware, Crucial/Micron, kernel, PCIe/AER, EDAC/MCA, amdgpu RAS and USB4 errata/security authorities. Record which exact products are covered, fixed firmware availability, signature and rollback properties, disclosure limitations, and which correctable/uncorrectable telemetry is documented as supported, unsupported, or unknown for Strix Halo. Treat absence of public information as `[OPEN]`, not evidence of no errata or no fault risk.

**Expected output:** a downloadable folder containing everything styled like an LLM Wiki. The folder must preserve raw official bulletins, firmware metadata, source/docs captures and manifests, use exact product/revision/firmware identifiers and access dates, literal claim labels, licenses/reuse limits, hashes, and a product-to-advisory applicability table with explicit confidence and gaps.

**Decision unblocked:** the source-backed firmware/security watch list, candidate rollback sources, pre-deployment update holds, telemetry gaps, and integrity-check candidates where hardware RAS coverage is absent or unproven. Firmware rollout and hardware-service thresholds still require local inventory, rollback proof, and human approval.

### PF-IR-10 — Legally redistributable cross-fork semantic conformance assets

**Priority:** P0.

Identify legally redistributable or deterministically generated minimal assets for a differential corpus run unchanged against exact llama.cpp, ROCmFPX, CachyLLama, and HaloFPX candidates. Cover tiny GGUF tensors/models, tokenizer edge cases, default/named chat templates, special-token behavior, tool calls/structured output, streaming API traces, malformed-boundary inputs, state save/restore, and—where a minimal legitimate artifact exists—recurrent, MTP/speculative, sampler/RNG/grammar state. Every logical fixture must resolve to one immutable file, record, byte range, JSON pointer, or deterministic recipe. Research upstream test assets first, then publisher/open-data sources; exclude assets whose terms do not permit the intended storage and execution. Separate exact-output oracles from numerical comparators and fork-specific expected rejection.

**Expected output:** a downloadable folder containing everything styled like an LLM Wiki. The folder must preserve raw primary provenance and license evidence, exact source/model commits and access dates, literal claim labels, per-fixture licenses and hashes, deterministic generation recipes, dependency manifests, and an explicit `required` / `expected-reject` / `not-applicable` / `open` applicability proposal. Candidate scripts must remain unexecuted evidence unless separately qualified.

**Decision unblocked:** the external provenance and license inputs to OPEN-TEST-01 and OPEN-API-01, plus a candidate fixture roster and generation recipes for a stable semantic oracle. Local source-derived applicability, static review, isolated execution, and human approval of the exact accepted asset manifest remain mandatory.

### PF-IR-11 — Linux XDNA2 suitability for a bounded auxiliary role

**Priority:** P2.

Research the exact Linux support boundary for the Ryzen AI MAX+ 395 XDNA2 NPU: upstream `amdxdna` kernel driver, firmware, compiler, userspace runtime, model formats, operator/data-type coverage, DMA/memory movement, IOMMU dependency, scheduling, telemetry, reset/suspend, packaging, and licenses. Evaluate only bounded auxiliary roles such as embeddings, reranking, moderation, prompt classification, or a small draft model. Do not assume llama.cpp transformer offload, shared coherent memory, or useful end-to-end performance. Identify what is upstream, vendor-only, Windows-only, preview, unsupported, or missing on the target distro.

**Expected output:** a downloadable folder containing everything styled like an LLM Wiki. The folder must preserve raw primary kernel/vendor/runtime sources and manifests, exact commits/versions/PCI applicability and access dates, literal claim labels, licenses, hashes, and a minimal non-invasive local probe plan. It must be willing to conclude `keep excluded`.

**Decision unblocked:** a dated keep-excluded decision or one tightly bounded auxiliary NPU experiment; this must not distract from the primary HIP/Vulkan fork path.

## Local-only work and experiments — do not send to an Internet research agent

These gaps are closed by the preserved local clones, design decisions, isolated tooling review, or nimo-1/nimo-2 experiments. External research cannot substitute for them.

1. **ROCmFPX base qualification:** matched clean builds and tests of `a5605a72768c6562241b248e268e33dc92787394` versus `61f2f2d7bc4955e9bca821095ef69125837133b5`, including CPU/HIP/Vulkan TurboQuant/FA numerical parity, graph capture, MTP, cache restore, RPC, memory peaks and retained raw logs (`OPEN-PIN-01`).
2. **Donor source archaeology:** derive exact introduction commits, hunk ownership, dependency closure, upstream-equivalence and deletion hazards from the existing complete local Git objects; convert the accepted behavior set into P3 provenance records (`OPEN-PROV-01`).
3. **State/API ownership map:** inspect the selected ROCmFPX source for transformer, recurrent/hybrid, MTP/speculative, sampler, RNG, grammar, slot, streaming and output-commit ownership; freeze compatibility fixtures (`OPEN-STATE-01`, `OPEN-API-01`).
4. **Candidate test-asset intake:** hash/license/static-review imported scripts, authorize an isolated environment, run only synthetic qualification there, retain raw results, and promote only a reviewed exact asset manifest (`OPEN-TEST-01`).
5. **HaloKV v1 design:** define object kinds, required-stream profiles, recovery capsule, compatibility fingerprint, parser bounds, local/global manifest composition, transaction point, deduplication scope, quotas/GC and migration policy (`OPEN-FMT-01`).
6. **Executable protocol model:** locally model-check epochs, record counters, anti-replay, rank ownership, cancellation, output commit, rank-local durability, global publication, reconnect and fallback; map counterexamples to implementation assertions and fault tests.
7. **Disposable cache prototypes:** prove validate-before-mutate import, suffix-only reuse, corruption-as-miss, one-stream-at-a-time mutation, cross-language fingerprint vectors, RPC tensor-cache read-time rehash, exact-length checks and atomic publication.
8. **Storage/capacity remediation:** resolve nimo-1's approximately 43 GiB free-space constraint and existing approximately 112 GiB RPC tensor cache without deleting preserved evidence; define quotas, reserve, staging, rollback and disposable fault-test locations (`OPEN-STORAGE-01`).
9. **Paired machine manifest:** lock package closure, loaded DSOs, kernel configs/modules, firmware hashes, compiler resources, Mesa/RADV identity, swap/zswap/boot policy, device permissions and node-role parity on both systems (`OPEN-BASE-01`).
10. **USB4 baseline measurements:** recapture physical/sysfs rail identity; measure isolated/simultaneous one-rail and dual-rail directional/bidirectional goodput, latency distributions, CPU/IRQ cost, MPTCP subflow attribution, fallback and failure independence.
11. **USB4STREAM qualification:** add a reversible candidate kernel without replacing stable/LTS rollback; boot one idle node first; prove ROCm/NVMe/USB4NET health; then test one stream, dual streams, error handling, CPU cost and fallback. No live-service reboot is implied by this backlog.
12. **GPU-to-peer end-to-end path:** measure GPU-produced to peer-GPU-consumed transfers, staging/copy costs, synchronization and tail latency; do not infer GPU-direct or tensor-parallel benefit from host microbenchmarks.
13. **Distributed correctness/failure:** define rank/process/KV/sampler/output ownership, then test cancellation, disconnect, malformed records, link/rank/process loss, epoch fencing, replay, restart/rejoin and single-node reduced-model or explicit-failure behavior.
14. **Large-model fit and support:** after PF-IR-05 selects artifacts, run exact conversion/tensor audits, CPU reference loads, HIP/Vulkan/RPC operator coverage, progressive context fit, realized placement, per-node memory/GTT/PSI/swap accounting and safe OOM rejection.
15. **Matched performance and quality:** calibrate operation-specific tolerances against an independent CPU oracle; measure target quality, TTFT, prefill/decode throughput, tails, thermals, wall power and failures with randomized paired runs. Derive `OPEN-ACCEPT-01` thresholds only from retained local pilot distributions.
16. **Durability/endurance/fault campaign:** on disposable data, run kill/restart/ENOSPC/EIO/partial-write/corruption tests at every publication boundary; measure physical writes, WAF, compaction/migration/rotation, SMART/TBW and quarantine behavior.
17. **Security operations:** verify deployed binary and loaded-library provenance, private binding/firewall, service user/capabilities, peer restriction and negative reachability before any RPC fault test; review world-writable `/dev/kfd` and render-node permissions before multi-user exposure.
18. **Human governance decisions:** repository owner/name/visibility, remote creation/push authority, protections/signing keys, security owner, release/waiver authority, distribution model, clean-room roles, anonymous/public sharing, key ownership and rollback policy are human decisions, not Internet research.

## Deduplication notes

- Do not commission a generic ROCmFPX/CachyLLama feature inventory: the local clones and donor patch map already own it.
- Do not commission a generic dual-USB4 benchmark Wiki: only the source/ABI/standards unknowns are external; performance and independence are local measurements.
- Do not split public USB4/PCIe errata into a second transport package: PF-IR-02 owns the standards-to-Linux mapping so ABI, errata, and implementation conclusions cannot drift.
- Do not commission generic cache atomicity, conformance, formal-methods, or model-fit research without an exact decision boundary; those now have local owners above.
- Do not treat current upstream heads, releases, benchmark claims, or returned Wiki recommendations as automatic baseline updates.

## Internal synthesis basis

- `wiki/HaloFPX_Wiki/12_Project_Execution_and_Governance/85_Internet_Research_Backlog_Upstream_Watch_and_Knowledge_Freshness/`
- `reviews/intake/2026-07-17__*.md`
- `reviews/plans/2026-07-17__rocmfpx-llama-ai-fork-plan__draft__v03.md`
- `reviews/plans/2026-07-17__rocmfpx-llama-ai-fork-plan__review__v01.md`
- `reviews/plans/2026-07-17__rocmfpx-llama-ai-fork-plan__review__v02.md`
- `reviews/plans/2026-07-17__rocmfpx-llama-ai-fork-plan__review__v03.md`
- `sources/repositories/manifest.yaml`
- `sources/measurements/2026-07-17-strix-halo-live-inventory/`
