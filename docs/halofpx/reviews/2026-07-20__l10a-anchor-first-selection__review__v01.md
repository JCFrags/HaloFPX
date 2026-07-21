# L10a anchor-first selection independent review

Verdict: **ACCEPT for the narrow default-off selection seam after one focused
test correction.** This verdict does not authorize production persistence,
automatic multi-key discovery, shared scope, generation advancement, or a
runtime-mode promotion.

## Scope and authorities reviewed

The review covered only the current uncommitted L10a decoder, authenticated
carrier, `restore_selected` adapter entry point, focused tests, CMake linkage,
and ADR-0037. It checked the implementation against ADR-0008/0009,
ADR-0035/0036/0037, the project L02/L05e/L05f contracts, and the canonical Wiki
rules that corrupt or incompatible cache state must miss/recompute and that
cache identity must bind all output-affecting state.

## Findings

1. **Bounded canonical decoding is correct for the closed anchor schema.** The
   decoder rejects null, empty, oversized, truncated, trailing, reordered,
   duplicate, unknown, indefinite, and non-shortest encodings by accepting only
   the exact nested map counts, ordered integer keys, exact byte lengths, exact
   registered ID, fixed algorithms, and end-of-input. Integer-width arithmetic
   and remaining-buffer checks are bounded by the 1 KiB anchor limit.
2. **No unauthenticated carrier or selected digest escapes.** Parsing writes to
   local value objects. The public carrier remains invalid until the complete
   canonical envelope passes the existing exact HMAC verifier. Fixed trusted
   store, namespace, policy, lineage, manifest-key generation, authority epoch,
   generation, and null-predecessor fields are checked, while only the selected
   manifest digest is learned from the authenticated body.
3. **Filesystem authority is preserved.** `anchor.v1` is opened relative to the
   already pinned anchor descriptor with `openat2`, beneath/no-follow/no-xdev
   resolution. The file must be regular, owner-matched, mode `0600`, link count
   one, on the pinned device and mount, and within the anchor bound. A missing
   anchor maps to `miss_not_found`; malformed, hostile, wrong-authority, or
   unauthenticated state maps to opaque `miss_corrupt`. The selected digest then
   enters the existing exact manifest/admission/object/identity/profile/token
   restore path; it is not accepted as a hit by itself.
4. **Default-off and rollback boundaries remain intact.** The server adapter is
   still an `EXCLUDE_FROM_ALL` Linux target and reaches `llama-server` only under
   the pre-existing full-v1 compile gate chain. The new method performs no write
   and does not change the explicit-handle path. Source rollback is one coherent
   revert; operational rollback remains omission of the gates/runtime mode and
   offline retirement of disposable roots.
5. **Provenance remains target-native.** No donor implementation, GPL llama-ai
   material, CachyLLama transplant, new dependency, WebUI, remote, or reference
   repository change appears in this diff.

## Correction made during review

The first selection test exercised only a missing anchor and therefore could
not prove that the authenticated selected digest reached the existing restore
path. The review added one compact server-level case: publish, close/reopen,
`restore_selected` hit with exact token/state equality, wrong-scope rejection,
and synchronized one-byte anchor corruption followed by safe miss. Its initial
fixture omitted the required owner-only `staging`, `objects`, and `manifests`
directories and correctly failed publication as `storage`; the fixture was
corrected to create the exact `0700` data layout. This was a test-fixture defect,
not an implementation defect.

## Focused evidence

On nimo-2, source `/var/tmp/halofpx-selection-src` and build
`/var/tmp/halofpx-selection-build`:

- `test-halofpx-context-store-v1-server-canary-selection`: 1/1 passed after the
  fixture correction;
- `test-halofpx-context-store-protected-canary-anchor`: 1/1 passed; and
- `git diff --check`: passed locally.

The positive server test proves selected-manifest handling through the full
adapter path. The anchor test independently retains positive decode, corruption,
truncation, wrong key, and wrong fixed-authority coverage. No wider fault matrix
was added because ADR-0037 explicitly defers it and no concrete implementation
defect created a new risk hypothesis.

## Remaining gates

This seam alone does not implement the exact request-key derivation, runtime
mode, prompt-boundary capture, or process canary required by the rest of L10.
Those remain subject to ADR-0037's feature-off, exact-request miss/write/restart/
hit, wrong-principal-or-token, quota/reserve, and independent promotion gates.
There is no blocker to proceeding to that next bounded integration slice.
