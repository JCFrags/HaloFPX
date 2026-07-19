# L05z predecessor-envelope focused-core independent review

**Result: ACCEPT_FOR_CORE_CONTINUATION_NOT_PROMOTION.** The seven-file L05z
core is accepted for the separate crash and returned-fault qualification lanes.
This is not milestone promotion or authorization to enable persistence.

## Reviewed boundary

The frozen diff is based on parent
`0e8af1fb8f637ce86dfd5287621eab6b09a1c52f` / tree
`3248cce359420e259eb28349e5826c8748f33c81`, with patch-stream Git object
`e570e63d5e7b2ff578642efd574307922cc93a30`. The receipt records the exact
SHA-256 identity of all seven modified source and test files; the two evidence
documents are intentionally not self-hashed.

The implementation conforms to the accepted L05z contract. It preserves the
L05w/L05x/L05y entrypoints as exact controls, retains fd4 and every readback in
locked storage, uses the facts-only protected-registry verifier, binds every
launcher pin, derives the exact lowercase registry-lab digest name, enforces the
4014/4015 path boundary and inherited logical/reserve limits, and closes the
writable marker descriptor before the first step-4 mutation.

Publication is exclusive temporary creation, fd-bound mode validation, bounded
exact write/read/EOF/authentication, file synchronization, immediate read-only
identity and content revalidation, sole `RENAME_NOREPLACE`, destination
`envelopes/` synchronization, then source `staging/` synchronization. The
final object remains bound to the original temporary inode and exact fd4 bytes.
The unchanged authenticated marker is confirmed again after the final layout
and envelope checks before qualification. `HEAD` remains absent, attempts and
staging remain empty, and every result remains discard-required.

## Corrections closed by final review

The first review rejected a marker-substitution window and four test-oracle
gaps. The frozen source closes them: a second final marker reopen verifies
pinned identity, metadata, exact length/EOF/bytes, authentication, and content
digest; inherited controls require every L05z field to remain zero; the live
oracle independently reconstructs the on-disk marker digest; returned envelope
digest and name are compared with the exact request; and the seam contract
checks the critical write/read/EOF/authentication/fsync order and occurrence
counts. All affected evidence messages now identify L05z accurately.

## Focused evidence reconciliation

- nimo-1 Release passed 6/6 focused tests. The CTest log is
  `6db6554df65bdf086bf0c4c5be1580ce25b0aea03808eea51d1f31379b161505`,
  the anchor executable is
  `c5464c45463fde9c232f49e0b9e3acf1b2563c168d6f9b6c2f790f083a8cb4e1`,
  and the two-object initializer archive is
  `8971b5676a26a17f19ed6a24e433ec7282a7aa6325a4f507cac0ed35935674ce`.
- nimo-2 passed 6/6 in the correctly labeled optimized Release ASan/UBSan
  lane: `-O3 -DNDEBUG` plus
  `-fsanitize=address,undefined -fno-omit-frame-pointer`. Its log is
  `c789690a0fd5950e6ae130b3b1c302532b7f99f4c5bc60b0b5b8b64d6b1d803a`,
  executable
  `0fdbb3b4cd59a564fc4df6130b4811fb599fae8ca558c6f87206313829acfd22`,
  and archive
  `f3a0d6a654344423b9cea2d812bf9613d18e891f04c92f9f7cbb0568d8d43eab`.
  The executable links both sanitizer runtimes and the retained log contains no
  sanitizer diagnostic.
- The final `-06` nodiscard live runs passed on both nodes. Their logs are
  `fe82c882eec5e9ae180808e5e9f3133acf696aa2a3d32d566ccdb02c38a90715`
  and `a10f130df2881a2d7ae8598bfceebbdbd4fe08b6da072e92dcbb29fd859b4179`.
  Each image remained exactly 1 GiB and fully allocated as 2,097,152 512-byte
  blocks before and after detach. No qualification loop, mount, or child
  process remained.
- nimo-1 `llama-server` remained healthy on port 8081 as PID 971 after the
  prior controlled reboot. nimo-2 `ggml-rpc-server` remained continuously
  reachable on port 50052 as PID 3562775. Retained service evidence reports
  zero restarts for both.

The sparse-after-discard `-05` images remain excluded from the full-allocation
claim. The initial rejected source and five-of-six Debug sanitizer archive
audit, plus later seam-only reruns, remain preserved as excluded development
evidence and are not counted as final passes.

## Provenance, isolation, and rollback

The independent provenance review returned ACCEPT. It found zero matches in
1,108 normalized four-line added-code windows and zero matches among 130
distinctive added lines against the locked llama-ai and CachyLlama trees. No
GPL implementation or documentation, CachyLlama code, donor format, dependency,
direct cherry-pick, NOTICE, SBOM, install, export, or product edge entered the
MIT engine. The target remains Linux-only, default-off, `STATIC
EXCLUDE_FROM_ALL`, and privately linked only to the existing target-owned wire
library. No HaloFPX remote is configured.

All four immutable references remain clean at their locked commit and tree
identities. Rollback is source-only: remove this L05z extent and its audit/test
surface while retaining the qualified L05y control. Any root that crossed the
latch remains whole-media discard-only and is not made adoptable by rollback.

## Remaining gates and nonclaims

Core continuation may now add the separately hashed, excluded qualification
controllers. Promotion remains closed until exact-production ptrace crash
qualification, returned-fault and hostile-input qualification, full inherited
and feature-off regression, retained evidence reconciliation, and another
independent promotion review pass.

This review makes no crash, returned-fault, full-regression, milestone-promotion,
persistence, cache-hit, restore, initialization-completion, power-loss,
distributed, inference-performance, or zero-regression claim. HIP, Vulkan,
ROCmFPX, TurboQuant, ROCmFP4, RPC, WebUI, and L14Q behavior remain unchanged.
