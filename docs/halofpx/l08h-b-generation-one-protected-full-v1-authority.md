# L08h-b generation-one protected full-v1 authority

L08h-b delivers the first target-native protected full-v1 filesystem authority
that can demonstrate miss, durable material publication, exact hit, and safe
restart recovery. It remains a Linux-only excluded library and does not touch
live llama state or the server.

The implementation parent is HaloFPX
`e62f9295d13955bc86ba846e7db6e3137150d364`, tree
`30812c33e9b28d03eeed18d76f99d059fa08c1a4`. The locked ROCmFPX base remains
`61f2f2d7bc4955e9bca821095ef69125837133b5`, tree
`0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`.

The authority binds the exact anchor, replay policy, admission metadata, root
identities, and authenticated pending/terminal records. Caller buffers are
checked against global manifest, configured per-frame, and aggregate byte caps
before any provider or commitment copy. Publication orders pending, material,
anchor, verified hit, terminal, and pending removal with required file and
directory synchronization. Restart after anchor publication recovers success;
anchor corruption quarantines and forces `miss_corrupt`.

On Strix Halo target `nimo-1`, GCC 16.1.1/CMake 4.3.4 Release qualification
passed the focused L08d-L08h runtime/static chain plus feature-off, L02, and
legacy context-store controls 11/11. The final cap-before-copy and terminal
allocation-safety closeout repairs passed the focused lifecycle again 1/1. The known-good
`minimax-m27-q6-server.service` remained active and enabled. All five preserved
reference repositories remained at their locked commits/trees with clean work
trees.

Independent adversarial review accepted the excluded milestone after fixes for
authority-domain coherence, secret lifetime on constructor failure, root
identity revalidation, authenticated terminal evidence, cap-before-copy, and
explicit thread confinement. The review deliberately did not expand into the
full crash/fault matrix: generation advancement, same-instance concurrency,
disk-full and filesystem mutation campaigns, retention, distributed recovery,
soak, and live server behavior are deferred to the boundary that introduces
those risks.

Closeout review against the canonical Wiki's cache-state safety invariants and
Sections 59/63 found the implementation aligned with bounded validate-before-
publish, immutable/no-replace objects, file and directory synchronization,
single-writer authority, and corruption-as-miss/quarantine. The Wiki still
requires exact-filesystem power-loss, storage-fault, and product-scope evidence;
L08h-b therefore remains excluded and makes no production durability claim.

The next milestone is the smallest default-off server canary that performs an
actual miss, write, process restart, and exact hit or safe recomputation. No
WebUI, remote, model mutation, service deployment, donor code, or product
admission entered L08h-b.
