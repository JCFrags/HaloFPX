---
title: "Fuzzing and fault-injection plan"
tags: ["fuzzing", "fault-injection", "chaos", "security-testing"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["FUZZ-01", "FUZZ-02", "FUZZ-03", "FUZZ-04", "FUZZ-05", "FUZZ-06"]
related: ["Formal-Modeling", "Security-Threat-Model", "Integrity-and-Corruption"]
---

# Fuzzing and fault-injection plan

## Test layers

| Layer | Target | Engines/approach | Primary oracle |
|---|---|---|---|
| Parser | protobuf/JSON/frame decoders | libFuzzer + libprotobuf-mutator, AFL++, Go native fuzzing | no crash/OOM/hang; bounded allocation; deterministic validation |
| Semantic validator | context, manifests, topology, ranges | structured mutation and property-based generators | accept iff schema, authz, epoch, topology, and completeness rules hold |
| Stateful protocol | begin/prepare/commit/cancel/reconnect sequences | P schedules, reference model, Hypothesis/QuickCheck/proptest-style state machines | TLA/P invariants and terminal-state agreement |
| Concurrency | queues, credits, cancellation, duplicate connections | deterministic scheduler/loom-style tests, thread sanitizers | no deadlock, races, negative credits, double terminal transition |
| Network | loss, duplication, delay, reorder, reset, half-open, partition | proxy/netem/Chaos Mesh or equivalent | safety preserved; bounded retry; authoritative reconciliation |
| Crash consistency | process kill/power-loss at persistence boundaries | write/flush/rename/CAS fault hooks, CrashMonkey-style record/replay | old commit, new commit, or abort only; never mixed state |
| Corruption | bit flips, truncation, wrong object, stale version | storage shim and DMA/deserialize hooks | reject before use; quarantine; independent repair only |
| Resource abuse | huge counts/lengths, compression bombs, reconnect storms | adversarial corpus and load generator | hard limits, fair quotas, control-plane responsiveness |

## Parser and hostile-input corpus

Seed with every valid message and then mutate:

- zero, maximum, and over-maximum lengths;
- truncated varints/frames, duplicate fields, unknown fields/enums, invalid UTF-8;
- 15/16/17-byte IDs and 31/32/33-byte digests;
- repeated fields at `0`, `1`, limit, and limit+1;
- integer wraparound in `offset + length`, page counts, shapes, and byte totals;
- overlapping, unsorted, duplicate, and gapped chunk ranges;
- conflicting authenticated versus self-asserted tenant/rank;
- `op_id` reused with a different request digest;
- lower epoch, unjustified higher epoch, generation rollback, sequence rollback;
- object ID valid for bytes but wrong rank/coordinate/topology header;
- compressed input whose uncompressed declaration or actual ratio exceeds limits;
- arbitrary path/URL strings in every byte/string field to prove they are never dereferenced.

A valid-but-large message must be rejected before proportional allocation when over policy.

## Stateful operation grammar

Generate operations from:

```text
BumpEpoch | AcceptEpoch(rank) | Begin | Durable(rank) |
SendPrepared(rank) | Deliver(message) | Cancel | Commit |
Crash(rank) | Recover(rank) | Disconnect | Reconnect |
Corrupt(object) | Repair(object) | Query | TryMaterialize |
GrantCredit | ConsumeCredit | ExpireCredit
```

Bias toward race windows: second prepare versus cancel, certificate CAS versus deadline, epoch bump with queued old frames, duplicate reconnect, corruption after verification but before materialization, and crash between file flush, rename, directory flush, rank-manifest publish, and certificate CAS.

## Properties

- A committed operation has both ranks at one position/topology.
- No aborted operation later commits.
- Duplicate/reordered requests are observationally idempotent.
- Stale generation/epoch/instance never mutates current state.
- Ambiguous commit observation resolves to the authority’s single terminal value.
- Credits and reservations remain within `[0, configured_limit]` and are eventually released after terminal/cancel/connection loss.
- A corrupt or structurally invalid object is never passed to GPU materialization.
- Recovery fetch set is a subset of verified missing/corrupt objects plus bounded metadata.
- A single-node decision is `true` only when the complete-model and complete-state predicates are true.
- Logs/traces contain no marked secret or synthetic prompt payload fields.

## Crash campaign

Instrument fault points around every durable step:

```text
create temp -> each partial write -> file flush -> verify -> rename/link ->
directory flush -> rank manifest write/flush/publish -> Prepared send/record ->
global manifest write -> certificate CAS request/server commit/response -> notice
```

For each point, crash rank, coordinator, storage client, and—where supported—reboot the host/storage emulator. On restart enumerate visible objects and authority state. The only legal externally readable states are the prior committed checkpoint or the fully committed new checkpoint. Prepared/orphan objects may exist but remain unreachable from a certificate.

## Silent-corruption campaign

Inject one-bit and burst flips in frame headers, payloads, page headers, manifests, certificate bytes, local files, object-store versions, and memory after read. Also inject wrong-but-valid pages with matching length, swapped rank manifests, stale object versions, and a producer that hashes intentionally wrong tensors. Verify checksum layer attribution, quarantine, repair source selection, and semantic-canary detection limits.

## Differential and metamorphic testing

- Serialize/parse/serialize must produce canonical bytes or an equivalent canonical digest.
- Chunk boundaries, frame ordering with valid offsets, and retry segmentation must not change final object ID.
- Replacing a Bloom filter with a full inventory may change transfer planning but not final verified page set.
- Replaying the same operation trace through reference model, P model, and implementation must agree on terminal state and rejection reasons at the abstraction boundary.
- Rebuild from prompt + forced emitted tokens must match declared cache/logit tolerances for the same topology.

## CI cadence

- Per change: deterministic unit/property tests, short parser fuzz smoke, reference traces, schema/lint.
- Nightly: multi-hour coverage-guided campaigns, deterministic concurrency, network schedules, corruption sampling.
- Weekly/release: crash-point matrix across supported filesystems/stores, long stateful campaigns, sanitizer builds, replay determinism matrix, recovery-scale tests with multi-gigabyte logical caches.
- Before protocol release: reproduce all historical counterexamples and compare coverage/corpus deltas.

## Triage

Persist seed, corpus hash, build ID, topology descriptor, fault schedule, authority history, and minimized trace. Security-sensitive crashes or resource-exhaustion paths are handled under `SECURITY.md`. Every fixed state-machine bug becomes both a model trace and an implementation regression.
