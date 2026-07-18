---
type: research-intake-review
status: complete
created: 2026-07-18
scope: PF-IR-01 through PF-IR-11 excluding pending PF-IR-04
verdict: retain-as-candidate-evidence-and-route-to-local-gates
---

# Review of ten returned PF-IR research Wikis

## Verdict

**RETAIN AND ROUTE.** The ten packages answer their assigned external-research
questions well enough to inform the named local gates. They do not approve a
kernel, ROCm lane, security exposure, donor asset, cache format, model, RCCL
transport, firmware update, or XDNA2 role. Their scripts were not executed and
their internal manifests were not treated as independent proof of their own
claims.

## Decision routing

| ID | Candidate conclusion | Project disposition | Next owner/gate |
|---|---|---|---|
| PF-IR-01 | Candidate source appears to contain current published guards, but standard builds still ship RPC/server attack surface | Keep release security **HOLD**; current LAN/RPC deployment is a lab exception, not a release policy | Local binary/DSO/build-definition provenance, listener/firewall/auth audit; G0C/G2/security release gate |
| PF-IR-02 | USB4STREAM is a kernel-owned-buffer byte-stream API, not mmap/dma-buf/GPU-direct; v7.2-rc3 needs the accepted DMA-unmap correction | Admit only a reversible patched-kernel candidate; retain Linux 7.1.3 as rollback control | Freeze K3 source/package manifest, idle-node boot/rollback qualification, then transport tests |
| PF-IR-03 | ROCm 7.2.4 control and Core SDK/TheRock 7.14 candidate are distinct lanes; raw tarball lacks authenticated expected digest | No mixed-lane upgrade; native signed packages preferred | Paired installed tuple and DSO closure, then separate control/candidate builds; OPEN-BASE-01/G2 |
| PF-IR-05 | Five large-model candidates advance; only GLM-4.7 Q4_K_M and DeepSeek-R1 Q2_K_XS have complete selected-shard manifests in the package; MiniMax held for license/conversion review | Keep current MiniMax as a measured operational reference, not the automatic Phase 2 target | Human workload shortlist, exact artifact verification, CPU/backend/operator preflight, fit/quality gates |
| PF-IR-06 | Immutable self-describing objects, validate-before-publish, file sync plus directory sync, one writer/root | Promote as design constraints, not an implementation format | Record deployed filesystem/mount stack; finalize OPEN-FMT-01/OPEN-STORAGE-01; disposable crash/ENOSPC/EIO tests |
| PF-IR-07 | Object-level AEAD and tenant/principal binding are required for multi-user persistent cache; lower-layer encryption is defense in depth | Preserve fail-closed `MISS_RECOMPUTE` invariant; human authority choices remain open | Threat/scope ADR, principal/key ownership, sharing and backup policy before persistent multi-user writes |
| PF-IR-08 | RCCL belongs in the experiment matrix as the standard IP-socket collective baseline; stock Socket is not GPU-direct USB4 | Admit bounded two-host experiment only | Build exact 2.27.7/control and active candidate separately; interface attribution, correctness, abort/recreate and performance tests |
| PF-IR-09 | Public OEM BIOS provenance/rollback remains incomplete; AMD and storage watch entries do not close exact-board applicability | No firmware rollout | Complete paired local IDs, obtain signed OEM recovery/rollback evidence, then human-approved maintenance window |
| PF-IR-10 | External fixture provenance is adequate for local review; tiny GGUF remains qualification-required | Start non-executing Stage 1 review; do not run or promote yet | Exact asset manifest, local applicability review, isolated Stage 2 execution, human Stage 3 promotion; OPEN-TEST-01/G0C |
| PF-IR-11 | Keep XDNA2 excluded from primary architecture; one isolated classifier experiment is optional | No impact on fork critical path | Optional read-only probe and isolated auxiliary experiment only after primary milestones |

## Cross-package implications

1. **Security precedes convenience.** The active llama.cpp LAN service is useful
   operationally but does not satisfy the proposed release boundary because it
   is unauthenticated and RPC remains compiled and running. Preserve it as a
   controlled lab deployment; do not inherit that exposure as HaloFPX defaults.
2. **Do not combine upgrades.** USB4STREAM kernel qualification and ROCm 7.14
   qualification are separate reversible experiments against a frozen userspace
   or kernel control. A simultaneous kernel/ROCm change would destroy attribution.
3. **The first implementation work is still source and contract work.** The
   research narrows choices but does not remove G0C, licensing, baseline, storage,
   threat-model, or human-governance gates.
4. **ROCmFP4 is the current performance incumbent.** User testing reports about
   28 initial generation tokens/s for the two-node ROCmFP4 MiniMax deployment,
   versus about 17–18 tokens/s for the current Q6 deployment. These are useful
   objective signals but are not a matched quality/quantization benchmark.

## Quality notes

- All ten archives had safe paths and matching extracted file counts.
- Decision pages consistently preserve machine-validation boundaries.
- PF-IR-06 includes a generated `__pycache__`; PF-IR-07 contains rendered HTML
  duplicates; neither is promoted into canonical knowledge.
- PF-IR-07 and PF-IR-10 include implementation templates or adapters with
  placeholders. They remain inert candidate material.
- Full primary-source revalidation is still required when a claim becomes a
  build, release, firmware, security, or destructive-test decision.

## Missing package

PF-IR-04 is the only outstanding Internet-research package. It owns the external
donor/test-asset/release-artifact license and provenance evidence. Its return is
needed before G1 can close, but it does not block continued local source-lock,
Stage 1 test-asset inspection, or baseline characterization.

