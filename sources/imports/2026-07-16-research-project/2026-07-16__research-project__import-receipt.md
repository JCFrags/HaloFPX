# Research project import receipt

Date: 2026-07-16
Reason: Establish the contents of `C:\Users\britt\Desktop\Research project` as the starting basis for the HaloFPX LLM Wiki project.

| Action | From | To | Status | Notes |
|---|---|---|---|---|
| Preserve | Desktop research project inputs | `sources/imports/2026-07-16-research-project/` | complete | Both ZIPs and the AGENTS template retained unchanged. |
| Extract | `HaloFPX_LLM_Wiki_Research_Prompts.zip` | `research/prompts/` | complete | 479 files; 86 standalone `PROMPT.md` assignments. |
| Extract | `HaloFPX_LLM_Wiki_Empty_Scaffold.zip` | `wiki/HaloFPX_Wiki/` | complete | 100 scaffold files. |
| Adapt | `AGENTS.md.template` plus Agent Harness governance | repository `AGENTS.md` | complete | Project routing, evidence labels, provenance, and review loop retained. |

## SHA-256

- `AGENTS.md.template`: `BE08B02FC574DDF282C9A82FEF1E429B6DF105A13359982796BC14D8709DB93D`
- `HaloFPX_LLM_Wiki_Empty_Scaffold.zip`: `CFDB32A400780AF81D9FED373C672834F0824E242FF5F208CA4C6D2B227DFE8B`
- `HaloFPX_LLM_Wiki_Research_Prompts.zip`: `CEC5735E985321A4C3075ED65BE005EA1B7494DA127C7CA0FCDE006174B7C37C`

## Verification

- Source checked: three expected input files existed.
- Destination checked: prompt package and Wiki scaffold have separate canonical paths.
- Count/hash checked: 479 prompt-package files, 100 scaffold files, 86 standalone prompts; hashes recorded above.
- Related docs updated: root `README.md`, `AGENTS.md`, and `references/agent-harness.md`.
