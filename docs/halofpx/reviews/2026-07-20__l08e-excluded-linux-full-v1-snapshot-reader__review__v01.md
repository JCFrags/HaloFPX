# L08e excluded Linux full-v1 snapshot reader independent review v01

Verdict: **ACCEPT**. No P1/P2 blocker remains after the reviewed corrections.

## Corrections required during review

- Factory validation now rejects all admission cardinality and limit errors
  before a borrowed object range can be copied.
- Root identity validation now precedes the authentication-key copy, leaving
  the secret copy as the constructor's final allocating action.
- Fixed-flag `openat2` `EINVAL` is classified as unsupported rather than a
  generic storage miss.
- A focused oversized-cardinality regression assertion covers the first defect.

## Review conclusions

- The selected manifest name derives only from the exact trusted anchor. Object
  names derive only from references exposed after successful manifest
  authentication.
- Every path traversal uses `openat2` with the full beneath/no-link/no-magic-
  link/no-cross-mount resolve mask and `O_NOFOLLOW`.
- File type, owner, mode, link count, device, mount, exact bounded length, EOF,
  and before/after identity checks precede the downstream authenticated frame
  verification. No partial candidate can escape.
- The target is Linux-only and `STATIC EXCLUDE_FROM_ALL`. It reports closed
  capabilities, disables publish, and has no server, writer, codec, live-
  restore, donor, install, or service edge.
- The focused golden, atomic corruption, hostile-link, authority, feature-off,
  inherited composition, and static-contract coverage is proportionate to this
  excluded read-only seam.
- The implementation is target-native and adds no GPL llama-ai code,
  CachyLLama transplant, WebUI asset, remote, dependency, or provenance duty.

## P3 nonclaims and deferred work

Factory root or `statx` capability failure is exception-classified rather than
a product miss, selected-manifest absence is an incomplete miss, and root and
fixed-directory metadata are point-checked rather than bracketed around the
entire lookup. Wrong-mode, hardlink, cross-mount, mid-read race, syscall-fault,
crash, and storage-exhaustion matrices remain deferred. Key wiping is best
effort and cannot cover allocator, compiler, crash, or process remanence.

Authorization and root acquisition are external. They must remain closed before
any server linkage or production persistent read is considered.
