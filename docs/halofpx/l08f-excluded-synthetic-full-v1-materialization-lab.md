# L08f excluded synthetic full-v1 materialization lab

L08f delivers the first full-v1 filesystem lifecycle: an absent selected
manifest is a miss, the complete authenticated synthetic snapshot is
materialized with objects before manifest visibility, and a newly constructed
reader returns the exact candidate and payloads. The result is intentionally a
disposable write-mechanics experiment, not production publication authority.

The implementation parent is HaloFPX
`8c39065b35ca1fc132bca9946d747c08c56e5fd8`, tree
`5bd490c64472e10f8ff2cc68541f68c00ce9967c`. The locked ROCmFPX authority
remains `61f2f2d7bc4955e9bca821095ef69125837133b5`, tree
`0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`.

## Delivered boundary

The Linux-only excluded materializer prevalidates and owns the complete
synthetic source before opening `writer.lock`, fixed child directories, or
performing mutation I/O. A permanent `0600` writer lock
and OFD locking provide one cooperating writer across instances and processes.
Fixed-directory `openat2` containment, exact digest names, `O_EXCL` staging,
post-sync byte rereads, stable identity checks, no-replace renames, and
destination/source directory synchronization establish the laboratory
data-before-manifest sequence.

Preflight admits only an entirely absent or entirely exact namespace. Exact
reuse synchronizes every file and both destination directories. A mixed root
conflicts. Once a fresh materialization attempt begins, every later failure is
`incomplete_or_uncertain_discard_root`; no retry or adoption is permitted.

Positive results are `materialized_non_authoritative` and
`already_equal_non_authoritative`. They cannot update an anchor, advance a
generation, enable a server, admit a codec, or restore live inference state.

## Risk-proportionate qualification

The representative nimo-1 GCC 16.1.1 Release build used CPU mode with HIP,
Vulkan, curl, and WebUI disabled. Eight focused tests passed: feature-off and
L02, the inherited memory and Linux readers plus static contracts, and the new
materializer and contract. The four materializer cases prove:

- fresh miss, materialization, reader recreation, and exact two-payload hit;
- corrupt source rejection before any filesystem byte or namespace mutation;
- all-exact synchronized reuse that preserves the original hit; and
- a separately held OFD root lock returning busy with no materialized files.

Published fixture files are regular, private `0600`, and single-link. The
known-good MiniMax service remained active and enabled. All eight transferred
source/test hashes matched, and the five immutable reference repositories
remained clean at their exact locked commits and trees.

The retained bundle is
`/var/tmp/halofpx-l08e-linux-reader-9907b90-20260720/halofpx-l08f-synthetic-materialization-nimo1-20260720.tar.zst`,
1,817 bytes, SHA-256
`0b3b82471820e891ab3b915cbecbb544fb91843a785985a6b5ca1eb83df5ba44`.
The focused materializer test SHA-256 is
`4bcf9c106a721b070644cab11bd64e62ef3c2e4275ef9e5c6a4eec2e9053423a`.

## Review, rollback, and next gate

Independent review initially rejected five P1/P2 defects and two subsequent
durability/uncertainty defects. The final narrowed implementation closes them
with explicit non-authority, OFD root fencing, exact post-sync rereads,
idempotent exact synchronization, owned validated naming, complete exception
mapping, all-absent/all-exact preflight, and whole-root discard semantics.
Final verdict: accept for the disposable laboratory boundary; no P1/P2 remains.

The next gate is a genuinely authoritative material carrier plus persistent
attempt registration/reconciliation and authenticated generation/anchor CAS.
Only that later seam can become the full-v1 writer used by a server canary.

No donor implementation, GPL llama-ai code, CachyLLama transplant, WebUI,
remote, dependency, persistent user data, or service deployment entered L08f.
