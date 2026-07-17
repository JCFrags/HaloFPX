meta:
  id: cachyllama_system_prompt_record_v1_lp64le
  title: CachyLLama kv_ssd_system_record v1
  endian: le
  license: CC0-1.0
  ks-version: 0.10
seq:
  - id: magic
    type: u4
    valid: 0x4b565359
  - id: version
    type: u4
    valid: 1
  - id: token_hash_fnv1a64
    type: u8
  - id: n_tokens
    type: u4
  - id: data_size
    type: u4
  - id: compat_hash_fnv1a64
    type: u8
  - id: created_at_unix_s
    type: u8
  - id: last_used_unix_s
    type: u8
  - id: access_count
    type: u4
  - id: token_count
    type: u4
  - id: token_prefix
    type: u4
    repeat: expr
    repeat-expr: 4096
  - id: state_data
    size: data_size
