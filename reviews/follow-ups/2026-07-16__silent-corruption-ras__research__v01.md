---
type: follow-up-research
status: proposed
priority: P0
target: HaloFPX dual-Strix-Halo inference reliability architecture
created: 2026-07-16
last_researched: 2026-07-17
risk: high
approval_required: human-and-machine
scope: memory, GPU, PCIe, NVMe, USB4/MPTCP, model/cache artifacts, distributed computation
wiki_changes: none
---

# Silent-corruption and RAS research for HaloFPX

## Executive finding

**[VERIFIED]** Linux, AMD SMI, PCIe AER, NVMe health logs, MPTCP, and model-file tooling expose pieces of an error-detection story. None of the reviewed primary sources proves that the exact two Strix Halo machines provide end-to-end error correction or silent-corruption detection. Interfaces are conditional on hardware, firmware, kernel configuration, driver ownership, enabled feature masks, namespace formatting, and protocol negotiation. [RAS-01][RAS-02][RAS-03][RAS-04][RAS-05][RAS-06]

**[OPEN]** The exact processor SKU, OEM board/firmware, memory protection, enabled AMDGPU RAS blocks, PCIe AER ownership, NVMe protection-information state, USB4 counters, and negotiated MPTCP DSS checksum have not been inventoried for this project.

**[RECOMMENDATION]** Treat the current protection level as **unknown**, not absent and not present. A counter that reads zero proves only that the exposed counter did not increment during the observation window. `N/A`, a missing sysfs node, an empty RAS mask, or an unsupported command must remain an explicit coverage gap.

**[RECOMMENDATION]** Add application-level integrity at every durable and cross-rank boundary even if lower layers report ECC, AER, TCP/MPTCP checksums, or NVMe health. Hardware telemetry is diagnostic evidence; it is not a substitute for model, cache, message, generation, and result validation.

## Decision requested

Accept this report as a P0 experiment and design backlog. Do not promote any Strix Halo RAS capability to **[VERIFIED]** until the inventory and fault-response experiments below pass on both exact nodes. No wiki pages were changed by this follow-up.

## Terms and evidence rule

| State | Meaning in HaloFPX | Required evidence |
|---|---|---|
| supported | exact hardware/driver advertises a feature | immutable inventory plus source/version |
| enabled | protection is active for the relevant block/path | runtime status and configuration |
| observable | a counter/event can be collected | pre/post read and a stable collection path |
| detection-validated | a controlled fault is reported and rejected | authorized injection with preserved raw evidence |
| recovery-validated | service returns without accepting stale/corrupt state | fault, reset/reload, recompute, and oracle comparison |
| corrected | lower layer reports correction and output oracle passes | event plus end-to-end validation |
| silent corruption | incorrect bytes or computation reach the next trust boundary without an error | digest/oracle mismatch with no prior fault indication |

**[RECOMMENDATION]** Store unsupported, unavailable, permission-denied, reset, and parser-error states as distinct values. Never coerce them to zero.

## Coverage map

