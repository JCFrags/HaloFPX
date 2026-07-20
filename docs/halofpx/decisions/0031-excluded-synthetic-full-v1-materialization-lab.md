# ADR-0031: excluded synthetic full-v1 materialization lab

Status: accepted only for the disposable, explicitly non-authoritative L08f
write-mechanics boundary. This is not the production full-v1 writer and cannot
create or advance publication authority.

## Context

ADR-0030 reads one exact synthetic-policy-selected full-v1 snapshot but deliberately
provides no write path. Before designing protected generation advancement, the
project needs direct evidence that target-native Linux primitives can place the
already-authenticated synthetic fixture on disk with data-before-manifest
ordering and then reproduce a read after recreating the provider.

The initial L08f writer draft was rejected in independent review because it
overstated authority, used only a per-instance fence, did not reread bytes before
visibility, exposed `noexcept` allocation termination, and lacked honest
post-rename uncertainty semantics. Those failures prohibit promotion as a
writer and define the narrower laboratory decision below.

## Decision

`halofpx-context-store-v1-linux-publish` remains a Linux-only
`STATIC EXCLUDE_FROM_ALL` target, but its public class is a synthetic snapshot
materializer and its only positive statuses are explicitly non-authoritative.
It accepts the L08d synthetic source, prevalidates the complete manifest and
ordered frame roster through the memory provider, copies the selected digest
from the validated candidate, and performs no I/O on prevalidation failure.
Caller-provided key, replay, and admission material is laboratory input, not
writer authority.

The disposable root must contain fixed `staging`, `objects`, and `manifests`
directories plus a permanent private `writer.lock`. The materializer duplicates
an already-open exact root descriptor and holds an OFD write lock on
`writer.lock` for the complete attempt, fencing other cooperating processes and
publisher instances. Every open uses `openat2` with beneath, no-symlink,
no-magic-link, and no-cross-mount resolution.

Before mutation, the immutable namespace must be either entirely absent or
entirely exact. Exact existing files are safe-opened, bounded, reread byte for
byte, synchronized, and identity-checked; their parent object and manifest
directories are then synchronized and revalidated. An all-exact root returns
`already_equal_non_authoritative`. A partial, mixed, wrong, or hostile root
conflicts. This preflight may improve durability of exact files but performs no
byte or namespace mutation.

For a fresh root, each object is written to an attempt-scoped `0600` staging
file, reread exactly after file synchronization, identity-checked, and renamed
with `RENAME_NOREPLACE` to its authenticated digest name. Only after all object
files and both affected directories synchronize does the materializer stage,
reread, synchronize, and no-replace rename the selected manifest. It then
synchronizes the manifest and staging namespaces.

Any non-success from the first write attempt onward returns
`incomplete_or_uncertain_discard_root`; retry, adoption, or salvage is not
admitted. The whole disposable root must be discarded. All allocations are
caught inside the `noexcept` operation.

## Consequences and closed gates

L08f proves an actual synthetic miss, synchronized materialization, provider
recreation, and authenticated hit. The returned hit remains the L08d fixture
candidate and cannot restore live inference state.

There is no protected material carrier, persistent attempt registry,
restart-time reconciliation, authenticated anchor replacement, generation
advancement, retention, quota, reserve, server link, codec, or product
durability claim. These remain mandatory for the production writer. Publish-
oriented filenames and target names are retained as internal clarity debt; the
class, result values, tests, and documentation are normative about the
non-authoritative boundary.

Rollback is one coherent revert. No donor code, GPL llama-ai implementation,
CachyLLama transplant, WebUI asset, remote, new dependency, service deployment,
or persistent user data entered this decision.
