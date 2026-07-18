# L05h authority-admitted bootstrap manifest

Status: accepted offline after independent adversarial review.

L05h removes the raw digest/count trust boundary retained by L05g. A bootstrap
request now carries only its 256-bit attempt identity and exact bounded manifest
bytes. The authority privately owns a distinct manifest-authentication key and
the complete trusted compatibility expectation, in addition to its distinct
anchor and bootstrap-admin key purposes.

The planner derives the exact full-manifest digest, authenticates the canonical
manifest under fixed scope and generation-one/null-predecessor replay state,
checks all compatibility components/root, and derives object count from the
authenticated result. Only then may it construct the same opaque, key-free
`authorized_unexecuted` plan.

No external administrative token, credential store, protected high-water or
replay journal, conclusive anchor-absence check, execution primitive, object
reader, filesystem, server/provider path, persistence, cache hit, or node path
is introduced.

The clean Windows CPU/WebUI-off Release control passed all 18 HaloFPX CTests
and seven focused inherited regressions. The exact manifest-authentication and
authority binaries each passed 100 independent processes. The first review
corrected an attempt-identity wording overclaim. The full clean matrix then
found that the new excluded authority consumer was missing from L04a's exact
offline allowlist; only the two authority source paths were admitted, and
independent re-review confirmed no runtime leakage. Exact hashes and counts are
retained in `evidence/l05h-manifest-authority-repeat-receipt.json`.