| Layer | Available telemetry or mechanism | What it can establish | Unverified or uncovered risk | Required fail-closed response |
|---|---|---|---|---|
| CPU/system memory | Linux EDAC correctable/uncorrectable counters when a driver registers the controller [RAS-01] | reported controller errors for exposed domains | exact LPDDR5x ECC/parity coverage, CPU caches, DMA paths, and Strix Halo driver exposure | quarantine run; stop admission on UE; verify/reload assets and recompute |
| GPU/APU memory and engines | AMDGPU RAS feature mask, per-block CE/UE counters, bad pages; AMD SMI ECC/RAS queries when supported [RAS-02][RAS-03] | advertised/enabled blocks and reported errors | gfx1151 APU support, unified-memory coverage, caches, GFX/SDMA/MMHUB/PCIE_BIF detection, silent ALU faults | invalidate all GPU state; restart runtime; rehash/reload model; replay request |
| GPU reset/recovery | AMD SMI reset events and kernel recovery paths are version/platform dependent [RAS-03][RAS-14] | a reset occurred and some driver state changed | reset blast radius, stale allocations, queue/KV validity, peer-rank coordination | generation fence; discard all affected rank state; full context recreation |
| PCIe | AER logs, recovery callbacks, and sysfs counters when AER is supported, enabled, and OS-owned [RAS-04][RAS-05] | reported link/transaction errors and recovery attempts | masked errors, firmware-owned AER, endpoint-specific errors, undetected payload corruption | abort affected transfer/run; snapshot both endpoint and root-port counters |
| NVMe | SMART critical warnings, media/data-integrity errors, unsafe shutdowns; Identify Namespace PI capabilities/state [RAS-06][RAS-07] | controller-reported health and namespace features | actual power-loss protection, NAND-to-host coverage, PI disabled/unsupported, filesystem/page-cache corruption | checksum rejection; immutable prior generation or miss/recompute; never repair in place first |
| USB4 link/tunnel | Linux USB4/Thunderbolt topology and authorization; network/interface counters if the tunneled interface exposes them [RAS-08] | enumerated domains/devices and reported interface faults | physical/link CRC visibility, retimer/controller counters, cable independence, tunnel error propagation | end-to-end message digest and sequence check; drop/retry on mismatch |
| TCP/MPTCP | TCP checksum; optional negotiated MPTCP DSS checksum and Linux `checksum_enabled` control [RAS-09][RAS-10] | some in-flight corruption or mapping errors when checksum is active | weak 16-bit checksum, checksum disabled, corruption before checksum or after verification, application misassociation | application digest/MAC plus request, rank, generation, offset, and length validation |
| model files | external SHA-256 manifest; GGUF structural parsing [RAS-11][RAS-12] | exact file identity if independently recorded and verified | GGUF v3 specification contains no intrinsic tensor payload digest; post-load memory corruption | hash before admission and after suspicious events; quarantine mismatched artifacts |
| persistent cache | donor CachyLLama has magic/version and compatibility checks but no payload checksum or atomic generation commit [RAS-13] | limited structural/compatibility rejection | torn writes, silent bit flips, narrow identity, mixed rank generations | per-object digest, immutable generation manifest, atomic publish, corruption-as-miss |
| distributed computation | duplicate computation, sampled activation/logit digests, ABFT/Freivalds-style checks [RAS-15][RAS-16] | disagreement or invariant failure at selected boundaries | identical common-mode fault, unchecked layers/tokens, nondeterministic tolerance mistakes | cancel generation, discard rank state, retry on known-good single node |

## Findings by layer

### 1. System and unified memory

**[VERIFIED]** Linux EDAC can expose correctable and uncorrectable counts for registered memory controllers and other EDAC devices. This says nothing about a controller that does not register or a fault outside the covered domain. [RAS-01]

**[VERIFIED]** AMD's public Ryzen AI Max+ 395 page identifies Strix Halo, 256-bit LPDDR5x, up to 128 GB, two native USB4 ports, and PCIe 4.0. The reviewed public specification does not state an ECC/RAS guarantee for system memory. Absence from the page is not proof that protection is absent. [RAS-17]

**[OPEN]** Because the GPU uses unified system memory, determine whether CPU and GPU accesses share the same detection/correction path and whether errors are reported through EDAC, AMDGPU RAS, firmware CPER, Machine Check Architecture, or nowhere visible.

### 2. GPU/APU RAS and resets

**[VERIFIED]** AMDGPU documents sysfs information, debugfs error injection, per-IP-block CE/UE counts, a hardware-specific RAS mask, and bad-page interfaces. It explicitly restricts operations to supported ASIC blocks. [RAS-02]

**[VERIFIED]** AMD SMI 26.2.2 provides APIs to query ECC counts/status and RAS feature information, but its availability does not promise support on a Strix Halo APU. [RAS-03]

**[INFERENCE]** A successful GPU reset is not proof that in-memory weights, KV, command queues, streams, or remote-rank state remain valid. KFD/ROCr state must be treated as invalid across reset unless target-machine experiments demonstrate a narrower contract.

