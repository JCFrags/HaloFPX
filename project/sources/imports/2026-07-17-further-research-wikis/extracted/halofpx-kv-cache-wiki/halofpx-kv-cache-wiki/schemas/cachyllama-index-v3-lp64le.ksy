meta:
  id: cachyllama_kv_ssd_index_v3_lp64le
  title: CachyLLama kv_ssd_index_header v3
  endian: le
  license: CC0-1.0
  ks-version: 0.10
seq:
  - id: magic
    type: u4
    valid: 0x4b564944
  - id: version
    type: u4
    valid: 3
  - id: next_id
    type: u8
  - id: compat_hash_fnv1a64
    type: u8
  - id: reserved
    type: u8
    repeat: expr
    repeat-expr: 12
