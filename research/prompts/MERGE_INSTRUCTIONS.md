# Merging Research-Agent Outputs into the HaloFPX Wiki

Keep this prompt package separate from the final wiki. The recommended local layout is:

```text
HaloFPX_Project/
├── AGENTS.md
├── source/
│   ├── ROCmFPX/
│   ├── CachyLLama/
│   ├── llama-ai/
│   └── llama.cpp/
├── wiki/
│   └── HaloFPX_Wiki/
├── wiki-prompts/
│   └── HaloFPX_LLM_Wiki_Research_Prompts/
├── experiments/
├── artifacts/
└── tools/
```

## Intake procedure

1. Place each returned research folder under `incoming/<agent-or-date>/`.
2. Confirm that its category and section path exactly match `section_index.json`.
3. Run the validator against the incoming folder.
4. Review sources, claim labels, applicability, and unresolved conflicts.
5. Compare against the current authoritative section.
6. Merge by file, not by blindly replacing the directory.
7. Archive superseded pages under `_archive/<section-id>/<date>/`.
8. Update `section.yaml`, the root manifest, related-section links, and the decision or assumption ledger.
9. Commit the wiki update separately from source-code changes when practical.
10. Regenerate the root index and rerun validation.

## Conflict rules

- **Measured results** are authoritative only for the recorded hardware, software, model, and experiment configuration.
- **Verified primary-source facts** outrank secondary summaries.
- Later sources do not automatically outrank older sources when they apply to different versions.
- Preserve conflicts explicitly in `facts_and_constraints.md` and list the experiment or source needed to resolve them.
- Never overwrite a still-applicable fact with a recommendation.
- Never treat an assumption as an implementation requirement unless a decision record promotes it.

## Cross-links

Use relative links. Refer to another section by both stable ID and title. Avoid copying large explanations between sections; identify one authoritative page and link to it.

## Sensitive and large material

Do not commit model weights, full cache images, secrets, private prompts, or very large raw logs to the wiki. Store them in the project artifact area and reference checksums, manifests, and local paths.