### 3. PCIe and NVMe

**[VERIFIED]** Linux PCIe AER works only when the capability and kernel support exist and firmware grants OS control. Counters may appear at the link partner rather than the device that originated the problem. [RAS-04][RAS-05]

**[VERIFIED]** NVMe defines controller health/error reporting and optional protection-information capabilities. The exact namespace configuration must be read; an NVMe device merely existing does not mean PI is enabled. [RAS-06][RAS-07]

**[RECOMMENDATION]** Use full-file/object cryptographic hashes above the storage stack. SMART `media_errors == 0` is useful telemetry, not evidence that every returned byte was correct.

### 4. USB4 and MPTCP

**[VERIFIED]** Linux exposes USB4/Thunderbolt devices and domains in sysfs, but the reviewed administration guide does not define a portable end-to-end payload-corruption counter for every tunnel/controller. [RAS-08]

**[VERIFIED]** RFC 8684 recommends MPTCP DSS checksumming and warns that corrupt data can reach the application when it is not negotiated. Linux exposes a per-network-namespace `net.mptcp.checksum_enabled` control. The DSS algorithm is the standard TCP checksum, not a cryptographic digest. [RAS-09][RAS-10]

**[RECOMMENDATION]** Require application framing with a strong digest, sequence number, byte range, connection/session ID, rank ID, plan ID, and generation epoch. Verify after reassembly and before GPU consumption. MPTCP/TCP integrity remains defense in depth.

### 5. Models, cache, and runtime state

**[VERIFIED]** The pinned GGUF specification describes magic, version, metadata, tensor descriptors, offsets, and data, but contains no `checksum`, `hash`, or `digest` field for tensor payload integrity. [RAS-11]

**[VERIFIED]** llama.cpp's security guidance instructs operators to confirm hashes of downloaded artifacts against known-good values. [RAS-12]

**[VERIFIED]** The section 14 donor audit found direct truncation of final CachyLLama cache paths, no payload checksum, no temp-file-plus-rename transaction, and a compatibility hash narrower than the complete model/runtime/state identity. [RAS-13]

**[RECOMMENDATION]** A HaloKV committed generation should include per-object cryptographic digests, lengths, format/ABI, complete model/tokenizer/template/build/backend/KV/topology identity, rank ownership, parent generation, and a manifest digest. Publish the immutable manifest only after data and directory synchronization. Any mismatch causes a miss/recompute and preserves the corrupt evidence for review.

### 6. Distributed computation

**[VERIFIED]** Research demonstrates that silent GPU faults can produce wrong computations without ordinary error signals, and that their effects are non-uniform. These studies concern other GPUs and mainly training; they justify testing methodology, not a Strix Halo failure rate. [RAS-15][RAS-16]

**[RECOMMENDATION]** Use multiple detection layers:

1. deterministic microkernel/golden-vector tests at process start and after reset;
2. content digests for weights, KV/cache objects, and every cross-rank payload;
3. finite/range/shape/sequence invariants at graph boundaries;
4. sampled activation or logit digests under a frozen deterministic canary;
5. periodic duplicate execution on the other node or CPU/reference path;
6. ABFT or randomized matrix-product verification for selected high-risk kernels;
7. final token/logit oracle checks in CI, after updates, and after any RAS event.

Duplicate execution detects disagreement, not truth. A disagreement must select a known-good reference or fail closed; it must not accept the faster or majority result without a validated policy.

## Proposed machine experiments

### P0-A — Read-only capability and counter inventory

Run on both nodes before any injection. Preserve exact commands, versions, raw JSON/text, timestamps, boot ID, kernel config, firmware, and device topology.

