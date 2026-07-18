# Pre-fork research return synthesis

Use this module after the 2026-07-18 PF-IR intake and before selecting the next
fork-preparation task. The preserved packages are candidate evidence; the
[intake review](../reviews/intake/2026-07-18__pf-ir-returns__review__v01.md)
owns their current disposition.

## What the returns changed

- Security policy should ship RPC off by default and require authentication or
  a protected proxy for non-loopback HTTP. Current services remain lab evidence.
- USB4STREAM is not zero-copy or GPU-direct. The candidate lane is a patched,
  reversible Linux 7.2-rc3 experiment, separate from compute-stack upgrades.
- ROCm 7.2.x and Core SDK/TheRock 7.14 are different lanes. Never mix their
  runtime, compiler, device-library, or package roots inside one qualification.
- Persistent cache publication requires immutable objects, exact validation,
  sync-before-publication, directory sync, one writer per root, and corruption
  or wrong-domain state becoming miss/recompute.
- Multi-user persistence requires object-level authenticated encryption and
  explicit principal, sharing, rank, epoch, and key authority.
- RCCL is worth a two-host socket-baseline experiment but has no established
  stock GPU-direct USB4 path and no target-machine qualification.
- XDNA2 remains outside the primary inference architecture.

## Completed preparation sequence

1. All eleven PF-IR packages are preserved and reviewed.
2. PF-IR-10 Stage 1/static qualification accepted 52 exact hash-pinned
   references and deferred seven candidate-execution assets.
3. `OPEN-PIN-01` selected ROCmFPX `61f2f2d...` from the completed two-node
   qualification; `a5605a...` remains the research control.
4. PF-IR-04 fixed the MIT-core/GPL-separation implementation-start policy; the
   direct cherry-pick roster remains empty until per-capability P3 approval.
5. The initial feature-off API/security contract is frozen.

The project is ready to begin the local custom fork. Remote governance,
persistent-write, deployment, performance, and release gates remain deliberately
later work.

## Performance objective signal

**[MEASURED, USER-REPORTED]** The user observed approximately 28 initial
generation tokens/s for two-node ROCmFP4 MiniMax and approximately 17–18
tokens/s for the current two-node Q6 MiniMax. Treat ROCmFP4 as the incumbent for
speed comparisons. Quantization, model variant, prompt, context, cache state,
sampling, power and quality were not matched, so this observation cannot rank
overall quality or establish a controlled speedup.
