---
type: implementation-milestone-review
status: accept
date: 2026-07-17
lane: L03a
parent_commit: ed2e15cce28ea2e945ee1a17bd1394397c583c7e
base_commit: 61f2f2d7bc4955e9bca821095ef69125837133b5
base_tree: 0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd
---

# L03a inert-provider-seam review

## Verdict

**Accept as the first L03 implementation milestone.** The target-owned internal
provider interface and disabled implementation are buildable and tested but
cannot enter a server runtime path or perform I/O. This permits later separately
gated interface work; it does not admit a codec, reader, writer, matcher,
current-cache adapter, or persistent provider.

## Architecture and provenance

Read-only source archaeology traced `common_params`, HTTP auth, task/slot state,
target/draft/speculative ownership, and inherited `server_prompt_cache`. The
least-coupled seam is the internal `tools/server` static library. Public
`llama.h`, core state serialization, common CLI/configuration, request/task/
slot paths, and model/runtime behavior are unchanged.

The inherited RAM/optional SSD prompt cache is not wrapped or presented as
HaloFPX v1. Current HTTP API-key validation does not propagate a principal or
security domain into tasks, so runtime lookup remains structurally forbidden.

The implementation is target-native. No CachyLLama or GPL llama-ai source or
documentation was copied, no donor unit was promoted to P3, and no attribution,
notice, SBOM, or distribution consequence was introduced.

## Independent adversarial review

The first read-only review returned `REVISE` on two seam-contract issues:

1. borrowed request/generation lifetime and exception-to-status behavior were
   not explicit; and
2. the no-runtime-hook test scanned only five files and could falsely pass.

The header and design record now require synchronous borrowing, no retention or
mutation, result-owned immutable candidates, bounded identity-reference
lifetime, and internal exception mapping across `noexcept`. The contract test
now recursively scans all production `common` and `tools/server` C++ headers
and sources, excluding only the seam itself. Re-review returned `ACCEPT` with no
remaining correctness, lifetime, concurrency, default-off, provenance, or test
false-pass blocker.

## Verification

| Check | Result |
|---|---|
| Clean Windows CPU Release build (`build/halofpx-l03a-clean`) | Pass, all configured targets |
| Clean HaloFPX CTests | Pass, 4/4 |
| Clean focused inherited CTests | Pass, 7/7 including fixture dependency |
| Disabled-provider unit test | Pass; assertions active in Release |
| Static no-I/O/no-hook contract | Pass across complete production source set |
| Clean baseline/current `llama-server --help` | Byte-identical normalized capture, 55,008 characters |
| Normalized help SHA-256 | Both `77083e868d047e7e6c46f58da8d5f5fcd54d0b7a54415eaf3eebffcdf11ac1b9` |
| `git diff --check` | Pass; autocrlf notices only |

The clean local build does not claim HIP/Vulkan/ROCm, target-node, persistence,
cryptography, state-continuation, or performance qualification. No provider is
instantiated, so there is no feature-off request-path or startup allocation.

## Remaining gates and reusable improvement

- A process-local current-cache adapter must remain explicitly ephemeral and
  cannot turn inherited disk state into a HaloFPX hit.
- The baseline codec is deferred to its own milestone with required-state
  inventory and exact cold-versus-restored continuation evidence.
- Any runtime owner/hook waits for authenticated scope propagation and complete
  compatibility identity.
- Offline parsing and writing remain L04/L05 and cannot enter the server.

The reusable improvement is an executable negative architecture contract: the
build can contain an inert seam while tests fail if later work silently adds
I/O, donor/state dependencies, or any production runtime hook.
