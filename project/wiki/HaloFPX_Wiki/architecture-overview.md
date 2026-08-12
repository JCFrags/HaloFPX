# HaloFPX Architecture Overview

Status: routing summary

Last verified: 2026-07-29

This page routes workers to source-backed architecture material.
This page does not approve an implementation design.

## Product boundary

HaloFPX targets one maintainable llama.cpp-derived runtime.
The runtime must preserve selected ROCmFPX behavior for AMD Strix Halo.
The runtime may add compatible cache and lifecycle behavior after provenance review.

The project has two ordered phases:

1. Build a stable, single-node-capable integration fork.
2. Optimize the accepted fork for two-node inference.

Read the exact [project goal](../../PROJECT_GOAL.md) before architecture work.

## Main components

| Component | Purpose | Authority |
|---|---|---|
| Integration fork | Own the accepted implementation and source lineage | [Repository and Engineering](03_Repository_and_Engineering/README.md) |
| Model runtime | Load models and execute inference | [Models, Quantization, and Inference](06_Models_Quantization_and_Inference/README.md) |
| Distributed runtime | Define rank, scheduler, ownership, failure, and fallback behavior | [Distributed Runtime](07_Distributed_Runtime/README.md) |
| Fabric | Carry authenticated work across the two nodes | [Fabric and Transport](08_Fabric_and_Transport/README.md) |
| HaloKV | Persist compatible prompt and inference state | [HaloKV Persistent Cache](09_HaloKV_Persistent_Cache/README.md) |
| Server and operations | Expose, deploy, observe, and recover the service | [Product, Server, and Operations](10_Product_Server_and_Operations/README.md) |
| Verification | Establish correctness, performance, and release evidence | [Verification and Performance](11_Verification_and_Performance/README.md) |

## Required behavior

- New behavior must remain default-off until its gates pass.
- Feature-off behavior must remain unchanged.
- Cache corruption must cause a miss or recomputation.
- Distributed designs must state rank ownership.
- Distributed designs must state failure and recovery behavior.
- Single-node operation must remain available for development or recovery.
- Performance claims must use matched accepted comparisons.

## Authority flow

1. Preserve source and machine evidence.
2. Create source-backed Wiki claims.
3. Record accepted choices in decision records.
4. Implement only the approved boundary.
5. Retain exact validation evidence.

Use the [evidence map](evidence-map.md) and [decision map](decision-map.md) for each step.
