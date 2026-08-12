# Portable recurrent-model qualification recipe

**Claim labels:** `UNEXECUTED-EVIDENCE`, `QUALIFICATION-REQUIRED`, `OPEN-QUESTION`  
**Accepted payload status:** no recurrent GGUF is accepted in this archive.

The pinned upstream `tests/test-llama-archs.cpp` can synthesize architecture models and save them with:

```text
test-llama-archs --arch <architecture> --seed <integer> --out <directory>
```

The upstream source also registers `test-recurrent-state-rollback`. A candidate recurrent artifact should be produced in an isolated checkout of the exact candidate commit, then subjected to this sequence:

1. Build only the candidate's test generator and recurrent rollback target from reviewed source.
2. Generate a dense `qwen35` model with seed `1234` into an empty output directory.
3. Record the compiler, C++ standard library, architecture, command line, output filename, file SHA-256, and generator binary SHA-256.
4. Run the generated file through the candidate that produced it and through the independent GGUF structural verifier.
5. Run the rollback recipe from `fixtures/state/recurrent-scenarios.jsonl` against a clean and a dirty destination context.
6. Compare restored logits with `atol=1e-5`, `rtol=0`; do not compare opaque state bytes.
7. Obtain human approval before adding the exact output hash to `manifests/accepted-assets.proposed.json`.

## Why this is not already deterministic across toolchains

The pinned upstream generator seeds tensors through `std::hash<std::string>` and samples with `std::normal_distribution`. Those choices do not define portable byte output across C++ standard libraries. A locally generated file can be accepted only by exact hash after the toolchain and generator are qualified, or after the generator is replaced by an integer-defined implementation equivalent to `recipes/generate_assets.py`.

## Applicability proposal

- `llama.cpp`: `open` until a generated recurrent file is isolated and approved.
- `ROCmFPX`: `open` until local source-derived support and rollback behavior are reviewed.
- `CachyLLama`: `open` until local source-derived support and rollback behavior are reviewed.
- `HaloFPX`: `open` because the candidate identity is unresolved.
