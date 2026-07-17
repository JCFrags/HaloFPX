---
section_id: "67"
title: "Configuration Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp", "charlie12345/ROCmFPX", "fewtarius/CachyLLama"]
  software_versions: ["commits in sources.md"]
  hardware_revisions: ["two-node Strix Halo target"]
related_sections: ["18", "23", "29", "30", "38", "47", "49", "60", "68"]
---

# Facts and constraints

## Verified source facts

- **[VERIFIED]** Pinned `llama-server` exposes many CLI options with corresponding `LLAMA_ARG_*` environment variables, including model, context, parallelism, cache, metrics, authentication, TLS, and router model sources [S67-01].
- **[VERIFIED]** Its router can discover cached models, a custom model directory, or presets; multi-file model directories have naming/layout expectations [S67-01].
- **[VERIFIED]** ROCmFPX publishes hardware/backend-specific build and runtime guidance and warns that tuning varies by hardware, drivers, model, prompt, and recipe [S67-02].
- **[VERIFIED]** CachyLLama adds runtime cache, per-user, and Vulkan tuning controls while saying it adds no new build flags [S67-03].
- **[VERIFIED]** JSON Schema 2020-12 defines a versioned schema vocabulary and validation model; object property order is not semantically significant [S67-04].
- **[VERIFIED]** RFC 8785 defines a JSON Canonicalization Scheme usable as input to repeatable hashing/signing [S67-05].

## Constraints

- **[INFERENCE]** A raw YAML/TOML file hash is unsuitable as a semantic compatibility key because formatting and ordering can change without changing meaning.
- **[RECOMMENDATION]** Parse and validate, resolve defaults/references, extract the compatibility subset, canonicalize as JSON, then SHA-256 hash it.
- **[RECOMMENDATION]** Secrets are references resolved at startup/request authorization; secret values are never part of manifests, hashes, dumps, logs, or plan IDs.
- **[RECOMMENDATION]** Hardware profiles distinguish discovered facts (`measured`) from operator intent (`configured`) and retain collection commands/timestamps.
- **[OPEN]** Exact node BOM, link identity/independence, memory reservations, storage budgets, and supported software matrix are not yet measured.

## Proposed per-field authority

**[RECOMMENDATION]** Determine who may set a field before applying precedence. A higher-ranked input that does not own a field is invalid, not an override.

| Field class | Authoritative source | Permitted constrained override | Sources that must not set it |
|---|---|---|---|
| Listener, TLS/authentication, secret references, operator roles, tenant isolation, filesystem roots, quotas, durability ceiling, allowed models/plans | Versioned service/deployment policy | Explicitly allowlisted operator CLI/environment field, within policy and recorded in effective config | Hardware, model, plan, request hint |
| Discovered node identity, devices, memory, links, firmware/kernel/driver facts | Signed or locally collected hardware profile | Fresh measurement may replace an older observation while retaining provenance | Model, plan, request hint |
| Artifact identity, tokenizer/template, architecture, quantization, license, supported context/backend claims | Admitted model/shard manifest verified against bytes | No runtime override of identity; admission creates a new manifest revision | Plan, CLI/environment, request hint |
| Rank/device placement, reservations, transport/cache execution choices, objective tuning, approved fallback | Admitted plan manifest constrained by service policy, hardware facts, and model compatibility | Operator may select another already-admitted plan; plan fields remain within declared bounds | Request body may not redefine plan contents or safety bounds |
| Per-request objective and optional plan hint | Authenticated request | Selection only among admitted choices allowed for that principal | Cannot change identity, security, quota, durability, topology, or compatibility fields |

Within each permitted field class, compiled safe defaults are lowest, the class authority supplies the declared value, and an explicitly allowlisted operator override may be higher only where the table permits it. Unknown fields, unauthorized setters, unresolved conflicts, and values outside policy fail validation. The runtime emits the winning source for every effective field. **[OPEN]** The production CLI/environment allowlist remains a security and operations decision.
