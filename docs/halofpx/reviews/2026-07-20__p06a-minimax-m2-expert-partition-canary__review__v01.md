# P06a MiniMax-M2 expert-partition canary independent review

Status: **ACCEPT; no P1/P2 correction.**

The review confirmed that the build option defaults off, the static component
is excluded from the ordinary build, and only the explicitly gated test links
it. No `llama` runtime edge, generic tensor-mode admission, attention change,
or KV-cache placement change is present.

The expert-plan implementation clears output and fails closed on wrong world
size, out-of-range IDs, and duplicate IDs. The partial reduction rejects null,
zero-token, and over-bound requests. The focused nimo-1 Release build, 1/1
CTest, direct run, feature-off target-absence check, six source hashes, evidence
manifest, and 10,481-byte bundle were independently reconciled with the report
and receipt.

Claims are appropriately limited to the isolated ownership, remapping, mask,
and reduction contract. Q8_0_ROCMFPX kernel execution, local/remote overlap,
runtime expert placement, the pinned 160 GB model, and any performance claim
remain closed for P06b. The change is target-native and introduces no donor or
GPL implementation, dependency, remote, WebUI, persistent write, model change,
deployment, or reference-clone mutation.
