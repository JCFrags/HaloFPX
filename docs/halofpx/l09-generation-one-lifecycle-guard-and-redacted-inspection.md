# L09 generation-one lifecycle guard and redacted inspection

L09 turns the L08i explicit-handle laboratory canary into a bounded,
single-generation lifecycle. It adds lifetime-held writer authority, strict
layout accounting, quota and filesystem-reserve admission, restart closure,
and fixed redacted controller observations. The complete feature remains
Linux-only, compile-gated, runtime-opt-in, private, and default-off. It is not
production persistence.

The implementation parent is HaloFPX
`8ba77d9d39862ebd6116a10b67d1ae7bef057b31`, tree
`447087a3da4ded080407e19ce303e0cd5320e2f2`. The locked ROCmFPX base remains
`61f2f2d7bc4955e9bca821095ef69125837133b5`, tree
`0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`.

## Delivered boundary

The generation-one authority now:

- holds both data-root and anchor-root OFD writer locks for its lifetime,
  acquired in a deterministic root-identity order;
- passes a typed caller-held data-lock capability into the inherited
  materializer, while leaving its unlocked excluded-test entry point intact;
- admits only the fixed data layout (`writer.lock`, `staging`, `objects`, and
  `manifests`) and the fixed anchor records for the one selected generation;
- accounts logical and allocated bytes separately, rejects duplicate inodes and
  unexpected paths, and exposes no online-deletion or eviction claim;
- requires `max_entries == 1`, enforces the configured logical-byte quota, and
  preserves the full configured reserve on every distinct backing mount;
- budgets immutable data, manifest, pending, anchor, and terminal upper bounds
  before publishing the pending record;
- rechecks reserve headroom before anchor visibility and keeps an already
  durable pending record for restart reconciliation if that late check fails;
- closes writes for every published, recovered, budget-exhausted, storage,
  synchronization, layout, accounting, or quarantine terminal state; and
- reports only fixed lifecycle, close-reason, byte-count, accounting-valid,
  writes-closed, and eviction-class fields to controller logs. Request results
  remain coarse and disclose no root, principal, digest, key identifier, or
  storage-layout fact.

There is no new HTTP inspection route because the server has no independently
admitted administrator role. There is no online eviction, retention deletion,
shared scope, automatic discovery, generation advancement, or product
admission.

## Target qualification

The representative nimo-1 Release build used GCC 16.1.1 on Linux 7.1.3 with
WebUI, HIP, Vulkan, and curl disabled and all four context-store canary gates
enabled. The selected focused and inherited set passed 12/12 in 0.05 seconds:
feature-off and L02 contracts, the inherited state transformer, full-v1 codec,
attempt wire, Linux reader, Linux publisher contract, and the generation-one
authority plus their static contracts.

The process-level canary used the retained Stories 15M Q4_0 fixture, SHA-256
`66967fbece6dbe97886593fdbb73589584927e29119ec31f08090732d1861739`.
It published an exact five-token state under selected manifest
`1c427401aae475428d9a0ac15588c28806b11c441df85e7b4f9e781885a8e76b`,
then stopped the process. A new process authenticated and restored the exact
handle as a hit. The final generation accounted 37,043 logical bytes and
53,248 allocated bytes, reported `recovered-success`, and remained write
closed.

A single representative reserve canary configured a 64 GiB reserve while the
observed available space was about 35.4 GB. Publication returned the deliberately
coarse `storage` result, the controller recorded `reserve-exhausted`, and no
pending, anchor, terminal, manifest, object, or staging file was created.

The first live layout run found one real portability defect: Btrfs reports link
count one for ordinary directories. The scanner's `st_nlink >= 2` assumption
therefore rejected a valid layout. L09 removed only that nonportable check;
directory type, owner, mode, device, mount identity, and `openat2` containment
remain mandatory. The corrected Btrfs canary and the selected 12-test set both
passed.

Raw sanitized evidence remains on nimo-1 at
`/var/tmp/halofpx-l09-evidence-20260720`. Its manifest SHA-256 is
`1458ba47400a12fe35062253917d40ab4e69ad7c6c5fe04be9a3849f8223a052`.
The compressed evidence bundle is
`/var/tmp/halofpx-l09-evidence-20260720.tar.zst`, SHA-256
`c65d2bbbf2420a702484d16eb54eaea20fbcee981b2edfdee3fa0e70fa2da3ed`.
The gated `llama-server` SHA-256 is
`7ef354e70ff8a69bf33b9eb4d57c6ef4bc17e06720f99647e074624f8d043851`.
The exact component-tuple record SHA-256 is
`cb0fde361fd2b902cdd9bac8892788313cb556024363039b5b6016fb724f4877`.
The known-good nimo-1 MiniMax service and nimo-2 RPC worker remained active and
enabled.

## Review against the Wiki and next gate

ADR-0036 freezes the state, scope, storage, security, failure, and observation
contract used here. The implementation follows the canonical Wiki's
validate-before-use, corruption-as-miss, immutable/no-replace publication,
data-before-visibility synchronization, single-writer, reserve, private-scope,
and no-unsafe-eviction requirements. It does not claim exact-filesystem
power-loss durability, production administrator authority, shared reuse,
multi-generation retention, distributed cache recovery, or production
persistence admission.

Broader filesystem/fault permutations, disk-full campaigns, concurrent-writer
stress, generation advancement, retention/deletion, multi-node cache recovery,
and long soak remain deferred until a concrete risk hypothesis or the product
admission boundary makes them material. The next product step is a narrowly
admitted default-off operational cache canary and then current-head matched
requalification and optimization of the already pinned and early-qualified
160 GB primary MiniMax workload; persistent writes remain gated.

No donor implementation, GPL llama-ai code, CachyLLama transplant, new
dependency, WebUI, remote, release, model mutation, or deployed-service change
entered L09. All five preserved reference clones remained clean at their locked
commit and tree.
