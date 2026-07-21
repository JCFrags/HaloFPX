# L10b exact-session authority

Status: **implemented and target-qualified; runtime integration remains closed**

L10b implements the target-owned exact request identity required by ADR-0037.
It does not yet connect normal HTTP completions to the persistent adapter.

The resolver accepts only a trusted 32-byte derivation key, the already
resolved opaque private-scope namespace, closed compatibility root, exact
canonical token sequence, a logical prompt boundary equal to the token count,
an output boundary in the admitted nonzero range at or before that logical
boundary, explicit target-only greedy-memoryless profile, nonzero
global/rank/placement digests, and a bounded valid rank/world-size topology
with a nonzero epoch. It returns a 32-byte opaque checkpoint-lineage/session
identifier or an all-zero result on rejection.

The HMAC-SHA-256 preimage is domain separated and deterministically encoded as
a canonical integer-keyed map. Tokens are one definite byte string containing
exactly four-byte big-endian unsigned representations of validated nonnegative
`int32_t` token IDs. Count/size multiplication is checked. The implementation
retains and returns no raw principal, prompt, token sequence, preimage, or key.
Changing scope, compatibility, any token or count, boundary, profile, topology
digest, epoch, world size, or rank changes the result.

The seam is an `EXCLUDE_FROM_ALL` static library and has no link or source edge
into `llama-server`, common argument parsing, or server context. An automated
graph contract enforces that isolation. Normal feature-off builds contain no
exact-session domain marker and perform no new root or key access.

## Qualification

The nimo-2 Release CPU qualification passed 3/3 focused tests: the typed C++
resolver, the product-exclusion graph contract, and an independent Python
golden. Inherited direct authentication and private-scope tests also passed.
The fixed golden session digest is
`90240152fe0449ba92a1746dcdf804d7cca55f0034d7a96d846cc59d87f1a25c`.

Review found and corrected two real boundary defects before promotion: the
initial token representation was not the required fixed-width encoding, and
the profile default was not fail closed. The corrected API defaults to `unset`
and accepts only the explicit admitted profile.

Raw evidence is retained on nimo-2 at
`/var/tmp/halofpx-l10b-exact-session-evidence-20260720-v2.tar.zst`, SHA-256
`d0f0117f9bafa8fb54e49335b2b52995b0aae203de3065c6c4ce4bf4b9df4574`.

## Boundary and next milestone

L10b does not enable a server option, automatic lookup, writeback, cache hit,
response change, persistent user root, prefix reuse, shared scope, multiple
entries, generation advancement, or production persistence. The next milestone
may transport this opaque identifier on an eligible parent completion, attempt
anchor-first restore before prompt evaluation, and publish once at the decoded
prompt boundary under a separate compile-and-runtime opt-in.

No donor implementation, GPL llama-ai code, CachyLLama transplant, new
dependency, WebUI, remote, model, deployment, or reference clone changed.
