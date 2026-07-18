# L05g offline bootstrap-authority review v01

- Date: 2026-07-18
- Scope: memory-only authority snapshot, purpose-separated bootstrap planning,
  generation-one anchor synthesis, secret lifetime, and default-off isolation
- Final verdict: **ACCEPT**

## Independent review

The first adversarial pass returned REVISE for one secret-lifecycle defect. Two
rare encoder failure exits erased only part of the local derived material and
could leave the authority-binding temporary on the stack.

The encoder now erases derived keys, tags, and authority commitments on every
post-derivation error and success path. A static contract couples the reviewed
explicit cleanup structure to at least eleven binding wipes. The independent
re-review rebuilt the exact Release targets, passed all five focused anchor and
authority tests, and returned ACCEPT.

The review otherwise confirmed full 256-bit attempt binding, fixed scope and
key-purpose binding, generation one with null predecessor, private key-free
plans, failure non-disclosure, deterministic concurrent planning, accurate
replay/rollback limitations, and absence from server linkage.

## Verification

| Gate | Result |
|---|---|
| Clean Windows CPU/WebUI-off Release build including `llama-server` | Pass |
| Full HaloFPX-labeled CTests | Pass, 18/18 |
| Focused inherited CTests | Pass, 7/7 |
| Anchor process repetitions | Pass, 100/100 |
| Authority process repetitions | Pass, 100/100 |
| Independent adversarial review | ACCEPT after one revision |

The initial multi-target clean-build command named the CTest fixture
`test-download-model` as though it were an MSBuild target. CMake correctly
reported that no such project exists. The six actual inherited executable
targets were then built, and the seven-test inherited matrix including the
fixture passed. This was harness-command attribution, not a product failure.

## Promotion boundary

L05g admits only an offline `authorized_unexecuted` plan. Concrete secret
custody, registry/high-water storage, external bootstrap-token wire, manifest
authority, replay journal, conclusive absence, create-if-absent execution,
filesystem durability, server integration, and nodes remain closed.
