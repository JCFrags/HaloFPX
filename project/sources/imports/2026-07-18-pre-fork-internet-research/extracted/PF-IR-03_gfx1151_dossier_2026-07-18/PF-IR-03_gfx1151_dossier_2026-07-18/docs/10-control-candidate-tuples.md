# Control and candidate tuples

## Control: PF-IR-03-CONTROL-7.2.4

Purpose: a reproducible legacy comparison lane.

- Ubuntu 24.04.3, retaining AMD's preliminary-installer caveat.
- gfx1151 as listed by the 7.2 Ryzen matrix.
- PyTorch 2.9 / ROCm 7.2 / Python 3.12 when framework comparison is in scope; FP16 is the officially validated data type.
- Exact ROCm 7.2.4 package closure from the legacy repository.
- Exact kernel package/config with the gfx1151 KFD fixes.
- Exact `linux-firmware` package and used blob hashes.
- No `amdrocm-*` or `/opt/rocm/core` files.

Machine record: [control-tuple.yaml](../manifests/control-tuple.yaml).

## Candidate: PF-IR-03-CANDIDATE-7.14.0

Purpose: test the current documented Core SDK gfx1151 lane without promoting it to the control baseline.

Choose exactly one:

1. Ubuntu 26.04 + GA kernel 7.0 + inbox driver; or
2. Ubuntu 24.04.4 + HWE kernel 6.17 + inbox driver.

Use stable native packages with GPG-verified metadata where signed-artifact provenance is required. Freeze `amdrocm-core-sdk7.14-gfx1151` and its complete dependency closure. The raw tarball is a separate delivery sublane and remains blocked until its expected digest/signature is authenticated.

Machine record: [candidate-tuple.yaml](../manifests/candidate-tuple.yaml).

## Local comparison: PF-IR-03-LOCAL-2026-07-17

The supplied ROCm 7.2.4 + Mesa 26.1.4 + kernel 7.1.3 tuple is retained exactly as a comparison point. It is not substituted into either official tuple. Machine record: [local-comparison.yaml](../manifests/local-comparison.yaml).

## Qualification matrix

| Test family | Control | 7.14 candidate | USB4STREAM overlay |
|---|---:|---:|---:|
| package/source provenance | required | required | inherit frozen userspace + kernel provenance |
| HIP compile/dispatch | required | required | regression required |
| HSA/KFD queue/CWSR stress | required | required | regression required |
| rocBLAS/hipBLASLt correctness | required | required | regression required |
| rocWMMA compile/correctness | required | required | regression required |
| RCCL single-node | when applicable | required | regression required |
| RCCL two-node | local | local | local with USB4STREAM isolated from RCCL transport claims |
| graphics/RADV | separately pinned | separately pinned | separately pinned |
| USB4STREAM | not baseline | not baseline | primary feature test |
