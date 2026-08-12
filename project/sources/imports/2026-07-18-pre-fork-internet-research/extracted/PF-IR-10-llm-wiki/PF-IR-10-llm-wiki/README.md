# PF-IR-10 — Cross-fork semantic conformance assets

> **Status:** `PROPOSAL` · `QUALIFICATION-REQUIRED` · candidate execution: **none**  
> **Prepared:** 2026-07-18  
> **License of self-generated fixtures and tooling:** CC0-1.0

This is a hash-locked, legally conservative corpus proposal for unchanged differential use against exact llama.cpp, ROCmFPX, CachyLLama, and a still-unresolved HaloFPX candidate. It contains deterministic tiny GGUFs, tokenizer/chat/API/state fixtures, comparator profiles, upstream provenance and license evidence, explicit exclusions, and an applicability proposal.

## Start here

1. Open [`wiki/Home.md`](wiki/Home.md), or the offline [`wiki/index.html`](wiki/index.html).
2. Verify [`MANIFEST.sha256`](MANIFEST.sha256) after it is generated during packaging.
3. Review [`manifests/candidates.json`](manifests/candidates.json), [`manifests/accepted-assets.proposed.json`](manifests/accepted-assets.proposed.json), and [`manifests/applicability.csv`](manifests/applicability.csv).
4. Confirm [`qualification/EXECUTION-STATUS.json`](qualification/EXECUTION-STATUS.json): no candidate was executed.
5. Follow [`recipes/qualification-runbook.md`](recipes/qualification-runbook.md) in a separate isolated workspace.

## Decision boundary

The folder is an external provenance/license input and candidate roster for OPEN-TEST-01 and OPEN-API-01. It is **not** an approved executable test manifest. Local source-derived applicability, static review, isolated execution, and human approval of exact accepted hashes remain mandatory.

## Key dispositions

- Accepted legal payload: deterministic CC0 files plus MIT license/provenance evidence.
- Excluded: upstream vocab/model binaries whose exact redistribution or conversion chain was not sufficiently established.
- Exact output, numerical, metamorphic, and expected-rejection oracles are separated.
- Opaque state bytes are not a cross-fork oracle; continuation and logits are.
- Model-free n-gram speculation has a fixture scenario. Recurrent and MTP artifacts remain `open` with qualification recipes.
- `HaloFPX` remains `UNVERIFIED-IDENTITY`; no substitute project is inferred.

## Top-level layout

| Path | Purpose |
|---|---|
| `fixtures/` | Immutable generated inputs and expected records |
| `comparators/` | Candidate-independent normalization/comparison tools |
| `manifests/` | Fixtures, candidates, applicability, licenses, exclusions |
| `evidence/` | Primary license text, commit records, literal source-range captures, source locators, research evidence |
| `recipes/` | Deterministic generation, verification, and isolated qualification recipes |
| `qualification/` | Self-checks and explicit non-execution status |
| `adapters/` | Inert `UNEXECUTED-EVIDENCE` capture skeletons |
| `wiki/` | Human-readable LLM Wiki |
