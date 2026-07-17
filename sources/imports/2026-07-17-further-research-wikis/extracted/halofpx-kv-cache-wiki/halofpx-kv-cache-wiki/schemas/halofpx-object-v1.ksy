meta:
  id: halofpx_kv_object_v1
  title: HaloFPX immutable KV object envelope v1
  endian: le
  license: CC0-1.0
  ks-version: 0.10
seq:
  - id: magic
    contents: [0x48, 0x46, 0x50, 0x58, 0x4b, 0x56, 0x43, 0x31]
  - id: major
    type: u2
    valid: 1
  - id: minor
    type: u2
  - id: header_len
    type: u4
    valid: 104
  - id: metadata_len
    type: u8
  - id: payload_len
    type: u8
  - id: segment_count
    type: u4
  - id: flags
    type: u4
  - id: metadata_sha256
    size: 32
  - id: payload_sha256
    size: 32
  - id: metadata_canonical_json
    size: metadata_len
    encoding: UTF-8
  - id: payload
    size: payload_len
