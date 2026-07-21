# L11 RPC state-ownership blocker independent review

Date: 2026-07-21

Reviewer scope: read-only review of the documentation-only L11 milestone based
on HaloFPX commit `6862ffb99a8056552f62827658f3ffdcc79b9af4`, the current source tree,
the canonical Wiki, and the retained disposable two-host evidence bundle.

Verdict: **ACCEPT**

## Promotion finding

Stopping without a two-rank restore canary is required by the delegated gate.
The present RPC tensor-split implementation can produce a behaviorally complete
coordinator-side sequence blob for the measured plan only by transferring
worker-resident KV pages through `RPC_CMD_GET_TENSOR` during capture and
`RPC_CMD_SET_TENSOR` during restore. The worker has no `llama_context`, local
sequence serializer, immutable checkpoint publisher, attempt nonce, readiness
authority, validation-before-mutation step, or commit-live participant.

Wrapping this monolithic transfer in plan binding or agreement messages would
not create rank-local ownership and would violate ADR-0005's identifier-only
normal-restore control-plane contract. Implementing worker-local serialization
and coordinated commit would be a new distributed protocol, explicitly outside
this milestone. ADR-0039 therefore takes the conservative action required by
the delegation: no compile gate, runtime mode, persistent writer, or two-rank
restore canary is introduced, and ordinary distributed inference remains a
cold-recomputation path.

## Evidence reconciliation

- The working branch is based on commit
  `6862ffb99a8056552f62827658f3ffdcc79b9af4`; the implementation files cited by
  L11 are unmodified. Their recorded Git blob IDs match the current tree.
- `llama_context::state_seq_write_data()` and `state_seq_read_data()` delegate
  to the context memory object. Attention KV and recurrent memory use
  `io.write_tensor()` / `io.read_tensor()`. The file serializers lower these to
  `ggml_backend_tensor_get()` / `ggml_backend_tensor_set()`.
- The RPC backend maps those calls to `RPC_CMD_GET_TENSOR` responses and
  `RPC_CMD_SET_TENSOR` requests containing tensor payload bytes. This directly
  supports the ownership conclusion; it is not inferred from API success or
  generated text.
- The disposable attention-only fixture records a 124,876-byte sequence blob,
  four 10,368-byte remote GETs during save (41,472 bytes), and four 10,368-byte
  remote SETs during restore (41,472 bytes). The worker log contains those
  distinct state-sized transfers. Save and restore report 18 tokens.
- The uninterrupted and restart-restored first suffix are both token `10534`,
  text ` waiting`. This is appropriately treated as semantic confirmation for
  this exact fixture, not proof of rank locality or general topology reuse.
- Sampler ownership is accurately bounded: `server_slot::smpl` and
  `common_sampler` live on the coordinator and are outside sequence-memory
  serialization. The worker has no server sampler. Recurrent state follows the
  same placement-sensitive tensor I/O path but was absent from this fixture;
  draft, speculative, MTP, MoE, and other execution plans remain explicitly
  unresolved rather than extrapolated.
- The final evidence bundle is
  `/var/tmp/halofpx-rpc-state-ownership-evidence-20260721-v1.tar.zst`, SHA-256
  `b0d631ff52be389f581f908c5807b8c3cc6dac84180a81114e3c94b0357cd0ff`.
  Its retained contents include raw request results, the state blob, coordinator
  and worker logs, source snapshots and hashes, build configuration, binary and
  model hashes, service/cleanup records, and canonical authority fingerprints.
- The canonical plan fingerprint is
  `121cf2ee200f3a2397d306a62a7f8219537d6df17fbf3bc8c3b53974896f084f`;
  the topology fingerprint is
  `9baf437f2f3ca50fc727d57be522fa86d5ce61754f93bd005da2c49d9746e4e2`.
  Their canonical inputs bind the measured model/binaries, endpoint, split,
  placement parameters, node roles, logical ranks, interface, transport, and
  world size.

## Wiki and preservation review

The milestone gives the evidence requested by Wiki O58-03 for the exact
measured RPC layer-split tuple: current sequence serialization is monolithic
and globally complete for that tuple, not rank-local. It gives a deliberately
partial O58-02 answer for attention KV, recurrent byte routing, and sampler
scope; the remaining plan and state classes correctly stay open.

The retained records show both known-good production services active with
`NRestarts=0` and the disposable worker service inactive after cleanup. No
production configuration, primary 160 GB model, persistent-cache behavior, or
network topology was mutated. The four locked reference clones were also
independently observed clean at their recorded commit/tree pairs; the preserved
Nathan candidate clone was clean at
`a18067a85e986f7798f43d98345ed5b86b55cf88` / tree
`130e9cac828f8d8ef877d87ea9c192e24b07c9af`.

## Review correction and residual limits

The first review pass found that the exact plan and topology were described but
not fingerprinted as required. Canonical input strings and separate SHA-256
digests were added to the bundle and milestone records, the bundle was
recreated, and its final hash was reverified before acceptance.

No unresolved correctness, provenance, security, rollback, or scope finding
blocks this documentation-only milestone. This acceptance does not admit a
distributed restore implementation or authorize primary-model qualification.
