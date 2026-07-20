# L05z mapped-but-closed role-authority independent review

**Result: ACCEPT_FOR_EXACT_SELECTOR_IMPLEMENTATION_NOT_EXECUTION_ADMISSION_NOT_L05Z_PROMOTION.**

Map v1 gives all 247 formerly aggregate-only roles stable target-owned
source-and-trace identities: 86 `fstat`, 36 `statx`, 30 `readlink`, 32
`getdents64`, 40 Btrfs-facts, and 23 `fstatfs`. The six reviewed fragment
hashes, source snapshot, physical profile, role fields, family counts, and
closed admission state produce manifest commitment `190abec2...`. Every row is
`mapped_not_executable`; none is an admitted returned-fault selector or cell.

The controller reinforces that distinction. All six aggregate boundary names
are rejected during CLI validation before target validation or process launch.
Closed-CLI v2 returned 2 for every family on both nodes, created no receipt,
emitted empty stdout, and produced the identical 851-byte usage diagnostic.
The exact results table is `200963e0...` and 56 bytes. Per-command traces
contain no `clone`, `fork`, or `vfork` call. This is negative execution evidence,
not successful fault injection.

Both nimo-1 and nimo-2 passed compile, link, mapped authority, legacy role,
response, semantic, hostile, static, feature-off, and L02 controls. The primary
project-default GNU++17 build produced identical binaries at `c1d8f885...` and
objects at `7ad41887...`; the explicit ISO C++17 lane produced identical
binaries at `bff90d61...` and objects at `035a3777...`. The mapped self-test is
`f21ba801...` and 352 bytes. Previously accepted role, response, and semantic
stdout remained byte-identical. Exact hashes, sizes, and evidence roots are in
the receipt.

The inherited feature-off and L02 checks validate only the unchanged product
and contract surface. The mapped-role evidence snapshot contains tests and did
not rebuild a complete server. No product source, link, install/export edge,
runtime default, persistence setting, or provider behavior changed.

Evidence bundles remain at
`/var/tmp/halofpx-l05z-mapped-role-authority-65ef0eb-20260719-evidence.tar.zst`.
Nimo-1's bundle is `811beca4...` and 1,231,132 bytes with manifest
`d77fd44c...` and 23,682 bytes; nimo-2's bundle is `0663bf34...` and 1,232,123
bytes with manifest `ee22dc49...` and 24,487 bytes. Node-specific strace PIDs
and environment evidence account for the expected bundle differences. Both
nodes ended with zero controller processes, qualification-root mounts, and loop
devices. Nimo-1's server remained PID 971 with HTTP 200; nimo-2's RPC server
remained PID 3562775 listening on port 50052. All four locked reference clones
remain clean at their exact recorded commits and trees.

One pre-existing selector defect independently prevents admission. The current
reserve-revalidation selector matches six root `fstatfs` rows (002, 003, 010,
011, 018, and 019), and its ceiling would select 002, 003, and 010; the true
reserve roles are 003, 011, and 019. This alias must be replaced by an exact
fail-closed phase selector. Other family-specific runtime witnesses and
late-success oracles likewise belong in the later admission version, not in
map v1.

Map v1 is therefore frozen closed. A later implementation must introduce a new
versioned exact-selector admission manifest, preserve map v1 as evidence, and
receive independent review before any mapped boundary can execute. Mutating v1
in place or treating clean-path ordinal identity as runtime authority is not
acceptable.

Canonical Wiki categories 63 and 80 were reviewed. Their requirements for exact
fault boundaries, disposable targets, fail-closed outcomes, and retained raw
evidence agree with keeping this map closed. No execution result or promotion
claim was added to the Wiki.

The work is target-native test infrastructure. It contains no donor or GPL
llama-ai implementation or documentation, adds no dependency or direct cherry
pick, and needs no P3 admission record. It remains default-off and absent from
product execution.

This review does not admit any of the 247 roles, qualify a returned-fault cell,
freeze physical execution cardinality, close full-scale or sanitizer coverage,
promote L05z, enable persistence, or make a durability, inference-performance,
or zero-regression claim.
