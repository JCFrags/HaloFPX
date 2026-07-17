---
section_id: "21"
title: "Storage facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: ["Linux 7.2 documentation", "fio 3.41"]
  hardware_revisions: ["exact devices open"]
related_sections: ["18", "19", "22", "60", "65", "77"]
---

# Storage facts and constraints

## Known source-backed constraints

| ID | Claim | HaloFPX relevance |
|---|---|---|
| 21-F01 | **[VERIFIED]** Strix Halo exposes PCIe 4.0 and supports NVMe, but the APU product page does not establish an OEM slot's actual generation, lane width, sharing, or installed drive ([S21-07]). | Capture negotiated `LnkSta`, not advertised product capability. |
| 21-F02 | **[VERIFIED]** blk-mq maps block I/O through software staging queues to hardware dispatch queues; scheduling occurs within a queue ([S21-03]). | Queue depth and CPU/IRQ placement are experiment inputs. |
| 21-F03 | **[VERIFIED]** `/sys/block/<dev>/queue/` exposes scheduler, request limits, logical/physical block sizes, rotational status, write cache, and other queue attributes; individual attributes may be absent ([S21-04]). | Preserve the whole queue snapshot per node. |
| 21-F04 | **[VERIFIED]** NVMe `percentage_used` is a vendor-specific life estimate and may exceed 100; `data_units_written` is reported in units of 1,000 x 512 bytes ([S21-01], [S21-02]). | Compute deltas from raw counters and retain vendor datasheet TBW separately. |
| 21-F05 | **[VERIFIED]** fio can select direct/buffered I/O, queue depth, patterns, steady-state criteria, verification, and JSON output ([S21-05]). | A job file plus raw JSON and environment record is required for any `[MEASURED]` result. |
| 21-F06 | **[VERIFIED]** `fsync()` requests transfer of modified file data and metadata; an application also needs to sync the containing directory for a newly created directory entry ([S21-06]). | A cache commit protocol cannot equate `write()` completion with durability. |
| 21-F07 | **[INFERENCE]** Device capacitor protection, controller write-cache behavior, filesystem barriers, and application ordering jointly determine recovery after power loss. SMART `unsafe_shutdowns` alone cannot prove correctness. | Validate recovery at the application record/manifest layer. |

## Required per-node inventory

No cell may be filled from resemblance between the nodes.

| Field | Node A | Node B | Evidence |
|---|---|---|---|
| Controller / namespace model, serial | **[MEASURED]** Crucial P310 `CT1000P310SSD8`; serial retained outside Wiki | same model; distinct serial retained outside Wiki | S21-L01 `lsblk`/SMART |
| Firmware revision | **[MEASURED]** `VACR001` | **[MEASURED]** `VACR001` | S21-L01 SMART/sysfs |
| PCI address / negotiated generation x width | **[OPEN]** | **[OPEN]** | `lspci -Dvv` `LnkCap` and `LnkSta` |
| Capacity, LBA format, sector sizes | **[MEASURED]** 999,665,881,088-byte device/filesystem class; 512-byte logical/physical observed | same | S21-L01; full namespace identify still open |
| Filesystem, UUID, mount point/options | **[MEASURED]** Btrfs subvolumes, `noatime`, `compress=zstd:1`, `discard=async` | same policy class | S21-L01; UUIDs intentionally omitted |
| Queue count, scheduler, limits, cache | **[OPEN]** | **[OPEN]** | sysfs queue snapshot |
| Temperature thresholds / sensors | **[MEASURED]** 32 C idle snapshot; 82.8 C high / 84.8 C critical sensor report | **[MEASURED]** 30 C; same reported thresholds | S21-L01; load behavior open |
| TBW/DWPD warranty rating | **[OPEN]** | **[OPEN]** | exact vendor datasheet and warranty revision |
| SMART baseline | **[MEASURED]** 1% used, 6.10 TB written, 14 unsafe shutdowns, zero media/errors | **[MEASURED]** 0% used, 5.51 TB written, 17 unsafe shutdowns, zero media/errors | S21-L01; repeat before tests |
| Spare/unallocated capacity | **[MEASURED]** about 43 GiB filesystem free | **[MEASURED]** about 318 GiB filesystem free | S21-L01; not device over-provisioning |

## Immediate capacity finding

- **[MEASURED]** nimo-1's Btrfs data allocation was 98.88% used and its root filesystem 96% full by `df`; visible consumers included about 276 GiB of models, 112 GiB of RPC tensor cache, 157 GiB of home-directory models, and 49 GiB of user cache [S21-L01].
- **[MEASURED]** nimo-2's filesystem was 66% full, with about 166 GiB of models under the cluster directory and about 154 GiB of home-directory models [S21-L01].
- **[RECOMMENDATION]** Define a minimum free-space reserve and per-cache quota before enabling HaloKV. Do not delete or repurpose the deployed RPC cache while its running service uses it; establish its rebuild cost and a reversible cleanup plan first.

## Capacity and write-budget model

**[RECOMMENDATION]** Record every estimate with inputs; do not store a single context-size number as fact.

- Model bytes = sum of actual artifact file sizes, including tokenizer, metadata, and duplicate quantizations.
- Dense KV bytes per sequence = `2 * layers * tokens * kv_heads * head_dim * bytes_per_element`; architecture, quantization, padding, rank ownership, and implementation can change this.
- Cache reserve = committed entries + staging/dirty tail + index/manifest + compaction headroom + recovery copy.
- Daily host writes = application payload + metadata + compaction/GC + filesystem amplification.
- Estimated days to rated TBW = `(remaining_rated_bytes) / measured_daily_host_bytes`; **[INFERENCE]** this is planning math, not a failure prediction.

**[OPEN]** Target models, contexts, cache format, compaction policy, rank ownership, and exact drive TBW are not yet fixed, so no footprint or lifetime number is promoted here.

[S21-01]: sources.md#s21-01
[S21-02]: sources.md#s21-02
[S21-03]: sources.md#s21-03
[S21-04]: sources.md#s21-04
[S21-05]: sources.md#s21-05
[S21-06]: sources.md#s21-06
[S21-07]: sources.md#s21-07
