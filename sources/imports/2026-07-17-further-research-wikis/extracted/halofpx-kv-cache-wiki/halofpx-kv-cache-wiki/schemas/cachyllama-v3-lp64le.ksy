meta:
  id: cachyllama_kv_ssd_record_v3_lp64le
  title: CachyLLama kv_ssd_record v3 (explicit conventional LP64 little-endian interpretation)
  endian: le
  license: CC0-1.0
  ks-version: 0.10
  doc: >
    Analysis schema for the native C struct written by CachyLLama commit
    6be745998f568e379ea197fcf827baec73ff9940. Upstream does not define a
    canonical packed wire format. Validate sizeof/offsetof on the producing ABI.
seq:
  - id: magic
    type: u4
    valid: 0x4b565243
  - id: version
    type: u4
    valid: 3
  - id: checkpoint_id
    type: u8
  - id: slot_id
    type: u4
  - id: pos_min
    type: s4
  - id: pos_max
    type: s4
  - id: abi_pad_after_pos_max
    size: 4
  - id: n_tokens
    type: u8
  - id: turn_created
    type: u4
  - id: abi_pad_after_turn_created
    size: 4
  - id: target_data_size
    type: u8
  - id: token_hash_fnv1a64
    type: u8
  - id: token_count
    type: u4
  - id: abi_pad_before_compat_hash
    size: 4
  - id: compat_hash_fnv1a64
    type: u8
  - id: token_prefix
    type: u4
    repeat: expr
    repeat-expr: 4096
  - id: draft_data_size
    type: u8
  - id: speculative_data_size
    type: u8
  - id: target_data
    size: target_data_size
  - id: draft_data
    size: draft_data_size
  - id: speculative_data
    size: speculative_data_size
