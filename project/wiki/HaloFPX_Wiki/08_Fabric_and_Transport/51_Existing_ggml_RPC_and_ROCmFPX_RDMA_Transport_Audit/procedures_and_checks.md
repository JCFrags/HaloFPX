---
section_id: "51"
title: "ggml RPC and RDMA Audit - Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ROCmFPX@a5605a7", "llama.cpp@788e07d"]
  software_versions: ["CMake", "libibverbs if present"]
  hardware_revisions: ["two target nodes"]
related_sections: ["49", "50", "53", "54", "55"]
---

# Procedures and checks

## Internet/source-code research completed

Pinned full commits, compared transport/CMake bytes, and audited handshake, commands, cache, graph reuse, and verbs calls. No execution result was inferred from source.

## FT-51-E0 - mandatory RPC security admission

Before starting any RPC listener, record both peers' executable and loaded-library hashes, exact source commits, compiler/toolchain and CMake options, `GGML_RPC` state, process account, arguments, listener address, routes, and firewall rules. Mechanically confirm the reviewed `serialize_tensor()` and `create_node()` guards corresponding to upstream `ba38f3b`. If provenance is incomplete, leave RPC disabled and port 50052 closed.

Any malformed-tensor behavior test must use a disposable sanitizer-enabled build and isolated test network, never a live inference service or public exploit/shell payload. Require rejection before backend graph execution with no crash, device operation, file write, or retained graph. Prove intended peer access and management/LAN rejection. Preserve the receipt; rebuilding/restarting both peers or changing source, toolchain, bind, route, firewall, or role invalidates it.

## FT-51-E1 - reproducible source identity

```bash
git -C ROCmFPX rev-parse HEAD
git -C llama.cpp rev-parse HEAD
sha256sum ROCmFPX/ggml/src/ggml-rpc/{transport.cpp,CMakeLists.txt}
sha256sum llama.cpp/ggml/src/ggml-rpc/{transport.cpp,CMakeLists.txt}
git diff --no-index llama.cpp/ggml/src/ggml-rpc ROCmFPX/ggml/src/ggml-rpc || true
```

Store output with clone URLs and dirty status.

## FT-51-E2 - build and capability inventory

Read-only except build directory creation; no root required.

```bash
cmake -S ROCmFPX -B build-rpc -DGGML_RPC=ON
cmake --build build-rpc --target rpc-server -j"$(nproc)"
ldd build-rpc/bin/rpc-server | grep -E 'ibverbs|rdma' || true
ibv_devices 2>&1 || true
ibv_devinfo 2>&1 || true
ulimit -l
```

Record whether CMake says RDMA enabled; do not equate successful compilation with an available USB4 RDMA transport.

## FT-51-E3 - baseline and fallback

Prerequisite: FT-51-E0 passes for both peers. Run pinned client/server over one `thunderbolt-net` interface with RDMA disabled/unavailable, then with any genuinely discovered verbs device. Capture handshake/version, selected carrier log, RPC command traces, latency by operation and payload, CPU, copies, and failure behavior. Confirm forced negotiation failure remains on TCP.

## FT-51-E4 - correctness/fault matrix

Use an isolated test build with sanitizers where supported. Test malformed size, response-size mismatch, receive completion larger/smaller than requested chunk, server exit during graph compute, stale graph UID, hash collision simulation/corrupted cache file, disconnect during transfer, and incompatible build tuple. Expected outcome is bounded structured failure, not overflow, hang, or process-wide abort.
