# L05o canonical encoders and static qualification review v01

- Date: 2026-07-18
- Scope: target-native canonical ROOT, HEAD, PREPARE, CLOSE, ABORT, and
  QUARANTINE encoders plus Windows and Linux static-isolation qualification
- Final verdict: **ACCEPT**

## Historical boundary

This is an additive qualification record. It does not rewrite the accepted
L05o wire-validator v02 review or its repeat receipt, which accurately describe
the earlier three-test validator milestone. The new machine-readable receipt is
`evidence/l05o-canonical-encoder-static-qualification-receipt.json`.

## Review findings and repairs

The encoder review required separate kind-specific values and witnesses,
complete CLOSE-chain evidence, distinct ABORT mismatch and recovery branches,
canonical absence handling, bounded output/alias rejection, move-only secret
custody, and removal of namespace-level construction surfaces. Static review
required whole-target closure rather than source-text inference, fail-closed raw
link-item handling, archive/member symbol audits, and negative compile/link
probes.

Two later adversarial findings were also repaired before promotion:

1. The accepted ABORT fixture keeps HEAD byte-exactly at the predecessor. It is
   therefore `recovered_not_applied` class 1, not operation-time predecessor
   mismatch class 0. The corrected content digest is
   `0880c4e52c10a7d9da305199f591d0b228e5e7e6c7c86d4374b596966f4717ef`.
2. Typed PREPARE input incorrectly required the digest of the envelope that had
   not yet been encoded. PREPARE input now contains only transition/body data;
   private admission derives and verifies the full authenticated-envelope
   digest, successful encoding returns it, and CLOSE/ABORT bind the returned
   digest to exact authenticated PREPARE bytes. The public raw verifier now
   rejects a wrong expected PREPARE digest. There is no externally linkable
   unchecked verifier.

The Linux ELF audit is gated by exact `CMAKE_SYSTEM_NAME STREQUAL "Linux"`, so
non-MSVC platforms such as macOS do not inherit an ELF/readelf configuration
requirement.

## Wiki and authority review

The delivered seam matches ADR-0018's frozen wire and ADR-0019's canonical
encoder contract. Each kind has a distinct caller value, private move-only
witness, and public encode function. Authentication, semantic validation,
exact lifecycle evidence, private scratch encoding, self-verification,
non-overlap checks, caller-output publication, and key/tag/scratch wiping occur
before a result can report `authenticated_semantic_only`.

That status remains deliberately non-authoritative. No result is convertible
to a Linux observation, synthetic bootstrap, protected material, anchor,
provider admission, cache hit, restore, or inference authority. The target is
`STATIC EXCLUDE_FROM_ALL`, absent from the product graph, and reachable only by
explicit qualification targets/tests.

## Verification

| Gate | Result |
|---|---|
| Windows Release focused registry-lab tests | Pass, 5/5 |
| Windows full configured CTests | Pass, 82/82 |
| Windows HaloFPX-labeled CTests | Pass, 38/38 |
| Independent Python oracle | Pass, 8 fixtures and 3,260 hostile mutations |
| Windows repeated registry-lab executions | Pass, 1,000/1,000; 200 per test |
| Windows archive/member and link-graph audit | Pass |
| Windows positive and negative compile/link probes | Pass |
| nimo-2 Linux Release focused registry-lab tests | Pass, 4/4 |
| Linux `ar`/`nm`/`readelf` archive/member audit | Pass, six archives and six objects |
| Independent adversarial rereview | ACCEPT |
| Immutable reference repositories | Clean at locked commits and trees, 4/4 |
| Configured implementation remote | None |

The exact commands, toolchain tuple, source/executable/archive/object hashes,
raw-evidence locations, and reference-clone identities are retained in the
machine-readable receipt. The Linux build explicitly used
`-DLLAMA_BUILD_WEBUI=OFF`; it performed no provider or persistent-cache action.

## Node rollback-control verification

nimo-1 coordinator and nimo-2 worker were stopped in dependency order for the
isolated Linux qualification, then restored worker-first. Both services are
active and enabled, their unit/wrapper/executable hashes match preflight, ports
50052 and 8081 are listening, health returned HTTP 200, both 65,536-token slots
were idle, and a real chat request completed with exact content
`HALOFPX_ROLLBACK_OK`. The observed 18.376 tokens/s is retained only as an
operational smoke signal, not a matched performance baseline.

## Provenance and promotion boundary

No donor implementation was used. No CachyLLama unit was admitted, the direct
cherry-pick roster remains empty, and no GPL llama-ai implementation or
separately licensed documentation entered the MIT engine. The four reference
clones remained unchanged.

Promotion proves deterministic target-owned canonical encoding and static
isolation only. It does not prove a transaction engine, record-decoding seam,
filesystem behavior, crash durability, persistence, cache reuse, runtime
compatibility, or performance non-regression. Those gates remain closed.
