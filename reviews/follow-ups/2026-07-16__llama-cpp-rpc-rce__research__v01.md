---
type: research-follow-up
status: researched
target: llama.cpp and ROCmFPX RPC source / HaloFPX distributed topology
created: 2026-07-16
last_verified: 2026-07-17
risk: high
approval_required: human
---

# llama.cpp RPC RCE advisory mapping for HaloFPX

## Executive verdict

- [VERIFIED] GitHub advisory [GHSA-j8rj-fmpv-wcxw](https://github.com/ggml-org/llama.cpp/security/advisories/GHSA-j8rj-fmpv-wcxw), published 2026-03-26, describes an unauthenticated remote-code-execution path through `RPC_CMD_GRAPH_COMPUTE`. It lists llama.cpp versions through `b7991` as affected and, as of 2026-07-17, still lists no patched version.
- [VERIFIED] Upstream commit [`ba38f3becce7d1283585c73d796eb47d72bbbd30`](https://github.com/ggml-org/llama.cpp/commit/ba38f3becce7d1283585c73d796eb47d72bbbd30), merged through [PR #21030](https://github.com/ggml-org/llama.cpp/pull/21030) on 2026-03-27, blocks the advisory's documented `buffer == 0 && data != 0` tensor path. The first llama.cpp release tag containing that commit is `b8552`.
- [VERIFIED] The project's pinned llama.cpp commit `788e07dc91d266ad3162a1ce9037665656269689` contains that upstream commit and both relevant source guards.
- [VERIFIED] The pinned ROCmFPX commit `a5605a72768c6562241b248e268e33dc92787394` and retained runtime-source commit `4860505ee322091f0f61eba77d6ad49be88cf4ea` contain the same relevant guards. Their advisory-sensitive RPC source is source-equivalent to the fixed llama.cpp path, although the fork's independent history does not preserve upstream commit ancestry.
- [INFERENCE] The specific malformed-tensor path documented in GHSA-j8rj-fmpv-wcxw is therefore not source-applicable to those exact pins. This is not a claim that llama.cpp RPC is generally secure.
- [OPEN] No live executable, loaded library, listener, firewall, or process was inspected in this research pass. Deployment safety remains unproven until both endpoints' built-artifact provenance and network exposure are revalidated.
- [RECOMMENDATION] If either endpoint cannot prove that it was built from the reviewed fixed source, stop or disable its port-50052 listener before further use.

## Scope, method, and safety boundary

This review maps the advisory to exact upstream and fork source, then evaluates reachability against the intended dual-Strix-Halo topology recorded by the project. Primary sources only were used: the GitHub advisory, exact repository commits and source files, the merged upstream PR, and the project's own deployment decisions.

No exploit or proof-of-concept payload was run. This review does not declare the current runtime safe and does not approve production use of the RPC protocol.

## Advisory claim map

| Item | Evidence-backed finding |
|---|---|
| Advisory | [VERIFIED] GHSA-j8rj-fmpv-wcxw, critical severity 9.8, published 2026-03-26. |
| Upstream affected range | [VERIFIED] The advisory lists llama.cpp `<= b7991`. |
| Advisory patch metadata | [VERIFIED] The advisory still says “Patched versions: None” as of 2026-07-17. |
| Entry point | [VERIFIED] An unauthenticated client reaches `RPC_CMD_GRAPH_COMPUTE` over the RPC TCP service, commonly port 50052. |
| Documented call path | [VERIFIED] `rpc_server::graph_compute()` calls `create_node()`, which calls `deserialize_tensor()`. |
| Unsafe state | [VERIFIED] In vulnerable source, an unrecognized/zero serialized buffer can produce `tensor->buffer == nullptr`, bypass buffer-bound checks, while an attacker-controlled nonzero serialized `data` value is assigned to `tensor->data`. Backend graph execution can then use that pointer. |
| Security boundary | [VERIFIED] The protocol has no authentication requirement in the documented path. Network reachability to the listener is sufficient to send RPC requests. |

Primary advisory: [GHSA-j8rj-fmpv-wcxw](https://github.com/ggml-org/llama.cpp/security/advisories/GHSA-j8rj-fmpv-wcxw).

## Upstream source fix and advisory-metadata discrepancy

[VERIFIED] [PR #21030](https://github.com/ggml-org/llama.cpp/pull/21030), merged on 2026-03-27 as commit [`ba38f3becce7d1283585c73d796eb47d72bbbd30`](https://github.com/ggml-org/llama.cpp/commit/ba38f3becce7d1283585c73d796eb47d72bbbd30), made two changes directly relevant to the advisory:

1. `serialize_tensor()` now serializes `data = 0` when the tensor is not backed by an RPC buffer.
2. `create_node()` rejects a deserialized tensor when `buffer == nullptr && data != nullptr` before admitting it to the graph.

[VERIFIED] The first llama.cpp release tag containing the merge commit is `b8552`.

[INFERENCE] These changes block the exact malformed state described by the advisory. They are appropriately treated as a source-level remediation for that documented path.

[OPEN] The advisory itself has not been updated to identify this commit or a patched release. Therefore, `b8552` is a repository-derived lower bound for inclusion of the blocking source change, not an official patched-version declaration from the advisory. Consumers must validate the exact commit and source content rather than relying on an assumed version threshold, especially for forks.

[VERIFIED] The PR changed both client-side serialization and server-side validation. Both RPC peers must be rebuilt and restarted from reviewed source; updating only one side is not an adequate deployment procedure.

## Exact pinned-source mapping

| Source authority | Exact revision | Relevant result | Provenance note |
|---|---|---|---|
| llama.cpp | `788e07dc91d266ad3162a1ce9037665656269689` | [VERIFIED] Contains upstream commit `ba38f3b`; rejects null-buffer/non-null-data tensors before graph execution. | `ggml-rpc.cpp` Git blob `d38057721834c95f4beda160dd89dc7f0c5ca86a`; `rpc-server.cpp` blob `08e680391415f24e7e3c6c547aa4b517e535840a`. |
| ROCmFPX project pin | `a5605a72768c6562241b248e268e33dc92787394` | [VERIFIED] Contains both relevant serialization and server validation guards. | `ggml-rpc.cpp` blob `1cb8f563d8583db966c424b7f2fd58f65f8f9c7f`; `rpc-server.cpp` blob `08e680391415f24e7e3c6c547aa4b517e535840a`. |
| ROCmFPX retained runtime-source pin | `4860505ee322091f0f61eba77d6ad49be88cf4ea` | [VERIFIED] Relevant RPC source blobs are identical to the project-pin blobs above. | Recorded as the runtime commit in the AgentRoot project authority; built binary identity remains [OPEN]. |

Exact primary source:

- llama.cpp [`ggml-rpc.cpp` at `788e07d`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/src/ggml-rpc/ggml-rpc.cpp)
- llama.cpp [`rpc-server.cpp` at `788e07d`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/rpc/rpc-server.cpp)
- ROCmFPX [`ggml-rpc.cpp` at `a5605a7`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-rpc/ggml-rpc.cpp)

[VERIFIED] The ROCmFPX repository is an independent source snapshot: its Git history does not establish ancestry from upstream `ba38f3b`. Applicability was therefore established from exact source and blob comparison, not from a release number or upstream ancestry.

[VERIFIED] The full ROCmFPX-to-pinned-llama.cpp `ggml-rpc.cpp` diff is limited to 34 changed lines in graph ownership/context handling. The serialization, deserialization, null-buffer/non-null-data rejection, and graph-compute admission path relevant to this advisory are equivalent.

## Source-path trace at the reviewed pins

The fixed admission sequence is:

```text
RPC_CMD_GRAPH_COMPUTE
  -> rpc_server::graph_compute()
     -> create_node()
        -> deserialize_tensor()
        -> reject when buffer == nullptr and data != nullptr
     -> only accepted nodes enter backend graph computation
```

[VERIFIED] Bounds validation still depends on recognizing an RPC buffer, but the added `create_node()` condition prevents the advisory's zero/unknown-buffer plus nonzero-data combination from reaching backend graph computation.

[VERIFIED] `GRAPH_RECOMPUTE` operates on a graph already admitted and retained by the server. It does not deserialize a new attacker-supplied tensor graph and therefore does not reopen this exact admission path.

[OPEN] This review did not audit every other RPC command or every backend kernel for other memory-safety defects.

## Intended HaloFPX topology and reachability

The current intended topology is documented in:

- `C:\Users\britt\AgentRoot\10_projects\11_active\11.02_strix-halo-usb4-cluster\README.md:15`
- `C:\Users\britt\AgentRoot\10_projects\11_active\11.02_strix-halo-usb4-cluster\DECISIONS.md:882`
- `C:\Users\britt\AgentRoot\10_projects\11_active\11.02_strix-halo-usb4-cluster\DECISIONS.md:896`
- `C:\Users\britt\AgentRoot\10_projects\11_active\11.02_strix-halo-usb4-cluster\MANIFEST.md:8`

[VERIFIED] The intended split makes nimo-2 the model/API owner and nimo-1 the private RPC worker at `10.44.0.1:50052`, with one RPC connection carried over two USB4 rails through MPTCP. The planned client device order is `RPC0,ROCm0`, and the build explicitly enables `GGML_RPC=ON`.

[VERIFIED] Upstream defaults reduce accidental exposure but do not protect this intended deployment: `GGML_RPC` defaults to `OFF`, and `rpc-server` defaults to `127.0.0.1:50052`. The intended HaloFPX configuration deliberately enables RPC and binds a non-loopback private address. The exact [`rpc-server.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/rpc/rpc-server.cpp) warns that non-loopback operation is experimental and insecure. The upstream [RPC README](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/rpc/README.md) likewise describes RPC as fragile/insecure and says it should not be exposed to open or sensitive networks.

[INFERENCE] In the intended topology, an affected build would be reachable from the peer and from any other process or network path permitted to connect to `10.44.0.1:50052`. A private cable, private subnet, firewall, or MPTCP path narrows reachability but does not authenticate RPC messages or make its parser memory-safe.

[INFERENCE] The reviewed pins block the specific documented GHSA path, but a compromised peer or local process remains inside the RPC trust boundary. Transport isolation is defense in depth, not a substitute for source remediation.

[OPEN] Older internal decisions record the worker on the opposite USB4 endpoint (`10.44.0.2:50052`). Roles and addressing changed during development. The current listener, process arguments, routing, and firewall must be observed before deployment facts can be claimed.

## Immediate safe mitigations

1. [RECOMMENDATION] If exact executable and loaded-library provenance cannot prove the reviewed fixed source on both nodes, stop or disable the port-50052 RPC service.
2. [RECOMMENDATION] Keep `GGML_RPC=OFF` in every build that does not explicitly require distributed RPC.
3. [RECOMMENDATION] Rebuild and restart both client and server from exact reviewed commits. Do not replace loaded runtime libraries in place.
4. [RECOMMENDATION] For bounded lab use, bind only the exact USB4 worker address, never a wildcard or management/LAN address.
5. [RECOMMENDATION] Permit port 50052 only from the exact peer addresses needed for both MPTCP USB4 subflows; explicitly reject management and household-LAN ingress.
6. [RECOMMENDATION] Run the server as a dedicated unprivileged account with only required render/video device access and minimal filesystem access. Do not run it as root.
7. [RECOMMENDATION] Leave the RPC disk cache disabled unless a separately reviewed requirement justifies it.
8. [RECOMMENDATION] Capture process arguments, listener address, firewall rules, executable hash, loaded-library hashes, source commit, build options, and build-toolchain identity as deployment evidence.
9. [RECOMMENDATION] Treat unmodified llama.cpp RPC as lab-only. For any production-like trust boundary, use an authenticated, integrity-protected peer channel and a reviewed application protocol. WireGuard or mTLS can authenticate/encrypt transport but cannot make a vulnerable RPC parser safe.

## Remediation and revalidation gate

Before the RPC topology is declared usable, perform all of the following:

### Artifact provenance

- [RECOMMENDATION] Record exact source commits and Git blob IDs for `ggml-rpc.cpp` and `rpc-server.cpp` on both peers.
- [RECOMMENDATION] Record executable and loaded-library hashes, compiler/toolchain version, CMake configuration, and `GGML_RPC` state.
- [RECOMMENDATION] Mechanically assert that `serialize_tensor()` clears `data` for non-RPC buffers and that `create_node()` rejects `buffer == nullptr && data != nullptr`.

### Safe behavior test

- [RECOMMENDATION] In a disposable, isolated test network—not against a live service—send a synthetic `GRAPH_COMPUTE` request whose tensor encodes `buffer = 0` and a nonzero `data` value.
- [RECOMMENDATION] Require the server to reject/close the request before backend graph computation, with no crash, device operation, file write, or retained graph.
- [RECOMMENDATION] Add an ASan/UBSan lane where supported. Do not use public exploit code or an operational shell payload.

### Network and process proof

- [RECOMMENDATION] Prove the listener is bound only to the intended USB4 address and port.
- [RECOMMENDATION] Demonstrate allowed connections from the intended peer and rejected connections from management/LAN paths.
- [RECOMMENDATION] Demonstrate that both USB4 rails still form the intended MPTCP subflows after firewall tightening.
- [RECOMMENDATION] Prove the service account, device ACLs, filesystem permissions, and process arguments match the least-privilege plan.

### Revalidation triggers

Repeat this review and the behavior/network gates after any:

- change to llama.cpp/ROCmFPX RPC source or source pin;
- compiler, linker, dependency, or build-option change;
- worker/controller role, address, MPTCP, bind, or firewall change;
- new RPC security advisory or update to GHSA-j8rj-fmpv-wcxw;
- introduction of RPC caching, a new backend, or a new graph command path.

## Uncertainty and open work

- [OPEN] Why GHSA-j8rj-fmpv-wcxw still reports no patched version despite the source-level blocking change merged one day later has not been resolved by an upstream security-maintainer statement.
- [OPEN] The actual currently installed binaries and loaded libraries on nimo-1 and nimo-2 have not been hashed or mapped to source.
- [OPEN] Current listener, bind address, firewall, MPTCP routes/subflows, and process privilege have not been observed.
- [OPEN] This review establishes only that the documented malformed-tensor path is blocked at the reviewed pins. It does not establish general RPC memory safety, protocol authentication, or production suitability.
- [OPEN] A broader command-by-command RPC threat model and fuzzing campaign remains separate work.

## Decision status

[RECOMMENDATION] Accept this review as a high-risk research input, not as deployment approval. A human owner must decide whether the lab-only residual risk is acceptable after the artifact, behavior, network, and least-privilege gates above produce retained evidence.
