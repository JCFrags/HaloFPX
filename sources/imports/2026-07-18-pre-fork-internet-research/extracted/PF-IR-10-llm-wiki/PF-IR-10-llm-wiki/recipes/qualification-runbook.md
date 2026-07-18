# Isolated qualification runbook

**Claim labels:** `PROPOSAL`, `QUALIFICATION-REQUIRED`.

This archive records no execution of llama.cpp, ROCmFPX, CachyLLama, or HaloFPX. Qualification is a separate, locally approved activity.

## Required controls

1. Verify `MANIFEST.sha256` and the archive hash before extraction.
2. Review `manifests/candidates.json`, then check out exactly the recorded commit. Do not use a moving branch.
3. Review source-derived applicability against the local checkout. The proposal is not authoritative.
4. Build in a disposable, network-disabled environment with compiler and dependencies recorded.
5. Execute only fixtures whose accepted status and candidate applicability have been approved.
6. Capture command, environment, executable hash, stdout, stderr, exit status, API body, and produced state files without overwriting this evidence package.
7. Normalize candidate output with the declared comparator profile; preserve raw output separately.
8. Obtain human approval of the exact accepted-asset manifest and any expected-reject exceptions.

## Prohibited shortcuts

- Do not infer `HaloFPX` identity.
- Do not download excluded publisher weights through candidate helper scripts.
- Do not accept opaque state-file byte equality as a cross-fork semantic oracle.
- Do not convert an `open` row to `required` solely because one fork accepts the input.
- Do not execute files under `adapters/` from this evidence package directly; copy and review them into the isolated workspace first.
