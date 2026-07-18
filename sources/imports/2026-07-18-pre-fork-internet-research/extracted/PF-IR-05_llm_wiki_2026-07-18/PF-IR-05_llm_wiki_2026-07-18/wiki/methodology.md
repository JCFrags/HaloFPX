# Methodology and claim discipline

## Scope

The target range is approximately 200–230 GB **as stored for the selected quantization**. Decimal GB display values are not converted into exact byte claims. Exactness requires a complete ordered shard set with pointer-reported byte counts and SHA-256 values at one immutable artifact revision.

## Catalog handling

No imported candidate catalog was present in the mounted workspace. This package records that absence in `catalog/import_status.json` and reconstructs the baseline from publisher repositories. It does not silently invent catalog provenance.

## Four evidence planes

1. Publisher facts: architecture, parameters, context, tokenizer/template behavior, tool behavior, and license from a publisher-controlled immutable revision.
2. Conversion facts: selected GGUF repository, artifact revision, shard pointers, quantization label, and converter calibration statements.
3. Static runtime source: exact llama.cpp and ROCmFPX revisions, architecture registration, graph construction, and operator-source presence.
4. Machine validation: target-host downloads, hashes, loads, traces, outputs, performance, and quality. This plane is `NOT_RUN`.

## Immutable identity

Every repository reference is a full 40-hex commit. Refresh scripts reject branches and tags as identity inputs. Missing exact fields use the literal sentinel `UNAVAILABLE`; they are never estimated from rounded UI values.

## Source preservation

`raw/` contains provenance-labelled normalized extracts. A file is only described as exact when `capture_type=exact_small_file`. The source registry and source-to-claim CSV provide claim lineage. Access date: 2026-07-18.
