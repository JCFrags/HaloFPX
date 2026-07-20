# ADR-0030: excluded Linux full-v1 snapshot reader

Status: accepted for the default-excluded L08e synthetic snapshot boundary.
This does not admit server linkage, protected root or key acquisition, a writer,
a codec, or live-state restore.

## Context

ADR-0029 composes an authenticated full-v1 manifest and complete immutable
object roster in memory. The next safe dependency is a Linux filesystem adapter
that can read one exact selected snapshot without turning paths, directory
contents, or unauthenticated manifest fields into authority. The owner directed
focused qualification rather than a broad storage-fault matrix at this stage.

## Decision

`halofpx-context-store-v1-linux-read-only` is a Linux-only
`STATIC EXCLUDE_FROM_ALL` target. Its factory receives an already-open root
directory descriptor, an exact expected root identity, the existing manifest
verification policy and synthetic admission, and explicit frame and aggregate
limits. It duplicates the descriptor and owns immutable copies of the policy,
admission, and key material. Root acquisition and authorization remain external
and are not admitted by this decision.

The adapter opens only fixed `manifests` and `objects` children using `openat2`
with `RESOLVE_BENEATH`, `RESOLVE_NO_SYMLINKS`, `RESOLVE_NO_MAGICLINKS`, and
`RESOLVE_NO_XDEV`, plus `O_NOFOLLOW` and `O_CLOEXEC`. It never enumerates a
directory or accepts a caller path. The selected manifest filename is exactly
`m-<anchor-selected-lowercase-sha256>.cbor`.

The manifest must be a same-owner regular `0600` single-link file on the pinned
device and mount, with a positive bounded length and stable descriptor identity
before and after an exact read. The adapter authenticates and replay-checks the
manifest before it derives any object filename. Each object filename is exactly
`o-<authenticated-object-id-lowercase-sha256>.bin` and receives the same type,
owner, mode, link, device, mount, length, EOF, and identity checks. The checked
aggregate limit is applied before allocation.

Only after every object is read does the adapter delegate the owned snapshot to
the ADR-0029 provider, which authenticates the manifest again, exact-matches the
fixture admission, verifies every frame, and constructs one all-or-nothing
candidate. Missing or incomplete material, corruption, incompatibility, replay,
unsupported capability, and storage failure remain misses with no partial
candidate. Publication is disabled and all reported product capabilities are
closed.

Factory bounds are checked before any borrowed admission range is copied. Root
identity validation occurs before the authentication key is copied, leaving the
secret copy as the final allocating construction action. Normal-destruction
wiping is best effort and makes no allocator, compiler, crash, or process-
remanence claim.

## Consequences and closed gates

L08e proves a real Linux filesystem read against synthetic fixtures, not a
production persistent hit. Protected root and key acquisition, an authenticated
publication anchor, real codec/profile admission, streaming into bounded live
storage, a full-v1 writer, retention and quotas, and live restore remain closed.

Root and fixed-directory metadata are point-checked while each regular file has
a before/after identity check. Factory root or `statx` capability failure is an
exception rather than a product miss. Missing selected manifests are classified
as incomplete. Exhaustive wrong-mode, hardlink, cross-mount, mid-read race,
syscall-fault, crash, and storage-exhaustion permutations are deferred until the
writer or product edge opens those risks.

Rollback is one coherent revert. The target has no install, server, writer,
enumeration, codec, restore, donor, or service-deployment edge. No GPL llama-ai
implementation, CachyLLama code, WebUI asset, remote, or new dependency entered
this decision.
