# Expected SSD endurance

<span class="badge inference">MODELED</span> This section provides formulas and illustrative scenarios, not a warranty prediction for a specific drive.

## KV bytes per token

For a dense transformer with separate K and V tensors:

```text
KV_bytes_per_token = 2 × layers × kv_heads × head_dim × bytes_per_element
```

Illustrative Llama-like grouped-query configuration:

```text
layers = 32
kv_heads = 8
head_dim = 128
bytes_per_element = 2   # fp16/bf16
KV_bytes_per_token = 131,072 bytes = 128 KiB
```

Approximate target-state payload:

| Tokens | Approximate KV bytes | Binary size |
|---:|---:|---:|
| 4,096 | 536,870,912 | 0.5 GiB |
| 8,192 | 1,073,741,824 | 1.0 GiB |
| 16,384 | 2,147,483,648 | 2.0 GiB |
| 32,768 | 4,294,967,296 | 4.0 GiB |
| 131,072 | 17,179,869,184 | 16.0 GiB |

Actual serialized llama.cpp state can differ because of cache layout, padding, quantized K/V types, recurrent state, allocator metadata, context settings, draft/MTP state and speculative blobs. The formula is an order-of-magnitude model, not a file-size parser.

## CachyLLama write pattern

<span class="badge observed">OBSERVED</span> Each CachyLLama checkpoint writes a complete serialized target sequence state and optional complete draft/speculative blobs to a new checkpoint file; it is not a delta against a prior checkpoint. `index.bin` is small by comparison but is rewritten after stores/shutdown paths. Cold eviction is count-based, so a configured count does not cap total host bytes.

For a 4 GiB state:

| Full checkpoints per day | Host writes/day | Approx. decimal TB/year |
|---:|---:|---:|
| 10 | 40 GiB | 15.7 TB |
| 20 | 80 GiB | 31.4 TB |
| 50 | 200 GiB | 78.4 TB |
| 100 | 400 GiB | 156.8 TB |
| 200 | 800 GiB | 313.5 TB |

The conversion above uses 1 GiB = 2^30 bytes and 1 TB = 10^12 bytes.

## Lifetime model

Let:

```text
host_TB_per_day = bytes_written_per_day / 10^12
nand_TB_per_day = host_TB_per_day × write_amplification_factor
estimated_years = rated_TBW / (nand_TB_per_day × 365)
```

`rated_TBW` is an input from the chosen drive specification. `write_amplification_factor` (WAF) depends on workload, free space, firmware, TRIM, block size, overwrite pattern and device class. The included calculator requires the operator to supply or model it.

### Illustrative scenarios

These are scenario inputs, not claims that a given capacity/class has a particular rating.

| Scenario | State GiB | Checkpoints/day | Modeled rated TBW | WAF | Host TB/year | Estimated years |
|---|---:|---:|---:|---:|---:|---:|
| Light | 4 | 10 | 600 | 1.5 | 15.7 | 25.5 |
| Moderate | 4 | 50 | 1,200 | 1.5 | 78.4 | 10.2 |
| Heavy | 4 | 200 | 2,400 | 1.7 | 313.5 | 4.5 |
| Long-context heavy | 16 | 100 | 2,400 | 2.0 | 627.0 | 1.9 |

The exact generated values are produced by [`validation/endurance_model.py`](../validation/endurance_model.py) from [`tables/endurance-scenarios.csv`](../tables/endurance-scenarios.csv). Small differences may result from decimal/binary conversion and rounding.

## Write amplification beyond payload size

Host-visible state bytes are the floor, not necessarily NAND writes. Additional factors:

- filesystem metadata, journal and copy-on-write behavior;
- direct truncation/rewrite of index/system files;
- SSD internal garbage collection and erase-block relocation;
- low free-space conditions;
- temporary-file protocols, which write once and rename rather than write twice if implemented correctly;
- encryption/compression framing;
- validation rereads, which affect read endurance/performance but not TBW materially;
- migration or re-encryption copies;
- replicas.

Atomic publication does not require double-writing the payload: write one temporary inode, sync it, and rename the same inode into the immutable object name.

## HaloFPX endurance controls

1. **Chunk/page persistence instead of repeated full snapshots.** Store immutable verified chunks and publish small manifests; unchanged prefix chunks deduplicate.
2. **Write admission.** Persist only when expected saved prefill cost and reuse probability exceed write/storage cost.
3. **Checkpoint coalescing.** Enforce minimum token/time/turn distance and avoid multiple snapshots of rapidly changing tails.
4. **Byte quotas, not count quotas.** Enforce global and tenant bytes plus a minimum free-space watermark.
5. **Delta metadata, immutable data.** Update small manifests/access journals rather than rewrite multi-GiB objects.
6. **Avoid access-time write storms.** Keep recency in an append/coalesced database or periodic checkpoint; do not rewrite every object on hit.
7. **Compression only when validated and beneficial.** KV tensors may compress poorly; measure CPU/I/O tradeoff and cap decompression.
8. **Reserve free space and issue TRIM/discard according to platform policy.** Low spare area can increase WAF.
9. **Monitor actual host writes and drive health.** Compare service bytes-written metrics with drive telemetry and rated TBW.
10. **Budget migration/rotation.** Re-encryption and format migration can equal a full cache rewrite; stage it under a daily byte budget.

## Capacity planning equations

```text
checkpoint_bytes = target_bytes + draft_bytes + spec_bytes + envelope_bytes
host_bytes/day = checkpoints/day × checkpoint_bytes × replicas
annual_host_TB = host_bytes/day × 365 / 10^12
annual_nand_TB = annual_host_TB × modeled_WAF
endurance_margin = rated_TBW / planned_service_years / annual_nand_TB
```

An `endurance_margin < 1` means the modeled write budget exceeds the drive's rated TBW over the planned life. Production targets should retain additional margin for workload spikes, migration and uncertain WAF.