| Domain | Read-only checks | Pass criterion |
|---|---|---|
| identity | `lscpu`, `dmidecode`, `lspci -nnvv`, kernel/firmware/ROCm/AMD SMI versions | exact SKU, OEM, BIOS, memory, GPU, USB4, root ports, NVMe resolved |
| EDAC/MCE | enumerate `/sys/devices/system/edac`, loaded EDAC modules, journal/rasdaemon/MCE/CPER paths | every available source labeled; missing driver remains `unknown` |
| AMDGPU | read `ras_mask`, card `ras/features`, per-block count files, bad-page file; run supported AMD SMI RAS/ECC queries | distinguish supported, enabled, count, N/A, permission failure |
| PCIe | record AER capabilities, OS ownership/config, root-port and endpoint AER sysfs counters | all relevant devices/root ports mapped; baselines stored |
| NVMe | `nvme id-ctrl`, `id-ns`, `smart-log`, `error-log` in machine-readable form | DPC/DPS/PI, SMART, firmware, namespace, unsafe shutdowns known |
| USB4 | enumerate `/sys/bus/thunderbolt/devices`, domains, ports, routes, authorization, tunneled interfaces and available counters | each cable/port maps to a stable path; counter absence explicit |
| MPTCP | record sysctls, endpoints/subflows, `ss -M`, packet capture of MP_CAPABLE/DSS negotiation | actual checksum negotiation proven, not inferred from sysctl alone |
| artifacts | SHA-256 manifest for source, binaries, model shards, tokenizer/template, configuration, cache generation | all runtime inputs resolve to immutable digests |

### P0-B — End-to-end asset and cache integrity

1. Hash every model shard and runtime artifact before first load.
2. Load, execute a deterministic canary, unload, rehash, reload, and repeat.
3. Create cache objects for dense, recurrent/hybrid, MTP/speculative, sampling, and RNG state where supported.
4. Flip one bit separately in header, length, identity, payload, manifest, and rank-generation metadata on a copied/sacrificial cache.
5. Truncate each file at boundary and mid-payload offsets; mix rank generations.
6. Verify each case is rejected, evidence is preserved, and inference recomputes from an immutable predecessor or no-cache state.

Pass: zero corrupt acceptance; deterministic resumed output matches the no-cache oracle; no repair overwrites the original failing artifact.

### P0-C — Host/GPU memory path integrity

Build a small deterministic harness that fills host, pinned host, unified/GTT-visible, and GPU-accessed buffers with seeded patterns; performs CPU copy, HIP copy, SDMA-shaped copy, and compute-kernel transforms; and validates full-buffer hashes after every boundary.

Run cold boot, thermal steady state, maximum intended allocation, long soak, concurrent NVMe, and concurrent dual-link traffic. Snapshot EDAC/MCE/AMDGPU counts and journal before/after every block.

Pass: no mismatch; any injected/synthetic mismatch is detected before inference consumption. A no-error soak establishes only observed behavior for that environment, not a universal failure rate.

### P0-D — PCIe/NVMe integrity and recovery

Use a validated sacrificial filesystem or loop-backed device first; never inject on the model store or workspace.

1. Write seeded objects, sync according to each durability mode, remount/reboot, and verify hashes.
2. Exercise bounded direct/buffered/io_uring paths and queue depths while capturing AER, SMART, error log, and filesystem errors.
3. Inject torn write, lost write, stale read, bit flip, I/O error, and disk-full behavior through a software fault layer where possible.
4. Separately authorize real power-loss tests only after preserving the drive, filesystem, and cache safety plan.

Pass: prior committed generation survives or cache misses/recomputes; no mixed/partial generation is accepted; telemetry and application evidence correlate when available.

### P0-E — USB4/MPTCP payload integrity

Use a generation-tagged, sequence-numbered transport harness with a strong end-to-end digest computed before send and verified after complete receive.

1. Test cable A, cable B, both subflows, both directions, payload/queue-depth sweep, decode/NVMe contention, subflow failover, and reorder/retransmit.
2. Verify MPTCP DSS checksum negotiation in packet captures.
3. Deliberately corrupt a payload in a user-space relay before kernel checksumming to demonstrate why the application digest is necessary.
4. Inject corruption after framing in the test harness and prove receiver rejection, generation cancellation, and retry on a clean epoch.

