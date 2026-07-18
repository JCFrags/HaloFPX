# Scope and method

## Exact source scope

| Label | Repository | Commit | Role | Source-selection proof | Deployed-binary proof |
|---|---|---|---|---|---|
| [VERIFIED] | charlie12345/ROCmFPX | `a5605a72768c6562241b248e268e33dc92787394` | baseline source snapshot | Commit object resolved | [OPEN] Not established |
| [VERIFIED] | charlie12345/ROCmFPX | `61f2f2d7bc4955e9bca821095ef69125837133b5` | candidate source snapshot | Commit object resolved | [OPEN] Not established |
| [VERIFIED] | fewtarius/CachyLlama | `6be745998f568e379ea197fcf827baec73ff9940` | CachyLlama source snapshot | Commit object resolved | [OPEN] Not established |
| [VERIFIED] | fewtarius/llama-ai | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | wrapper/submodule selection snapshot | Commit object resolved | [OPEN] Not established |
| [VERIFIED] | ggml-org/llama.cpp | `788e07dc91d266ad3162a1ce9037665656269689` | upstream comparison snapshot | Commit object resolved | [OPEN] Not established |
| [VERIFIED] | ggml-org/llama.cpp | `86d86ed4396b4130922f7b9af26e3d9fc11a591b` | upstream comparison snapshot | Commit object resolved | [OPEN] Not established |
| [VERIFIED] | charlie12345/rocmfp4-llama | `4860505ee322091f0f61eba77d6ad49be88cf4ea` | declared deployed predecessor source | Commit object resolved | [OPEN] Not established |

## Evidence taxonomy

| Label | Meaning |
|---|---|
| [VERIFIED] | Direct primary-source metadata, exact commit/ref/blob, or explicit project policy. |
| [MEASURED] | A source file, workflow, or symbol was inspected and a concrete property was observed. |
| [INFERENCE] | A conclusion follows from dated ancestry, source structure, or equivalent semantics but is not byte-level proof. |
| [ASSUMPTION] | A stated premise needed to evaluate reachability; it must not be mistaken for evidence. |
| [RECOMMENDATION] | A proposed control, backport, test, or release decision. |
| [OPEN] | Evidence is unavailable, contradictory, or explicitly left for local validation. |

## Method

[VERIFIED] The review enumerated the current official `ggml-org/llama.cpp` repository advisory list, the official project security policy, Cisco Talos GGUF reports, current cpp-httplib repository advisories, and official security pages for other bundled libraries where available.

[MEASURED] Exact source snapshots were resolved as commit objects. Material files were compared by Git blob ID where available and by security sentinel semantics where blobs diverged.

[MEASURED] Build inclusion was traced through CMake defaults, tool subdirectories, and the ROCmFPX/predecessor release workflow. Runtime reachability was assessed separately from compile-time inclusion.

[MEASURED] The review inspected GGUF/model loading, vocabulary loading, RPC deserialization and command handlers, HTTP middleware/defaults, slot/state file paths, prompt-cache storage, and bundled dependency versions.

[VERIFIED] No public exploit proof-of-concept was copied into the package. No exploit code was run. No live listener was contacted.

## Proof boundaries

[VERIFIED] Byte-identical blobs prove source-file identity only.

[VERIFIED] Semantic source equivalence proves that the reviewed source contains the material validation behavior; it does not prove compiler flags, dead-code elimination, patch contamination, or produced artifact identity.

[OPEN] Deployed executable hashes, build IDs, initialized submodules, compiler/linker inputs, loaded shared libraries, effective configuration, listener bindings, firewall policy, and negative reachability remain local evidence.

[OPEN] A repository security page can omit unpublished/private advisories. Public enumeration is therefore a lower bound, not proof of complete maintainer knowledge.

## Freshness

[VERIFIED] Access date is `2026-07-18`. Security advisories and project policies are time-sensitive and must be refreshed at each release candidate.
