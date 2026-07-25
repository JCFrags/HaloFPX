# L33 exact-primary live-state discriminator result

Date: 2026-07-24

Base: `601479a9cf7c18f81a37187663decece47f5fb05`

Outcome: **NOT PROMOTED — EXACT FIRST TOKEN MISMATCH**

## Bounded execution

[VERIFIED] The single controller-managed L33 transition used the exact
159,873,097,824-byte artifact with SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.
It retained the frozen 1,129-token prompt, 1,128-token boundary, one generated
token, Q8_0 K/V, flash attention on, context 4096, batch and ubatch 512,
seed 1234, temperature zero, explicit `RPC0,ROCm0`, layer split, tensor split
`1,1`, and all default-off diagnostics.

[MEASURED] Residency A used coordinator PID 1483859 and worker PID 2320013 /
InvocationID `a7bf992b624c46339daa77041293fbbd`. It captured object
`52316b626e06d38cf105408eb71354f35432a12aad0ec5220f76b796a2a66a60`
and reference token 21549. Residency B used coordinator PID 1484007 /
InvocationID `c5f1ad4d213d4c70ab72c00c2643cac2` and distinct worker PID 2320138 /
InvocationID `0d172a18447840d69be8a0f313eb3c9c`. It restored token 9283.
The token hashes are respectively
`7d44cce76babad0e2459d2295ac8704af317509458dd9ea8bbfe2e6cc151e051`
and `1d99e63f856e6c2a2903c515234d94ce863b30512beabe1a06e26cef831f9382`.
Exact correctness therefore failed at the first generated token.

## Four-phase authenticated interpretation

[MEASURED] Coordinator original capture, restore input, independent live
post-apply recapture, and adjacent pre-generation marker are byte-identical
under the diagnostic contract:

- control: 15,048 bytes,
  `2d614e8634f7f9defc4ed59f59b900490e021e382045c566109588bd288a0cbb`;
- coordinator-local state: 2,301,688 bytes,
  `7117319f7dc2b848d3ce3b35469aee3c62bc93a8262f88cb53949e7bbc5ceaca`;
- component manifest:
  `7ad364fb0a047ca8db745c439bf8ad3f6fe6b92ae56f7b9f632caa88e47c69b3`;
- token boundary: 1,128.

The authenticated coordinator report tag is
`119e59fbb5351651ca66e17847c5d83eedfec0af9b7a257c1120c77c824462db`;
the independent recapture object digest is
`871212c95de4318e6c2148e8e62490d043dce686a6cde12c0891588d990b8662`.
Thus original equals restore input, restore input equals live recapture, and
live recapture equals the adjacent pre-generation marker for every
coordinator field represented by the current contract.

[MEASURED] Worker capture, validated stage, live post-apply, and independent
recapture each contain 64 components and 2,454,528 bytes. Every phase has
content aggregate
`014a1024f13225a3f7bd7bba6be43dce1106a0354d68b5043f284263cce19bc9`;
authenticated identity/content comparison reports zero mismatches.
Capture, stage, and recapture share canonical Merkle root
`0fc1f297514f5b7a38f9db179e80df0bb9b5ac6ae3865858d518669ef8a79884`.
Apply has Merkle root
`aac062cc10c560807b80d06bcf49ea79deb563f89d6924b7d933d278a66fe429`
because the authenticated Merkle record includes live range topology.
This range-topology divergence is an observation; component identities and
content remain equal and L33 does not claim the topology is defective.

[VERIFIED] These equalities exclude evidence/object transport and represented
live application state under the current authenticated normalization. Combined
with the first-token mismatch, they establish only that the current
serializer/recapture contract omits or fails to represent some
primary-specific semantic state. L33 does not identify that state and makes no
semantic correction.

## Allocation, transport, recovery, and cleanup

[MEASURED] Both residencies allocated 80,950,550,528 worker material bytes.
Capture timing was 5,487.159 ms prompt, 118.850 ms state, and 74.671 ms
generation. Restore state and generation were 180.517 ms and 73.982 ms.
These values are diagnostic evidence, not performance claims.

[VERIFIED] Capture and restore state windows each contain zero legacy
`GET_TENSOR` and zero `SET_TENSOR` operations. The controller retained 620
bounded transport records and the child retained 233; neither stream contains
a timeout.

[VERIFIED] Production recovered worker-first. Nimo-2's exact system unit
`minimax-m27-rpc-worker.service` is active/running in
`/system.slice/minimax-m27-rpc-worker.service`, PID 1485678, listener 50052,
`NRestarts=0`. Nimo-1's exact system unit
`minimax-m27-q6-server.service` is active/running in
`/system.slice/minimax-m27-q6-server.service`, PID 2320476, serves the
standard UD-Q6 model on 8081 with HTTP 200, and has `NRestarts=0`.

[VERIFIED] All L33 transient units, port 50233, keys, state/evidence staging,
rendezvous, remote source archives/build roots, and the local source archive
are absent.

The immutable evidence root is
`C:\Users\britt\Documents\Custom_Inference_Project\sources\halofpx\l33-primary-20260724`.
Its immutable payload contains 35 files totaling 2,013,575 bytes and has
canonical
relative-path-plus-NUL-plus-content SHA-256
`94eb2fe872f5c457d8a56510428bd0f73578d7f8f6fb81dfa616f255503a7696`.
`evidence-tree-manifest.json` records this payload hash and excludes only
itself from the canonical scope.

## Boundary

L33 is terminal NOT PROMOTED and was not retried. It does not authorize a
semantic fix, another primary run, cache promotion, production cache
enablement, performance claims, or L34.