Pass: zero corrupt delivery to GPU/decoder; all mismatches identify request/rank/generation/range; fallback never reuses partial remote state.

### P0-F — GPU reset and distributed generation fencing

This is disruptive and requires an isolated maintenance window. Begin with supported software-triggered reset only; AMDGPU RAS injection is allowed solely when the exact ASIC block advertises support and the experiment card identifies recovery/rollback.

1. Capture baseline counters/events and run an in-flight distributed deterministic request.
2. Trigger the approved reset/hang/fault at controlled graph boundaries.
3. Verify pre/post reset events, peer detection, request cancellation, and generation-epoch advance.
4. Destroy and recreate ROCr/HIP contexts, queues, allocations, streams, graphs, model mappings, and rank-local cache state.
5. Rehash/reload artifacts and rerun the request against the single-node oracle.

Pass: no stale buffer or pre-reset completion is admitted; peer rank does not commit a mixed generation; service resumes only after full health/canary gates.

### P0-G — Computation-level silent-error detection

1. Freeze deterministic prompts, model/build/backend, sampling, thread/device placement, and numeric settings.
2. Produce golden CPU or independently validated single-node outputs for microkernels, selected layer boundaries, logits, and tokens.
3. Compare identical runs on each node and distributed modes; define exact versus tolerance-based fields before examining results.
4. Inject controlled activation, collective, and output bit errors in a test build.
5. Evaluate sampled duplicate execution, activation/logit digests, range/finite checks, and ABFT/Freivalds-style verification overhead and detection rate.

Pass: every injected error class is either detected before token emission/commit or explicitly recorded as an uncovered class. Performance overhead is secondary to coverage for P0.

## Minimum runtime policy before production use

**[RECOMMENDATION]** Until P0 experiments pass:

- verify immutable model/runtime hashes at every cold load and after any reset, I/O, AER, RAS, or unexplained output event;
- use strong application digests on all cross-rank payloads and persistent cache objects;
- include rank and generation identity in every message and cache commit;
- stop admission on UE, GPU reset, increasing media/data-integrity errors, fatal/nonfatal AER, checksum mismatch, or oracle failure;
- quarantine the run and preserve logs/artifacts before restart;
- discard affected in-memory and rank-local state, then reload and recompute;
- expose missing telemetry as `coverage_unknown`, not healthy;
- maintain a verified single-node fallback.

## Priority backlog

| Priority | Work | Exit condition |
|---|---|---|
| P0 | capability inventory and immutable evidence schema | both nodes have comparable, machine-readable coverage maps |
| P0 | model/cache/message digests and generation fencing | all deliberate corruptions reject before consumption/commit |
| P0 | GPU reset and peer-rank recovery | no stale/mixed state; deterministic oracle passes after recovery |
| P0 | computation canaries and injected-fault detection | detection matrix reports covered and uncovered classes |
| P1 | long host/GPU memory and I/O soak | no mismatches; counter/timing baselines retained |
| P1 | ABFT/duplicate-execution cost study | coverage versus overhead measured per mode |
| P1 | real NVMe power-loss and USB4 physical fault experiments | approved safety card and repeatable recovery evidence |
| P2 | background scrub cadence and predictive counter thresholds | field data supports thresholds without hiding raw counts |

## Open gaps

1. **[OPEN]** Exact node SKU/OEM/BIOS and whether system LPDDR5x has exposed correction, parity, or only undocumented/internal protection.
2. **[OPEN]** AMDGPU RAS mask and per-block status for gfx1151 on the installed kernel/firmware.
3. **[OPEN]** Whether AMD SMI RAS/ECC/CPER calls support these APUs or return N/A.
4. **[OPEN]** Which agent reports unified-memory errors: EDAC, MCE/CPER, AMDGPU, firmware, or none.
5. **[OPEN]** PCIe AER OS ownership and counter coverage for USB4 controllers and NVMe root paths.
6. **[OPEN]** NVMe namespace PI capability/enablement, drive power-loss behavior, and vendor telemetry accuracy.
7. **[OPEN]** USB4/retimer/controller payload-error counters and whether both cables share an unobserved common fault domain.
8. **[OPEN]** MPTCP checksum negotiation and behavior on the exact installed kernel.
9. **[OPEN]** HaloFPX transport framing, digest algorithm, retry, and generation-cancellation semantics.
10. **[OPEN]** Complete cache/model/runtime identity and atomic commit format.
11. **[OPEN]** Determinism/tolerances for cross-node and cross-backend output comparison.
12. **[OPEN]** Which computation checks provide useful SDC coverage at acceptable inference overhead.

