# Forks and pinning

## Research snapshot

| Logical fork | Repository | Branch observed | Commit observed | Gate status |
|---|---|---|---|---|
| `upstream` | `ggml-org/llama.cpp` | `master` | `86d86ed4396b4130922f7b9af26e3d9fc11a591b` | Design reference; repin before execution |
| `rocmfpx` | `charlie12345/ROCmFPX` | `main` | `a5605a72768c6562241b248e268e33dc92787394` | Resolved public candidate |
| requested `cachyllama` | `llama-ai/CachyLlama` | — | — | Not resolved during research |
| provisional `cachyllama` | `fewtarius/CachyLLama` | `master` | `6be745998f568e379ea197fcf827baec73ff9940` | Candidate only; identity confirmation required |
| `integration` | supplied by program owner | — | — | Required before any release comparison |

The researched CachyLLama candidate describes itself as a fork of upstream with a parent project at `fewtarius/llama-ai`. That relationship is why it is used for suite design; it is not evidence that the requested repository path and the candidate are interchangeable.

## Pin file contract

Create one lock per run:

```json
{
  "fork": "integration",
  "repository": "OWNER/REPOSITORY",
  "commit": "40-hex commit",
  "dirty": false,
  "submodules": [{"path": "ggml", "commit": "40-hex commit"}],
  "binary_sha256": "64-hex digest",
  "build_id": "human-readable immutable lane ID"
}
```

Record submodules, generated sources, vendored dependencies, and dynamic backend libraries. A clean top-level commit with a modified submodule is dirty.

## Upstream alignment

For each fork, capture:

- merge base with the pinned upstream reference;
- commits ahead and behind;
- upstream test paths added, removed, renamed, or patched;
- public API/ABI changes;
- GGUF and state-format changes;
- server endpoint/field differences;
- backend and quant-type capability deltas.

Store this in the run artifact, not only in CI logs.

## Future integration fork policy

The integration fork should be tested at three points:

1. **pre-merge parents:** each feature fork against the same upstream base;
2. **integration candidate:** merged implementation against upstream and each feature fork;
3. **post-rebase candidate:** after every upstream sync that touches GGUF, tokenizer, sampling, state, server, speculative, RPC, or backend code.

Do not compare the integration fork against moving branch heads in the same report. Update all pins intentionally and start a new evidence set.
