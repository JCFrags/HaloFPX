# Qwen model deletion receipt

Timestamp: 2026-07-17 19:26 PDT

Authorization: the user explicitly requested deletion of all downloaded Qwen models from `nimo-1` and `nimo-2`.

## Resolved scope before deletion

| Node | Resolved directory | Files | Apparent bytes |
|---|---|---:|---:|
| `nimo-1` | `/opt/llm-usb4-cluster/models/qwen-official` | 18 | 56,256,327,870 |
| `nimo-1` | `/opt/llm-usb4-cluster/models/qwen-quality-reference` | not captured | 32,483,960,073 |
| `nimo-2` | `/opt/llm-usb4-cluster/models/qwen-official` | 18 | 56,256,327,870 |

The scope intentionally excluded llama.cpp source-tree tokenizer fixtures such as
`ggml-vocab-qwen*.gguf`; they are test data, not downloaded inference models.

## Verification after deletion

**[MEASURED]** All three exact directories were absent. A case-insensitive filename
search below `/opt/llm-usb4-cluster/models` returned no remaining Qwen files on
either node.

| Node | Available bytes before | Available bytes after | Filesystem-observed change |
|---|---:|---:|---:|
| `nimo-1` | 45,268,774,912 | 133,994,205,184 | +88,725,430,272 bytes (82.63 GiB) |
| `nimo-2` | 340,131,090,432 | 340,131,090,432 | 0 bytes |

The nimo-2 directory entries were deleted and verified absent, but the root
filesystem did not report additional available blocks. Therefore no reclaimed
capacity is claimed for nimo-2.

Recovery: permanent deletion; the model weights can only be restored from
another retained copy or by downloading them again.

