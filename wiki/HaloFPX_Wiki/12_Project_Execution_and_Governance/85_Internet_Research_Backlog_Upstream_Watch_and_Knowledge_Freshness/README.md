---
section_id: "85"
title: "Internet Research Backlog, Upstream Watch, and Knowledge Freshness"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories:
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
  software_versions:
    - "Linux v7.2-rc3; ROCm 7.2.1 Ryzen-supported control, 7.2.3 research baseline, Core SDK 7.14.0 unqualified candidate, installed tuple OPEN; Mesa 26.1.5; RCCL 2.27.7 baseline; liburing 2.15 research snapshot"
  hardware_revisions:
    - "two intended matched AMD Strix Halo/gfx1151 systems; exact BOM, OEM, BIOS, controllers, cables, and revisions open"
related_sections: ["02", "04", "11", "13", "15", "18", "23", "24", "29", "50", "62", "72", "84", "86"]
---

# 85 - Internet Research Backlog, Upstream Watch, and Knowledge Freshness

## PF-IR return status — 2026-07-18

**[VERIFIED]** Ten requested research archives—PF-IR-01, 02, 03, and 05 through
11—were preserved and extracted under the dated intake at [S85-18]. PF-IR-04 is
the only outstanding package. **[INFERENCE]** The returned material narrows the
external option set but leaves its implementation, machine, security, license,
and human-decision gates open. The reviewed disposition is authoritative for
project routing; package self-verdicts are not [S85-18].

| Return group | Current project effect |
|---|---|
| Security and conformance (01, 10) | Start local artifact/source review; no release exposure or fixture promotion approved |
| Kernel, compute and RCCL (02, 03, 08) | Admit separate reversible candidate experiments; no combined upgrade or target support claim |
| Models (05) | Shortlist inputs available; human workload selection and machine preflight remain required |
| Persistence and keying (06, 07) | Source-backed design constraints available; format, authority and destructive fault tests remain open |
| Hardware and XDNA2 (09, 11) | Firmware rollout stays held; XDNA2 stays excluded from the primary path |

The next external intake is PF-IR-04. The immediate local sequence is maintained
in the [research-return synthesis](../../../../knowledge/pre-fork-research-return-synthesis.md).

## Decision-use summary

**[VERIFIED]** On 2026-07-17, live default-branch checks returned the same four project heads frozen by Sections 11-15: llama.cpp `788e07d`, ROCmFPX `a5605a7`, CachyLLama `6be7459`, and llama-ai `1017f3d` [S85-02, S85-03, S85-04, S85-05]. This is a checked-at snapshot, not a promise that those refs remain unchanged.

**[VERIFIED]** Current upstream observations relevant to the stack are Linux mainline `v7.2-rc3` dated 2026-07-12; the ROCm `7.2.1` Ryzen/gfx1151-supported control lane; ROCm `7.2.3` dated 2026-05-04 as the research baseline; the separate TheRock/Core SDK `7.14.0` lane released 2026-07-16 as an unqualified candidate; Mesa stable `26.1.5` dated 2026-07-15; RCCL `2.27.7` in the 7.2.3 baseline with active development moved to `ROCm/rocm-systems/projects/rccl`; and liburing `2.15` dated 2026-06-29 [S85-06, S85-07, S85-08, S85-09, S85-10, S85-11, S85-12, S85-13, S85-16]. The exact installed ROCm tuple remains **[OPEN]**, and these versions have not been qualified together on either target machine.

**[INFERENCE]** HaloFPX has a cross-stack compatibility problem, not a “latest version” problem. USB4STREAM is in the Linux 7.2 development line, while the documented gfx1151 ROCm support tuple is narrower; release feeds alone cannot establish a supported combined lane [S85-07, S85-08, S85-09, S85-10].

**[RECOMMENDATION]** Treat monitoring as an evidence intake process:

```text
feed observation -> immutable candidate source -> semantic diff -> affected claim IDs
-> review -> source or machine revalidation -> wiki revision -> decision/ADR impact
```

An observed change never silently replaces a verified page, frozen baseline, or decision. See [procedures and checks](procedures_and_checks.md#safe-watch-cycle) and [design implications](design_implications.md#evidence-promotion-and-decision-propagation).

## Highest-priority backlog

| Priority | Backlog ID | Research outcome | Why now |
|---|---|---|---|
| P0 | IR-85-01 | Resolve exact node BOM, OEM support URLs, BIOS/firmware versions, USB4 controller/retimer IDs, and cable revisions | Hardware errata cannot be scoped without product identity. |
| P0 | IR-85-02 | Reconcile Linux 7.2 USB4STREAM with a documented gfx1151 ROCm/Mesa lane and distro packaging | The proposed transport and compute support boundaries currently do not coincide. |
| P0 | IR-85-03 | Establish security/data-integrity watch for AMD bulletins, kernel/amdgpu, firmware, cache, and I/O fixes | Security or corruption changes must preempt scheduled review. |
| P1 | IR-85-04 | Diff each new llama.cpp head/release against the HaloFPX pin for model graph, GGUF/type IDs, backends, RPC, state, server, and tests | High-churn upstream can invalidate patch and cache assumptions. |
| P1 | IR-85-05 | Track ROCmFPX/CachyLLama donor heads and provenance against pinned feature maps | Donor APIs and custom formats are experimental and unversioned. |
| P1 | IR-85-06 | Qualify ROCm, HIP/HSA, RCCL, Mesa/RADV, kernel/amdgpu, and firmware as one tuple | Component release status is not tuple compatibility. |
| P1 | IR-85-07 | Pin complete model packages: config, tokenizer, chat template, license, weights, converter, and runtime support | A model-repository SHA alone does not prove deployable identity. |
| P2 | IR-85-08 | Watch io_uring/liburing fixes affecting cancellation, registered resources, direct I/O, and completion ownership | HaloKV asynchronous I/O semantics remain unmeasured. |
| P2 | IR-85-09 | Track performance-only kernel/compiler/backend changes for controlled A/B qualification | Performance claims require matched machine evidence. |
| P3 | IR-85-10 | Quarterly discovery sweep for new models, transport APIs, and relevant standards | Discovery must not bypass candidate and validation gates. |

Detailed feeds, queries, cadence, expiry, and triggers are in [facts and constraints](facts_and_constraints.md). Open gaps and closure evidence are in [open questions](open_questions.md).

## Research split

1. **Internet/source-code now:** poll trusted feeds, pin immutable revisions, diff relevant paths, triage issues/PRs as provisional evidence, and map changes to claim IDs.
2. **On-machine:** inventory exact installed tuple and hardware identity, reproduce candidate fixes, run targeted correctness/performance checks, and prove rollback/single-node fallback.
3. **Contingent decisions:** baseline upgrades, USB4STREAM/backport use, RCCL/custom transport selection, model support, cache schema changes, and firmware rollout remain gated on those measurements.

There are no **[MEASURED]** performance or compatibility claims in this section.
