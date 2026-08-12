# Maintainer and reviewer runbook

## Intake

1. Freeze the exact proposed artifact tree and assign a release identifier.
2. Hash every file and record symlinks, gitlinks, submodules, LFS pointers, archives, and generated outputs.
3. Export exact Git status, commits, submodule status, and build environment.

## Scan and reconcile

4. Run a full-tree license/origin scan and compare it with `manifests/files.*`.
5. Reconcile every exception, dual license, generated file, copied fixture, and missing header.
6. Resolve each row in `manifests/unresolved.*`; never replace `NOASSERTION` with a guess.

## Build and map

7. Rebuild from clean, pinned inputs.
8. Generate source and binary SBOMs.
9. Populate the source-to-binary map, including embedded Web UI assets.
10. Capture build provenance and reproduce outputs where feasible.

## Obligations and notices

11. Determine the release/distribution model and all corresponding-source obligations.
12. Assemble license texts, copyright notices, attribution, Apache/LLVM exception material, npm package notices, model/data terms, and documentation notices.
13. Verify that source-access links/offers remain adjacent to corresponding binaries where required.

## Approval

14. Approve clean-room roles and review contamination evidence.
15. Record maintainer/legal signoffs for admissibility, permissions, release model, and residual risk.
16. Archive the final SBOM, hashes, notices, sources, binaries, provenance, and decisions together.