## Primary source ledger

### RAS-01 — Linux EDAC

- Linux kernel documentation, EDAC devices, kernel 6.15 documentation.
- https://docs.kernel.org/6.15/driver-api/edac.html
- Supports registered memory/device CE/UE concepts and sysfs exposure.
- Limitation: a generic interface does not prove a Strix Halo driver or memory protection.

### RAS-02 — AMDGPU RAS

- Linux kernel documentation, AMDGPU RAS Support, kernel 6.15.
- https://docs.kernel.org/6.15/gpu/amdgpu/ras.html
- Supports feature masks, per-block counts, bad pages, reset behavior, and conditional injection.
- Limitation: operations are ASIC/block dependent; do not run examples before target support and safety review.

### RAS-03 — AMD SMI RAS

- AMD, AMD SMI 26.2.2 / ROCm 7.2.4 production documentation, accessed 2026-07-17.
- https://rocm.docs.amd.com/projects/amdsmi/en/latest/conceptual/ras.html
- Supports ECC/RAS/CPER query availability in the tool/API.
- Limitation: much AMD RAS material is accelerator-focused; target APU support must be queried.

### RAS-04 — PCIe AER driver

- Linux kernel documentation, PCIe AER HOWTO, kernel 6.15.
- https://docs.kernel.org/6.15/PCI/pcieaer-howto.html
- Supports capability/kernel/firmware ownership, event logging, counters, and recovery.
- Limitation: AER covers reported PCIe errors, not every device-internal or silent payload error.

### RAS-05 — PCIe AER sysfs ABI

- Linux kernel testing ABI, kernel 6.15.
- https://docs.kernel.org/6.15/admin-guide/abi-testing-files.html
- Supports `aer_dev_*`/`aer_rootport_total_err_*` counter semantics and link-partner attribution caveat.
- Limitation: attributes appear only for AER-capable devices.

### RAS-06 — NVMe specifications

- NVM Express, Base Specification 2.3, ratified 2025-08-01; NVM Command Set specification family.
- https://nvmexpress.org/specification/nvm-express-base-specification/
- Supports SMART/health/error framework and current specification authority.
- Limitation: optional features and exact namespace state are device-specific.

### RAS-07 — NVMe command-set protection information

- NVM Express, NVM Command Set Specification 1.0a, ratified 2021-07-26.
- https://nvmexpress.org/wp-content/uploads/NVMe-NVM-Command-Set-Specification-1.0a-2021.07.26-Ratified.pdf
- Supports DPC/DPS/protection-information and media/data-integrity error vocabulary.
- Limitation: inventory must use the version implemented by the actual controllers.

### RAS-08 — Linux USB4/Thunderbolt administration

- Linux kernel documentation, USB4 and Thunderbolt, accessed 2026-07-17.
- https://docs.kernel.org/admin-guide/thunderbolt.html
- Supports domain/device/sysfs topology and authorization.
- Limitation: does not promise portable payload-error telemetry for all hardware.

### RAS-09 — MPTCP protocol integrity

- IETF RFC 8684, March 2020.
- https://www.rfc-editor.org/rfc/rfc8684.html
- Supports negotiated DSS checksum, failure behavior, and corruption warning when absent.
- Limitation: uses the standard TCP checksum and does not protect application identity/semantics.

### RAS-10 — Linux MPTCP checksum control

