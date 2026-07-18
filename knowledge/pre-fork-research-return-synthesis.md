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

## Immediate sequence

1. Import and review PF-IR-04 when it arrives.
2. Complete non-executing Stage 1 review of PF-IR-10 candidate assets and draft
   the exact proposed asset manifest; do not execute them yet.
3. Resolve `OPEN-PIN-01` from the already-built ROCmFPX candidates and finalize
   the paired installed/source/build/DSO manifest for `OPEN-BASE-01`.
4. Apply PF-IR-04 to capability-level donor/license decisions and keep the
   direct cherry-pick roster empty until G1 is approved.
5. Freeze API/state/cache/security contracts before any donor implementation.
6. Run later, separately authorized experiments in isolation: RCCL socket
   baseline, patched USB4STREAM kernel, and large-model candidate preflights.

## Performance objective signal

**[MEASURED, USER-REPORTED]** The user observed approximately 28 initial
generation tokens/s for two-node ROCmFP4 MiniMax and approximately 17–18
tokens/s for the current two-node Q6 MiniMax. Treat ROCmFP4 as the incumbent for
speed comparisons. Quantization, model variant, prompt, context, cache state,
sampling, power and quality were not matched, so this observation cannot rank
overall quality or establish a controlled speedup.

