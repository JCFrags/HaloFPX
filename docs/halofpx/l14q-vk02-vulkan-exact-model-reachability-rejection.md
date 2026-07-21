# L14Q-VK-02 Vulkan exact-model reachability rejection

Status: **two-node exact-model Vulkan route rejected; VK-01 remains default-off**

VK-02 was the final zero-source-change go/no-go gate for the existing VK-01
Q8_0 KV pre-dequant experiment. Its purpose was to prove that the pinned 160 GB
primary artifact could load, reach the eligible Vulkan path, and justify a
short matched performance screen. The two-node Vulkan/RPC runtime instead lost
the remote Vulkan device during the server's startup decode probe. The worker
and coordinator both failed closed before an HTTP listener or inference result
became available. No performance trial or rescue run was attempted.

## Authority and unchanged implementation

The source authority is HaloFPX commit
`7f0e91c2d89e4081d555ce00f6bcf29353876c0e`, tree
`730bdd2ee955df8146831a8e96d5aa7a8e93e07c`. Its Git archive SHA-256 was
`3a82588b1a610a259455064f2095315eb4645b0b809e0143254fd458b6ca0bce`.
VK-02 changes no source. VK-01 remains the target-native, P3-governed,
compile-time experiment `GGML_VULKAN_FA_Q8_0_PREDEQUANT`, default `OFF`.

Fresh matched OFF and ON Release builds were produced on nimo-2 with gfx1151,
HIP, Vulkan, RPC, forced MMQ, no VMM, tests enabled, and WebUI disabled. The
feature-off, L02, and VK-01 selector contracts passed 3/3 in both builds. The ON
runtime bundle was copied byte-for-byte to nimo-1 for representative two-node
qualification; its mode-0600 transfer archive SHA-256 was
`ae9f40e809f8780050fb986e50be73165688ca9157be01300e9d6f5653bfdaa8`.

| Variant | Vulkan library SHA-256 | RPC server SHA-256 | Server SHA-256 |
| --- | --- | --- | --- |
| OFF | `8a321cb9188577113b85067eb8bc48e43ac344ad3c6c2c4847c41f73d111a49b` | `c449d1a8f787462f2766b465b8d82fc535a9f21e7e07f2d1a8bfa60e92507885` | `e780e9bc62acd37f872885fba2df6ee43162ed631f422e3b750f0078cce092fe` |
| ON | `39f2cd062e5b0aa3d8f1bf1cade3fef24df5ee146d33287075ef4ce62263ea66` | `cdadfe79c4519e55c35661060b0b44bea380e715c47de5b2374966c4da92afb4` | `dde5437b0befdd43bdc8f6737e455ae7ea63e3863d7842692bcba9a2cd74ae97` |

## Exact workload and failure

The exact artifact remained repository
`rcmorano/saricles-MiniMax-M2.7-REAP-172B-A10B-ROCMFPX`, revision
`dba517197f2854f3d362529e13abddcdcad6c10b`, file
`saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`, size
`159873097824` bytes, previously verified SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.
VK-02 reused that lock and did not reread 160 GB merely to repeat the hash.

The ON worker ran Vulkan0 on nimo-1 at `10.44.0.1:50053`; the nimo-2
coordinator used `RPC0,Vulkan0`, layer split `1,1`, Q8_0 K/V, context 4096,
batch/ubatch 512, FlashAttention, direct I/O, WebUI off, and no RPC cache. Load
planning assigned approximately 77,200.46 MiB to RPC Vulkan and 83,235.56 MiB
to local Vulkan. After about nine minutes, during
`common_context_can_seq_rm()`'s startup decode probe, the worker threw
`vk::DeviceLostError` from `vk::Queue::submit` while servicing a Vulkan buffer
read for RPC `get_tensor`. The coordinator then rejected the missing/malformed
RPC response at `ggml-rpc.cpp:491` and aborted.

No `q8_predequant` admission/debug record appeared before failure. Therefore
the VK-01 candidate route was **not proven reached**, and the device loss must
not be attributed specifically to VK-01. It proves that this current two-node
Vulkan/RPC exact-model tuple is not a safe or benchmarkable product route.
Running OFF or repeating ON could not promote VK-01 under the predeclared
kill-fast rule, so both were correctly omitted.

## Decision and rollback

VK-01 remains compile-time default-off and performance promotion is closed for
the current release lane. No new HIP/Vulkan port is justified: H03's exact
primary-model generation point estimate was only `+0.05164%` with an interval
crossing zero, whereas P08 identified the much larger serialized-rank
bottleneck. L14Q can be reconsidered only after an independent Vulkan/RPC
reachability repair or a materially different workload/backend hypothesis; it
must not block the fused expert-partial performance lane.

The failed coordinator was already stopped. The disposable worker was stopped,
then the known-good nimo-2 RPC worker was restored before the nimo-1
coordinator. Both production services are active with zero restarts; nimo-2
listens on 50052, nimo-1 reports `{"status":"ok"}` on 8081, and the deployed
server retains SHA-256
`d62ab220a4743a347461c958ce99a701e7ed21a938d9ab033334d9fb77fabbdb`.

Raw configuration, builds, tests, journals, core metadata, kernel windows,
model stat, hashes, and rollback evidence are preserved in verified mode-0600
bundles on their originating nodes. No source, model, reference clone,
dependency, persistence behavior, WebUI behavior, deployment binary, or Git
remote changed.

