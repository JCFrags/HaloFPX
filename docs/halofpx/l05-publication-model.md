# L05 P63-00 publication-model gate

Status: final candidate pending independent adversarial acceptance. Persistent
writes, server integration, and canary use remain closed.

## Decision boundary

The target-owned model implements accepted ADR-0004. Recovery validates only
the exact protected anchor-selected identity; it never selects a generation by
directory enumeration. Identity binds lineage, generation, manifest digest,
predecessor digest, policy epoch, key generation, and writer-authority epoch.
Every bounded predecessor must remain complete and valid.

The model separates object and manifest write/verify/file-sync/no-replace
publication/directory-sync steps; exact anchor replacement and sync;
acknowledgement; crash-old/crash-new outcomes; writer restart/transfer and
attempt fencing; corruption/removal; fingerprint, digest, predecessor, and
cross-lineage replay faults; fail-closed rejection; recomputation; and
abandonment. Anchor replacement is serialized across lineages to represent the
single writer per publication root.

The model is offline verification material. It is linked to no product target
and provides no filesystem writer, provider hook, cache hit, or server option.

## Pinned tools and evidence

TLC uses TLA+ Tools `v1.7.4`, revision `5a47802`, JAR SHA-256
`936a262061c914694dfd669a543be24573c45d5aa0ff20a8b96b23d01e050e88`.
The secondary checker is Apalache `v0.57.0`, build `635865a`, archive SHA-256
`61c3eb4d694cc7dcd0bcb52f98cf187c9390576b967f8c1639fcea91cc4ac412`,
checker-JAR SHA-256
`1c2500ec2b014fcf41a7b0bd4c30fc3204b69377028fd689224eea9cf23f66f5`.
Both used Temurin OpenJDK `25.0.2+10-LTS`. The tools remain outside the engine.

Promoted TLC evidence:

```text
C:\Users\britt\Documents\Custom_Inference_Project\experiments\P63-00\runs\2026-07-18-l05-model-final-v05
```

- source SHA-256: `320d294949624469a5c636fc510300f6f558845094139402a6845d765b1c38fe`
- manifest SHA-256: `35069f6bfe387aa15218a8c3040e2dac678030cb7268346be3687bb08058ca7a`
- retained: 160 files / 126,359,264 bytes
- declared and verified artifacts: 159; the manifest is the 160th file
- exact commands, arguments, seeds, configs, source/runtime identity, timings,
  stdout/stderr, and every TLC state file carry length and SHA-256 records

Promoted Apalache evidence:

```text
C:\Users\britt\Documents\Custom_Inference_Project\experiments\P63-00\runs\2026-07-18-l05-apalache-v05
```

- same source SHA-256 as TLC
- manifest SHA-256: `cf57171d9cb1df719c9f07a5093cdfab5aa68c43c877d673d3d030c239bc17d6`
- retained: 10 files / 143,880 bytes; 9 declared artifacts plus manifest
- full typecheck passed; independent bounded `Safety` check passed through
  computation length 5

## TLC results

| Configuration | Repeats | Expected result | Generated / distinct per run | Depth |
|---|---:|---|---:|---:|
| two-rank crash/authority safety | 3 | exhaustive pass | 14,797,502 / 1,979,280 | 36 |
| generation 0 -> 1 -> 2 chain | 3 | exhaustive pass | 1,381 / 422 | 36 |
| two-lineage interleaving | 3 | exhaustive pass | 37,745 / 7,380 | 37 |
| reduced single-worker liveness | 3 | temporal pass | 221 / 86 | 19 |
| premature acknowledgement mutation | 1 | required violation | 991 / 325 | 21 |
| mixed-generation recovery mutation | 1 | required violation | 337 / 142 | 9 |
| newest-unanchored recovery mutation | 1 | required violation | 161 / 69 | 14 |
| digest replay recovery mutation | 1 | required violation | 27,210 / 5,936 | 17 |
| cross-lineage anchor replay mutation | 1 | required violation | 230 / 152 | 3 |

All 17 outcomes matched their contracts: 44,539,476 generated states,
5,968,128 distinct states, and 82,606 ms of TLC process time. Each negative
returned exit 12 with a retained counterexample. The digest replay trace
explicitly reaches `MutateManifestIdentity` before the broken recovery action.

TLC action coverage records nonzero transition evaluations for pre-existing
equal-object/equal-manifest verification and unequal-collision rejection. A
verified-existing transition can generate a state already reachable through
normal no-replace publication, so its distinct-state contribution may be zero;
the retained evaluation count proves the path is enabled and explored.

The fault-enabled two-lineage Cartesian product was retained as an unpromoted
development run after exceeding the bounded execution window. The exhaustive
two-lineage suite therefore claims interleaving/local identity isolation only;
fault behavior is exhaustively covered by the bounded single-lineage crash
suite and cross-lineage replay by its deliberate negative configuration.

## Gate interpretation and limits

Independent acceptance of P63-00 opens only implementation of a disabled,
offline, target-native writer and fault harness. It does not enable persistent
writes, server lookup/hits, deployment, or canaries. Crash-point tests at every
publication boundary; ENOSPC, EDQUOT, EIO, read-only and sync-failure injection;
quota/reserve/eviction; observability; administrative controls; rollback; and
M63-01..03 remain L05 exit and promotion gates for that harness.

The bounded pass does not prove C++ conformance, cryptography, path safety,
actual filesystem atomicity/synchronization, device durability, performance,
or power-loss resistance. Those claims require target-native tests and retained
machine evidence.
