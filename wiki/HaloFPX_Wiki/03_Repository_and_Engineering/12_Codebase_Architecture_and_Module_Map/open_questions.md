---
section_id: "12"
title: "Codebase Architecture Open Questions"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
  software_versions: []
  hardware_revisions: []
related_sections: ["11", "13", "14", "15", "32", "39", "51", "56", "57"]
---

# Open questions

| ID | Question | Why unresolved | Required evidence / owner |
|---|---|---|---|
| OQ12-01 | Which exact ROCmFPX commit and upstream merge base become the HaloFPX baseline? | The four audited default branches are at different revisions. | Section 11 lineage graph and frozen tag; section 15 patch policy. |
| OQ12-02 | Which ROCmFPX types and operations are correct on HIP and Vulkan on both `gfx1151` nodes? | Source presence and repository claims are not machine proof. | M12-01 plus sections 13, 30, 32, 37, and 74. |
| OQ12-03 | Can upstream RPC be extended for two links and required failure semantics, or must it be wrapped/replaced? | Current registry/scheduler integration does not establish multipath or product recovery. | M12-04 and section 51 protocol/source audit. |
| OQ12-04 | What is the stable checkpoint compatibility fingerprint? | Public sequence-state APIs expose serialization but no project portability contract has been proven. | M12-03 and sections 56-57; exact source/version matrix. |
| OQ12-05 | Where should HaloKV hook into libllama/server to minimize upstream merge conflicts? | CachyLLama demonstrates a working seam but integrates deeply into common/server paths. | Section 14 patch inventory, section 56 port map, prototype diff. |
| OQ12-06 | Which component owns distributed plan selection and scheduler translation? | ggml scheduler executes device graphs, but product modes require cost/topology/failure policy. | Sections 38-39 and 47 ADR. |
| OQ12-07 | What complete state must move/persist for recurrent, MTP, and speculative models? | Attention KV alone may omit recurrent, draft, sampler, or RNG state. | Sections 35-36 and 61, model-specific round trips. |
| OQ12-08 | Which server/runner features from `llama-ai` are adopted versus reimplemented? | The repository mixes useful operational policy with environment-specific assumptions and claims. | Section 14 feature inventory, section 16 packaging review, requirements trace. |
| OQ12-09 | What are the backend fallback rules when one ROCmFPX op/type is unsupported? | Silent CPU fallback can preserve correctness while invalidating placement/performance expectations. | Op-support trace and explicit plan-manifest schema. |
| OQ12-10 | How are cache corruption and partial multi-rank commits detected and recovered? | No HaloKV on-machine implementation has been tested. | Sections 57-65 and fault-injection experiments. |

## Internet follow-up

- **[OPEN]** Re-run commit and ancestry inspection when section 11 freezes a baseline; use exact merge bases rather than default-branch names.
- **[OPEN]** Track upstream changes to state serialization, backend scheduler, RPC protocol, HIP/Vulkan registries, server slot management, and speculative APIs before each rebase.
- **[OPEN]** Audit open upstream issues/PRs only after the baseline is selected; issue discussions are secondary to the pinned code and tests.

## Machine follow-up

- **[OPEN]** Execute M12-01 through M12-05 from [procedures and checks](procedures_and_checks.md#on-machine-validation).
- **[OPEN]** Record negative results and unsupported combinations; do not collapse them into a single "works on Strix Halo" statement.

## Decision gate

**[RECOMMENDATION]** Do not approve the distributed/runtime/cache integration ADR until OQ12-01, OQ12-03, OQ12-04, OQ12-05, and OQ12-06 have owners and evidence plans. Backend defaults additionally require OQ12-02 and OQ12-09.

