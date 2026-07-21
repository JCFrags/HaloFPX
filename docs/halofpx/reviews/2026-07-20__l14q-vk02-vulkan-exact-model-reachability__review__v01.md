# L14Q-VK-02 Vulkan exact-model reachability review v01

Verdict: **accept**. No P0, P1, or P2 finding remains, and no rescue run is
justified.

The review independently verified source commit/tree and archive identity, the
default-off VK-01 seam, its approved Vulkan runtime P3 boundary, both matched
OFF/ON configurations, all recorded binary hashes, and 3/3 focused contracts
in both builds.

Both node manifests verify. Bundle hashes, sizes, and mode `0600` match the
receipt. The exact model path, size, revision, and previously verified artifact
lock also agree with prior milestones.

The worker journal and kernel window prove a RADV GPUVM/device-loss failure;
the worker threw `vk::DeviceLostError` in Vulkan `get_tensor`, and the
coordinator then rejected the malformed/missing RPC response during
`common_context_can_seq_rm()` startup decode. No Q8 pre-dequant admission record
appears. The milestone therefore correctly states that VK-01 was not proven
reached and does not attribute the device loss specifically to that candidate.
The inability to reach an HTTP listener satisfies the predeclared kill-fast
rule, so omitting OFF and rescue performance trials is correct.

Rollback evidence shows the known-good worker and server active with zero
restarts, expected listeners, healthy endpoint, and original deployed server
hash. All five immutable references remain clean at their locked commits and
trees. No source, donor code, GPL implementation, dependency, model,
persistence, WebUI, deployment binary, or Git remote changed.