- Linux kernel documentation, MPTCP sysfs variables, kernel 6.8 reference.
- https://docs.kernel.org/6.8/networking/mptcp-sysctl.html
- Supports `net.mptcp.checksum_enabled` availability.
- Limitation: sysctl capability does not prove per-connection negotiation; verify packets/socket state.

### RAS-11 — GGUF specification

- ggml-org/ggml, commit `af97976c7810cdabb1863172f31c432dab767de7`, observed 2026-07-17.
- https://github.com/ggml-org/ggml/blob/af97976c7810cdabb1863172f31c432dab767de7/docs/gguf.md
- Supports GGUF v3 structure; an exact-text search found no checksum/hash/digest field.
- Limitation: absence of an intrinsic digest does not prevent external manifest hashing.

### RAS-12 — llama.cpp security guidance

- ggml-org/llama.cpp security policy, accessed 2026-07-17.
- https://github.com/ggml-org/llama.cpp/security
- Supports confirmation of downloaded artifact hashes against known-good values.
- Limitation: operational guidance, not an automatic runtime integrity mechanism.

### RAS-13 — CachyLLama donor audit

- Local Section 14 source-backed audit, frozen donor commit `6be745998f568e379ea197fcf827baec73ff9940`, verified 2026-07-16.
- `wiki/HaloFPX_Wiki/03_Repository_and_Engineering/14_llama_ai_and_CachyLLama_Feature_and_Patch_Inventory/`
- Supports direct-truncate/no-checksum/narrow-identity findings.
- Limitation: donor behavior is not yet HaloKV behavior.

### RAS-14 — AMD GPU reset behavior

- AMD SMI development documentation 26.5.0, GPU reset behavior, accessed 2026-07-17.
- https://rocm.docs.amd.com/projects/amdsmi/en/develop/conceptual/gpu-reset-behavior.html
- Supports full runtime recreation and platform-dependent reset effects.
- Limitation: XGMI/Instinct-oriented and development-version guidance; apply only as a conservative recovery model until Strix Halo tests pass.

### RAS-15 — OCP Silent Data Corruption in AI

- Open Compute Project, `SDC in AI` white paper v1.1, 2025-12-16.
- https://www.opencompute.org/documents/sdc-in-ai-ocp-whitepaper-ver-1-1-final-pdf
- Supports layered SDC threat/detection framing for AI systems.
- Limitation: fleet/datacenter scope; not a Strix Halo rate or capability source.

### RAS-16 — Algorithm-based fault tolerance

- Bosilca, Delmas, Dongarra, Langou, “Algorithm Based Fault Tolerance Applied to High Performance Computing,” arXiv:0806.3121, 2008.
- https://arxiv.org/abs/0806.3121
- Supports checksum/ABFT-style distributed matrix-computation detection/correction precedent.
- Limitation: different algorithms/hardware; HaloFPX coverage and cost must be measured.

### RAS-17 — AMD Strix Halo public specification

- AMD Ryzen AI Max+ 395 product specification, accessed 2026-07-17.
- https://www.amd.com/en/products/processors/laptop/ryzen/ai-300-series/amd-ryzen-ai-max-plus-395.html
- Supports Strix Halo codename, LPDDR5x memory, PCIe 4.0, and two native USB4 ports for that SKU.
- Limitation: exact project SKU/OEM remains unverified; reviewed page does not establish memory ECC.

## Review decision

Decision: **proposed / accept as P0 backlog**.

Reason: silent corruption can yield plausible but invalid model outputs or persistent state without a crash. The project currently has useful lower-layer telemetry candidates but no verified end-to-end integrity or recovery chain. Small safe improvements are not sufficient without target-machine inventory and fault-response evidence.

## Closeout review

- Correctness: source claims are limited to documented interfaces and exact revisions where available.
- Freshness: moving AMD/Linux documentation is version-labeled; installed versions remain a machine gap.
- Provenance: all substantial claims route to primary official/kernel/research sources or the pinned local donor audit.
- Reversibility: no wiki, runtime, machine, or source changes were made.
- Reusable improvement: adopt the supported/enabled/observable/detection-validated/recovery-validated ladder for all future RAS claims.
