# L08e excluded Linux full-v1 snapshot reader

L08e adds the first target-native full-v1 filesystem read path. It loads one
exact anchor-selected synthetic snapshot on Linux, authenticates the manifest
before deriving object names, and returns a candidate only after the complete
object roster passes the already-reviewed L08d admission. It remains excluded,
default-off, read-only, and disconnected from the server and live inference
state.

The implementation parent is HaloFPX
`9907b9045dd71ae82f32269f48ea3d49626d72ae`, tree
`c9927f2961315ae0b2609ccfaf31ad1b05cd626f`. The locked source authority
remains ROCmFPX `61f2f2d7bc4955e9bca821095ef69125837133b5`, tree
`0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`.

## Delivered boundary

The Linux-only `halofpx-context-store-v1-linux-read-only` target is
`STATIC EXCLUDE_FROM_ALL`. It duplicates an already-open, exactly pinned root
descriptor and opens only fixed children and digest-derived names through
`openat2` with the complete beneath/no-link/no-magic-link/no-cross-mount resolve
mask. It performs regular-file, owner, `0600`, single-link, device, mount,
positive bounded length, exact EOF, and before/after identity checks.

Manifest selection comes only from the trusted replay anchor. Object filenames
come only from authenticated descriptors. No directory enumeration, arbitrary
path, writer, discovery, codec, restore, server option, or background work was
introduced. Every transient descriptor closes before lookup returns, and the
returned candidate owns its bounded bytes. Publish remains disabled and the
capability surface remains closed.

The root descriptor, verification policy, and admission are still synthetic
test authority. L08e makes no protected root-acquisition, key-custody, product
profile, codec, or live-restore claim.

## Risk-proportionate qualification

The representative nimo-1 Linux Release build used GCC 16.1.1 with HIP,
Vulkan, curl, and WebUI disabled. Six focused tests passed: the Linux golden
snapshot and static contract, inherited L08d composition and static contract,
feature-off server contract, and L02 contract. Coverage includes an exact
two-object hit, closed capabilities and publication, missing and corrupt second-
object atomic misses, a hostile selected-manifest symlink, exact root-authority
rejection, and pre-construction rejection of oversized admission cardinality.

The known-good MiniMax service remained active and enabled. Local and nimo-1
SHA-256 values matched for all seven transferred source/test files. All five
immutable reference repositories retained their exact locked commits and trees
with clean worktrees.

The retained evidence bundle is
`/var/tmp/halofpx-l08e-linux-reader-9907b90-20260720/halofpx-l08e-linux-reader-nimo1-20260720.tar.zst`,
1,740 bytes, SHA-256
`1dce1907b192b8d083a9e902a9d28986773832170376cba9adfb6497ee1f1dfc`.
The focused test binary SHA-256 is
`20eaa2617fddcc925edb2870ea01ba369023e86bab2e9c1f28f7b24ccc17dbac`.

## Independent review, rollback, and next gate

Independent review accepted the corrected milestone with no remaining P1/P2.
It found and drove fixes for pre-construction cardinality validation, key-copy
ordering on a throwing root check, and unsupported `openat2` `EINVAL` mapping.

Nonclaims remain explicit: root and subdirectory metadata are point checks;
factory root/`statx` failure is exception-classified; missing selected manifests
are incomplete misses; normal key wiping is best effort. Broader filesystem,
race, syscall-fault, crash, and exhaustion permutations are deferred until a
writer or product edge creates the corresponding risk.

Rollback is one coherent revert. The next product-progress milestone is the
default-off full-v1 publication/generation seam; it must establish transactional
data-before-visibility durability while server linkage and actual persistent
writes remain closed during implementation.

No donor implementation, GPL llama-ai code, CachyLLama transplant, WebUI,
remote, new dependency, persistent write, or service deployment entered L08e.
