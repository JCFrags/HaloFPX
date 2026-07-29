# L96 result

Disposition: **NOT PROMOTED; no production/model attempt was consumed**.

## Retained correction

L96 adds a default-off relocatable staged-runtime build contract. When
enabled, CMake links the staged executables and bundled project libraries with
the exact `DT_RUNPATH`:

`$ORIGIN:/opt/rocm/lib`

The controller-enforced nimo-2 gate runs before `controller.preflight()` and
therefore before any production shutdown. It cross-binds the source archive,
local and staged manifest, helper, source root, build ID, worker/canary/
placement binaries, modes/owners, all DT_NEEDED resolutions, relative
non-escaping library symlink chains, and canonical canary provenance. It
rejects DT_RPATH, any other DT_RUNPATH, missing/tampered binaries, unresolved
dependencies, unapproved roots, and malformed/escaping symlinks.

Focused gates: 74 tests passed; Python compilation and diff checks passed.
The corrected independent pre-runtime review passed with no P1/P2.

Identity:

- source root:
  `9b2070e771c182496827e53e99faf2a012cafb3eb7a6b9ec6857d0f6cf9fdac9`;
- build ID:
  `4f51a77efb54648e958fe2f00056f22231098e6ffeff3bc3c3d7b07977319a76`;
- source archive: 167436800 bytes, SHA256
  `3167f1ca4cc9a5d2ac1dcfc304146dd21dfc73c1bd9b8e59d4418158fb603446`;
- build archive: 237393920 bytes, SHA256
  `5a61af2bf54adb7452fb6e6da83e9c170d5305cfe7f2b1386577d791d0d565fb`;
- worker:
  `a69dc9f5d5e7e5e04e6a7772b11e278916ad8070c3f74bcddd38df984a9e46ad`;
- canary:
  `cd3ea6ce5607a593c6382a68499985907cbfe90e95d0a3453d5916bf16f48492`;
- placement:
  `efc690a08f54d6a3d6884a84af1620ed1ed77d27e6f69cc24903c6f97884e991`.

The matched feature-off Linux build completed with
`GGML_RPC_HALOFPX_LOCAL_STATE=OFF` and
`HALOFPX_RELOCATABLE_STAGED_RUNTIME=OFF`.

## Exact terminal boundary

The separately retained source/build/model/capacity/production preflight
passed. During the conditional controller invocation, the source-enforced
package gate reached its no-model probes but refused because:

`test-halofpx-distributed-state-canary --help`

returned rc `2` with empty stderr. The gate therefore produced no PASS receipt,
the controller never reached production preflight/shutdown, and no child,
model, capture, restore, authority, token, state, or cache work ran.

This is a package-gate probe-contract P2: the exact canary does not treat
`--help` as a successful no-model probe. It is not evidence against ELF
relocatability, dependencies, model behavior, or cache correctness. Per the
kill gate, L96 made no correction and performed no retry.

Controller stdout was empty (SHA256
`e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`);
stderr was 1354 bytes (SHA256
`56646872278b062313abf47039ab3f9b769da6e2ff029cc7a999532e932f83de`).
