# P05 RPC small-command coalescing rejection review v01

Date: 2026-07-20

Verdict: **ACCEPT**

## Scope

Independent review covered the P05 candidate mechanism, exact build-source authority, matched performance arithmetic, raw evidence manifests and bundles, dual-rail counters, deterministic output, rollback, reference-clone integrity, and provenance boundaries. No additional test expansion was requested.

## Findings resolved before acceptance

1. The exact node build source is commit `6c01b9769c3d0c0034acfdd3c1e47c3f632ea670`, tree `d0905b9c2d77290d1f0b4dcf898aaeb4c3c60432`, source archive SHA-256 `6fed59764da4f3d9c02737382c6fc1da52c92e539a0fe9c487cf0971e8650154`, plus candidate patch SHA-256 `f59dfe0cf261bbdccccab9bd49203b2308438ada6b97479c72f66e7b5637c182`. Commit `a179d16be57a0bfe5814a82691f54b5dafcf6de4` is separately recorded as the logical repository tip; the intervening changes were documentation-only.
2. The record now states precisely that no P05 source is integrated in the implementation repository and no P05 binary is deployed or running, while inert node-local patched source and build artifacts remain retained as experiment evidence.
3. The promotion-rule wording no longer claims a retained pre-run declaration that was not present in the sealed P05 evidence.

## Independent checks

- Recomputed means, sample deviations, Welch-Satterthwaite degrees of freedom, and approximate 95% intervals match the receipt.
- The mechanism diagnostic matches 8,509 OFF versus 8,257 ON `sendto` calls, a reduction of 252 calls or 2.96157%.
- Request admissions, HTTP statuses, token counts, request hash, and decoded-output hashes match the retained ledgers.
- Both node manifests verify completely: 46 entries on nimo-1 and 94 entries on nimo-2; manifest and compressed-bundle hashes and sizes match the receipt.
- OFF and ON binaries and CMake caches match across nodes, and the compile definition is present only in ON.
- All experiment units stopped successfully. The preserved nimo-2 RPC worker and nimo-1 server are active with zero restarts, original hashes, healthy HTTP status, and two active MPTCP subflows.
- All five immutable reference clones remain clean. No remote, donor implementation, GPL llama-ai code, model, notice, license, or SBOM changed.

## Decision

The candidate is correctly rejected without rescue trials. Its syscall reduction is real, but prompt processing, generation, and end-to-end point estimates are all adverse and their intervals cross zero. The evidence supports neither a speedup nor a reproducible regression. Generation above 30 tokens/s remains an owner stretch objective rather than a fact or pass/fail gate.
