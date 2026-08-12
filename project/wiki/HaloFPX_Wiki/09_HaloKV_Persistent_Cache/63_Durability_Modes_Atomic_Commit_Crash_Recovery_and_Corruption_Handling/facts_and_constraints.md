---
section_id: "63"
title: "Durability facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: []
related_sections: ["56", "59"]
---

# Facts and constraints

**[VERIFIED]** CachyLLama v3 stores `[record][target][draft][spec]`, handles short reads/writes, optionally calls `fsync`, and scans checkpoint files to reconstruct its in-memory index [S63-01]. It checks identifiers/header fields but lacks a cryptographic or CRC payload digest in `kv_ssd_record`.

**[VERIFIED]** Linux `fsync(file)` flushes file data/metadata but does not necessarily persist the directory entry; directory fsync is separately required [S63-02]. `rename()` replacement is atomic within a filesystem, but atomic namespace change is distinct from durable persistence [S63-03].

**[INFERENCE]** A scan can discover a torn final-path file after a crash. Without a payload digest, structurally plausible corruption may escape detection. HaloKV must not inherit this as trusted behavior.

## Live RPC tensor-cache warning

- **[VERIFIED]** At deployed commit `4860505e`, the RPC server documents `--cache` as a cache for large model tensors that avoids network retransmission; it is not an attention-KV/session cache [S63-L01].
- **[VERIFIED]** `set_tensor()` hashes incoming bytes with 64-bit FNV, writes directly to the hash-named final path, and then installs the tensor. `get_cached_file()` selects by requested hash but does not recompute the hash after reading [S63-L01].
- **[INFERENCE]** A same-sized corrupted file, or some shorter partial files, can be accepted by this inspected path because filename identity is trusted and no independent content digest is checked. The existing bounds check is necessary but not sufficient.
- **[RECOMMENDATION]** Any retained RPC-cache feature needs cryptographic content identity, exact-length checks, temp-write/flush/atomic-rename publication, read-time verification, quota/reserve/eviction, and corruption-as-miss semantics before promotion.

## Durability modes

| Mode | Acknowledgement promise |
|---|---|
| performance | checkpoint may be lost after process/host/power failure; never corruptly reused |
| turn-durable | completed turn manifest and required local data survive the tested host-crash model |
| strict | all required rank-local shards and coordinator commit survive the declared power-loss model before acknowledgement |

**[RECOMMENDATION]** The invariant common to all modes is validation, not persistence: recovery validates every manifest/object reference and rejects, quarantines, or recomputes an incomplete generation. Performance mode makes no post-failure survival promise for a recently visible generation. The stronger no-dangling-reference-after-acknowledgement guarantee belongs only to turn-durable/strict mode and only within the exact failure model demonstrated by M63-03.

**[ASSUMPTION]** Strict durability can be achieved with the selected filesystem/device flush path. This requires destructive power-cut testing on sacrificial cache data.
