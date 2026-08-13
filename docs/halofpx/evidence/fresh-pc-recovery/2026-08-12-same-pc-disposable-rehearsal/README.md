# HaloFPX same-PC disposable recovery rehearsal — 2026-08-12

Tracking: [issue #2](https://github.com/JCFrags/HaloFPX/issues/2) remains
**OPEN**. The bounded result was posted in
[issue comment 5277106922](https://github.com/JCFrags/HaloFPX/issues/2#issuecomment-5277106922).

## Claim boundary

**[MEASURED]** The retained receipts report a full GitHub-only continuation
rehearsal in a new disposable directory on the existing Windows 11 + WSL2
control PC. The source checkout was exact draft PR #56 commit
`bdd87e2e29d56d1e9d7fc302c08d7f6c170fa519`, and the continuation registry was
`docs/publication/continuation-releases.json` with SHA-256
`93d4874d8a836554c3da94442dff08b33ebb175a2ce4c16a8437546a7e015a96`.

This evidence is **not** a never-used-PC recovery result and does not satisfy
issue #2. No Strix Halo target or SSH access occurred. It establishes no HIP,
Vulkan, model-quality, performance, single-node, or dual-node result. It also
does not promote the recovery runner itself to a complete trust authority; the
receipt records the broader manually supervised rehearsal and its individual
checks.

## Tracked artifacts

Only the five small terminal artifacts are copied into Git. Their tracked bytes
match the retained source files exactly:

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| [`receipt__open-v01.json`](2026-08-12__halofpx-same-pc-full-recovery__receipt__open-v01.json) | 12,519 | `b9d5c1fd8494dfa977c7a92bd14c40d9f39bb90514ac50bc9366eba24a403c45` |
| [`receipt__open-v01.md`](2026-08-12__halofpx-same-pc-full-recovery__receipt__open-v01.md) | 13,670 | `a9e8a17e16d05175acd5b57faee6bf5cde6ee42af1aeb069d2cbd9c76094fdeb` |
| [`amendment01.json`](2026-08-12__halofpx-same-pc-full-recovery__receipt__open-v01-amendment01.json) | 925 | `43c75712fb5d2473f0da301cd0105f350456c9e1e54933346a7bbcc4f8d66276` |
| [`amendment01.md`](2026-08-12__halofpx-same-pc-full-recovery__receipt__open-v01-amendment01.md) | 872 | `a1a38af0584f63bab20dad83048e9657787fbe024d234b4170cc841ec2a8c106` |
| [`v02 SHA-256 ledger`](2026-08-12__halofpx-same-pc-full-recovery__sha256-ledger__v02.txt) | 10,040 | `8a4d8f72d1e36ceb77c068caab94c97bebe2577e12452c17d09ce70790ff7ca9` |

The amendment is normative for the feature-off generator description: that
lane used Unix Makefiles and GNU Make 4.4.1, not Ninja. The v02 ledger is the
authoritative continuation index. The earlier v01 ledger remains in the local
source root for historical scope and is deliberately not duplicated here.

## Local retention boundary

The full retained source root is:

```text
C:\Users\britt\Documents\HaloFPX_FullRecovery_Rehearsal_20260812
```

That path is workstation evidence, not a portable path or Git authority. At the
receipt's terminal pre-receipt snapshot, the retained tree contained 49,896
files totaling `50,107,213,417` bytes. The subsequent receipt, amendment, and
ledgers are small and remain beside that tree. No cleanup was performed during
the rehearsal.

The local root retains `raw/`, `downloads/`, `recovered/`, `build/`,
`workspace/`, `tmp/`, and `receipts/`. None of those large or scratch trees was
copied into Git. In particular, this tracked directory contains no release
payload, split reconstruction, recovered Git repository, fixture GGUF, build
output, virtual environment, command output, credential, or token.

The v02 ledger's `raw/...` and `receipts/...` paths are relative to that local
rehearsal root. Its 86 entries bind the retained command evidence and receipts,
but most entries cannot be re-read or reverified from a Git clone alone because
the referenced raw files remain local. The ledger is an integrity inventory,
not an embedded replacement for the retained evidence tree.

## Interpretation order

1. Read the Markdown receipt for the scoped result and retained failures.
2. Apply normative amendment 01 to the feature-off generator description.
3. Use the JSON receipt for machine-readable values.
4. Treat the v02 ledger as the authoritative integrity index for the retained
   local evidence.
5. Keep issue #2 open until the complete gate runs on a genuinely never-used
   PC under the current acceptance procedure.
