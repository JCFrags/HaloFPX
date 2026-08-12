#!/usr/bin/env python3
"""Deterministically generate PF-IR-10 self-authored conformance assets.

SPDX-License-Identifier: CC0-1.0

The implementation is intentionally Python-stdlib-only and defines all binary
bytes, tensor values, record ordering, and malformed mutations explicitly.
It does not invoke any candidate repository or binary.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Sequence

GGUF_MAGIC = b"GGUF"
GGUF_VERSION = 3
DEFAULT_ALIGNMENT = 32

# gguf_metadata_value_type
U8, I8, U16, I16, U32, I32, F32, BOOL, STRING, ARRAY, U64, I64, F64 = range(13)
# ggml_type
GGML_F32 = 0
GGML_TURBO4_0_ROCMFPX = 106

SPECIAL_TOKENS = [
    "<unk>",
    "<s>",
    "</s>",
    "<|eot_id|>",
    "<|pad|>",
    "<|tool_call|>",
    "<|end_tool_call|>",
]
BYTE_TOKEN_BASE = len(SPECIAL_TOKENS)

DEFAULT_TEMPLATE = """{%- for message in messages -%}<|{{ message['role'] }}|>\n{{ message['content'] | default('') }}\n{%- endfor -%}{%- if add_generation_prompt -%}<|assistant|>\n{%- endif -%}"""
STRICT_TEMPLATE = """{%- for message in messages -%}{%- if message['role'] not in ['system','user','assistant','tool'] -%}{{ raise_exception('unsupported role') }}{%- endif -%}[{{ message['role'] }}]{{ message['content'] | default('') }}\n{%- endfor -%}{%- if add_generation_prompt -%}[assistant]{%- endif -%}"""
TOOLS_TEMPLATE = """{%- if tools -%}<tools>{{ tools | tojson }}</tools>\n{%- endif -%}{%- for message in messages -%}<{{ message['role'] }}>{{ message['content'] | default('') }}{%- if message.get('tool_calls') -%}{{ message['tool_calls'] | tojson }}{%- endif -%}</{{ message['role'] }}>\n{%- endfor -%}{%- if add_generation_prompt -%}<assistant>{%- endif -%}"""


def canonical_json(obj: Any) -> str:
    return json.dumps(obj, ensure_ascii=False, sort_keys=True, separators=(",", ":"))


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def write_bytes(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8", newline="\n")


def write_json(path: Path, obj: Any) -> None:
    write_text(path, json.dumps(obj, ensure_ascii=False, sort_keys=True, indent=2) + "\n")


def write_jsonl(path: Path, rows: Iterable[dict[str, Any]]) -> None:
    write_text(path, "".join(canonical_json(row) + "\n" for row in rows))


def p_u32(v: int) -> bytes:
    return struct.pack("<I", v)


def p_i32(v: int) -> bytes:
    return struct.pack("<i", v)


def p_u64(v: int) -> bytes:
    return struct.pack("<Q", v)


def p_i64(v: int) -> bytes:
    return struct.pack("<q", v)


def p_f32(v: float) -> bytes:
    return struct.pack("<f", v)


def p_f64(v: float) -> bytes:
    return struct.pack("<d", v)


def gguf_string(value: str | bytes) -> bytes:
    raw = value.encode("utf-8") if isinstance(value, str) else value
    return p_u64(len(raw)) + raw


def scalar_bytes(value_type: int, value: Any) -> bytes:
    if value_type == U8:
        return struct.pack("<B", int(value))
    if value_type == I8:
        return struct.pack("<b", int(value))
    if value_type == U16:
        return struct.pack("<H", int(value))
    if value_type == I16:
        return struct.pack("<h", int(value))
    if value_type == U32:
        return p_u32(int(value))
    if value_type == I32:
        return p_i32(int(value))
    if value_type == F32:
        return p_f32(float(value))
    if value_type == BOOL:
        return struct.pack("<B", 1 if value else 0)
    if value_type == STRING:
        return gguf_string(value)
    if value_type == U64:
        return p_u64(int(value))
    if value_type == I64:
        return p_i64(int(value))
    if value_type == F64:
        return p_f64(float(value))
    raise ValueError(f"unsupported scalar type {value_type}")


@dataclass(frozen=True)
class Meta:
    key: str
    value_type: int
    value: Any
    array_type: int | None = None


@dataclass(frozen=True)
class Tensor:
    name: str
    dims: tuple[int, ...]
    ggml_type: int
    data: bytes


@dataclass
class BuildResult:
    data: bytes
    locators: dict[str, Any]


def encode_meta(meta: Meta) -> bytes:
    out = bytearray(gguf_string(meta.key))
    out += p_u32(meta.value_type)
    if meta.value_type == ARRAY:
        if meta.array_type is None:
            raise ValueError(f"array metadata {meta.key} lacks array_type")
        out += p_u32(meta.array_type)
        out += p_u64(len(meta.value))
        for item in meta.value:
            out += scalar_bytes(meta.array_type, item)
    else:
        out += scalar_bytes(meta.value_type, meta.value)
    return bytes(out)


def align_up(value: int, alignment: int) -> int:
    return value + ((alignment - value % alignment) % alignment)


def build_gguf(metadata: Sequence[Meta], tensors: Sequence[Tensor], alignment: int = DEFAULT_ALIGNMENT) -> BuildResult:
    if alignment < 8 or alignment % 8:
        raise ValueError("GGUF alignment must be a multiple of 8")

    header = bytearray()
    header += GGUF_MAGIC
    header += p_u32(GGUF_VERSION)
    header += p_u64(len(tensors))
    header += p_u64(len(metadata))

    locators: dict[str, Any] = {
        "header": {"start": 0, "end": len(header)},
        "metadata": {},
        "tensor_info": {},
        "tensor_data": {},
    }

    for meta in metadata:
        start = len(header)
        enc = encode_meta(meta)
        header += enc
        locators["metadata"][meta.key] = {"start": start, "end": len(header), "length": len(enc)}

    tensor_infos = bytearray()
    tensor_offset = 0
    for tensor in tensors:
        if not tensor.dims or len(tensor.dims) > 4:
            raise ValueError(f"invalid dimensions for {tensor.name}: {tensor.dims}")
        info_start = len(header) + len(tensor_infos)
        tensor_infos += gguf_string(tensor.name)
        tensor_infos += p_u32(len(tensor.dims))
        for dim in tensor.dims:
            tensor_infos += p_u64(dim)
        tensor_infos += p_u32(tensor.ggml_type)
        tensor_infos += p_u64(tensor_offset)
        locators["tensor_info"][tensor.name] = {
            "start": info_start,
            "end": len(header) + len(tensor_infos),
            "relative_offset": tensor_offset,
            "offset_field_start": len(header) + len(tensor_infos) - 8,
        }
        tensor_offset = align_up(tensor_offset + len(tensor.data), alignment)

    prefix = header + tensor_infos
    data_start = align_up(len(prefix), alignment)
    prefix += b"\x00" * (data_start - len(prefix))
    locators["tensor_data_start"] = data_start

    body = bytearray()
    for tensor in tensors:
        relative_start = len(body)
        if relative_start % alignment:
            body += b"\x00" * (align_up(relative_start, alignment) - relative_start)
            relative_start = len(body)
        body += tensor.data
        locators["tensor_data"][tensor.name] = {
            "start": data_start + relative_start,
            "end": data_start + len(body),
            "length": len(tensor.data),
        }
        if len(body) % alignment:
            body += b"\x00" * (align_up(len(body), alignment) - len(body))

    return BuildResult(bytes(prefix + body), locators)


def gpt2_bytes_to_unicode() -> dict[int, str]:
    bs = list(range(ord("!"), ord("~") + 1))
    bs += list(range(ord("¡"), ord("¬") + 1))
    bs += list(range(ord("®"), ord("ÿ") + 1))
    cs = list(bs)
    n = 0
    original = set(bs)
    for b in range(256):
        if b not in original:
            bs.append(b)
            cs.append(256 + n)
            n += 1
    return dict(zip(bs, (chr(c) for c in cs), strict=True))


def tokenizer_metadata(include_chat: bool = True) -> list[Meta]:
    byte_map = gpt2_bytes_to_unicode()
    tokens = SPECIAL_TOKENS + [byte_map[b] for b in range(256)]
    token_types = [2, 3, 3, 3, 3, 4, 4] + [1] * 256
    metadata = [
        Meta("tokenizer.ggml.model", STRING, "gpt2"),
        Meta("tokenizer.ggml.pre", STRING, "gpt-2"),
        Meta("tokenizer.ggml.tokens", ARRAY, tokens, STRING),
        Meta("tokenizer.ggml.token_type", ARRAY, token_types, I32),
        Meta("tokenizer.ggml.merges", ARRAY, [], STRING),
        Meta("tokenizer.ggml.bos_token_id", U32, 1),
        Meta("tokenizer.ggml.eos_token_id", U32, 2),
        Meta("tokenizer.ggml.eot_token_id", U32, 3),
        Meta("tokenizer.ggml.unknown_token_id", U32, 0),
        Meta("tokenizer.ggml.padding_token_id", U32, 4),
        Meta("tokenizer.ggml.add_bos_token", BOOL, True),
        Meta("tokenizer.ggml.add_eos_token", BOOL, False),
        Meta("tokenizer.ggml.add_space_prefix", BOOL, False),
    ]
    if include_chat:
        metadata += [
            Meta("tokenizer.chat_template", STRING, DEFAULT_TEMPLATE),
            Meta("tokenizer.chat_template.strict", STRING, STRICT_TEMPLATE),
            Meta("tokenizer.chat_template.tools", STRING, TOOLS_TEMPLATE),
        ]
    return metadata


def general_metadata(name: str, architecture: str, description: str) -> list[Meta]:
    return [
        Meta("general.type", STRING, "model"),
        Meta("general.architecture", STRING, architecture),
        Meta("general.name", STRING, name),
        Meta("general.version", STRING, "1.0.0"),
        Meta("general.description", STRING, description),
        Meta("general.alignment", U32, DEFAULT_ALIGNMENT),
        Meta("general.file_type", U32, 0),
        Meta("general.license", STRING, "CC0-1.0"),
        Meta("general.source.repo_url", STRING, "urn:pf-ir-10:self-generated"),
        Meta("pfir10.recipe", STRING, "recipes/generate_assets.py"),
        Meta("pfir10.deterministic", BOOL, True),
    ]


def splitmix64(state: int) -> tuple[int, int]:
    state = (state + 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF
    z = state
    z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & 0xFFFFFFFFFFFFFFFF
    z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & 0xFFFFFFFFFFFFFFFF
    z ^= z >> 31
    return state, z & 0xFFFFFFFFFFFFFFFF


def exact_weight_bytes(name: str, count: int, norm: bool = False) -> bytes:
    if norm:
        return b"".join(p_f32(1.0) for _ in range(count))
    seed = int.from_bytes(hashlib.sha256(("PF-IR-10:" + name).encode()).digest()[:8], "little")
    palette = (-0.0625, -0.03125, -0.015625, 0.0, 0.015625, 0.03125, 0.0625)
    out = bytearray()
    state = seed
    for _ in range(count):
        state, value = splitmix64(state)
        out += p_f32(palette[value % len(palette)])
    return bytes(out)


def tensor_f32(name: str, dims: Sequence[int], norm: bool = False) -> Tensor:
    count = math.prod(dims)
    return Tensor(name, tuple(int(d) for d in dims), GGML_F32, exact_weight_bytes(name, count, norm))


def generate_ggufs(root: Path) -> dict[str, Any]:
    out: dict[str, Any] = {}
    gguf_dir = root / "fixtures" / "gguf"

    # 1. Generic container/tensor probe. It is a valid GGUF container but not a runnable model.
    tensor_values = [0.0, 1.0, -1.0, 0.5]
    generic = build_gguf(
        general_metadata("PFIR10 Tiny Tensor", "pfir10probe", "One 2x2 F32 tensor for GGUF parser conformance")
        + [Meta("pfir10.tensor_values_hex", STRING, "000000000000803f000080bf0000003f")],
        [Tensor("probe.weight", (2, 2), GGML_F32, b"".join(p_f32(v) for v in tensor_values))],
    )
    generic_path = gguf_dir / "pfir10-tiny-tensor-v3.gguf"
    write_bytes(generic_path, generic.data)
    write_json(gguf_dir / "pfir10-tiny-tensor-v3.locators.json", generic.locators)
    out["generic"] = {"path": str(generic_path.relative_to(root)), "sha256": sha256_bytes(generic.data), "locators": generic.locators}

    # 2. Vocab-only tokenizer and default/named chat templates.
    vocab = build_gguf(
        general_metadata("PFIR10 Byte BPE + Chat", "llama", "Vocab-only byte BPE with default and named chat templates")
        + [Meta("llama.vocab_size", U32, BYTE_TOKEN_BASE + 256)]
        + tokenizer_metadata(include_chat=True),
        [],
    )
    vocab_path = gguf_dir / "pfir10-byte-bpe-chat-v3.gguf"
    write_bytes(vocab_path, vocab.data)
    write_json(gguf_dir / "pfir10-byte-bpe-chat-v3.locators.json", vocab.locators)
    out["vocab"] = {"path": str(vocab_path.relative_to(root)), "sha256": sha256_bytes(vocab.data), "locators": vocab.locators}

    # 3. Complete tiny 1-layer F32 Llama-shaped model. Candidate execution remains unqualified.
    n_vocab, n_ctx, n_embd, n_ff, n_head, n_head_kv, n_layer = BYTE_TOKEN_BASE + 256, 32, 8, 16, 2, 2, 1
    model_meta = general_metadata("PFIR10 Tiny Llama F32", "llama", "Self-generated 1-layer Llama-shaped model for semantic qualification") + [
        Meta("llama.vocab_size", U32, n_vocab),
        Meta("llama.context_length", U32, n_ctx),
        Meta("llama.embedding_length", U32, n_embd),
        Meta("llama.block_count", U32, n_layer),
        Meta("llama.feed_forward_length", U32, n_ff),
        Meta("llama.rope.dimension_count", U32, n_embd // n_head),
        Meta("llama.attention.head_count", U32, n_head),
        Meta("llama.attention.head_count_kv", U32, n_head_kv),
        Meta("llama.attention.layer_norm_rms_epsilon", F32, 1.0e-5),
    ] + tokenizer_metadata(include_chat=True)
    tensors = [
        tensor_f32("token_embd.weight", (n_embd, n_vocab)),
        tensor_f32("output_norm.weight", (n_embd,), norm=True),
        tensor_f32("output.weight", (n_embd, n_vocab)),
        tensor_f32("blk.0.attn_norm.weight", (n_embd,), norm=True),
        tensor_f32("blk.0.attn_q.weight", (n_embd, n_embd)),
        tensor_f32("blk.0.attn_k.weight", (n_embd, n_embd)),
        tensor_f32("blk.0.attn_v.weight", (n_embd, n_embd)),
        tensor_f32("blk.0.attn_output.weight", (n_embd, n_embd)),
        tensor_f32("blk.0.ffn_norm.weight", (n_embd,), norm=True),
        tensor_f32("blk.0.ffn_gate.weight", (n_embd, n_ff)),
        tensor_f32("blk.0.ffn_up.weight", (n_embd, n_ff)),
        tensor_f32("blk.0.ffn_down.weight", (n_ff, n_embd)),
    ]
    model = build_gguf(model_meta, tensors)
    model_path = gguf_dir / "pfir10-tiny-llama-f32-v3.gguf"
    write_bytes(model_path, model.data)
    write_json(gguf_dir / "pfir10-tiny-llama-f32-v3.locators.json", model.locators)
    out["model"] = {"path": str(model_path.relative_to(root)), "sha256": sha256_bytes(model.data), "locators": model.locators}

    # 4. ROCmFPX type-ID probe. 32 logical values, 18 zero bytes (4.5 bpw) for type 106.
    # This is intentionally marked qualification-required; it is not asserted to be a valid TurboQuant encoding.
    rocm_probe = build_gguf(
        general_metadata("PFIR10 ROCmFPX Type Probe", "pfir10probe", "Header/type dispatch probe for ROCmFPX GGML_TYPE_TURBO4_0=106")
        + [Meta("pfir10.source_type_id", U32, GGML_TURBO4_0_ROCMFPX), Meta("pfir10.qualification_required", BOOL, True)],
        [Tensor("turbo4.probe", (32,), GGML_TURBO4_0_ROCMFPX, bytes(18))],
    )
    rocm_path = root / "fixtures" / "fork-specific" / "rocmfpx-turbo4-type106-probe.gguf"
    write_bytes(rocm_path, rocm_probe.data)
    write_json(root / "fixtures" / "fork-specific" / "rocmfpx-turbo4-type106-probe.locators.json", rocm_probe.locators)
    out["rocm_probe"] = {"path": str(rocm_path.relative_to(root)), "sha256": sha256_bytes(rocm_probe.data), "locators": rocm_probe.locators}

    # Deterministic malformed GGUF derivatives.
    malformed_dir = gguf_dir / "malformed"
    base = bytearray(generic.data)
    mutations: dict[str, bytes] = {}
    bad_magic = bytearray(base); bad_magic[0:4] = b"XGUF"; mutations["bad-magic.gguf"] = bytes(bad_magic)
    mutations["truncated-header.gguf"] = bytes(base[:12])
    first_meta = next(iter(generic.locators["metadata"].values()))
    mutations["truncated-metadata.gguf"] = bytes(base[: first_meta["start"] + max(1, first_meta["length"] // 2)])
    tensor_end = generic.locators["tensor_data"]["probe.weight"]["end"]
    mutations["truncated-tensor-data.gguf"] = bytes(base[: tensor_end - 4])
    bad_offset = bytearray(base)
    offset_field = generic.locators["tensor_info"]["probe.weight"]["offset_field_start"]
    bad_offset[offset_field:offset_field + 8] = p_u64(1)
    mutations["misaligned-tensor-offset.gguf"] = bytes(bad_offset)
    first_type_offset = first_meta["start"] + 8 + len("general.type".encode("utf-8"))
    unknown_type = bytearray(base); unknown_type[first_type_offset:first_type_offset + 4] = p_u32(255); mutations["unknown-metadata-type.gguf"] = bytes(unknown_type)
    for name, data in mutations.items():
        write_bytes(malformed_dir / name, data)
    write_json(malformed_dir / "mutations.json", {
        "base": out["generic"],
        "mutations": [
            {"file": name, "sha256": sha256_bytes(data), "length": len(data)} for name, data in sorted(mutations.items())
        ],
    })
    out["malformed"] = {name: {"path": str((malformed_dir / name).relative_to(root)), "sha256": sha256_bytes(data)} for name, data in mutations.items()}
    return out


def byte_ids(raw: bytes) -> list[int]:
    return [BYTE_TOKEN_BASE + b for b in raw]


def special_parse_ids(raw: bytes, parse_special: bool) -> list[int]:
    if not parse_special:
        return byte_ids(raw)
    markers = sorted(((token.encode("utf-8"), idx) for idx, token in enumerate(SPECIAL_TOKENS)), key=lambda x: (-len(x[0]), x[1]))
    out: list[int] = []
    pos = 0
    while pos < len(raw):
        match = next(((marker, idx) for marker, idx in markers if raw.startswith(marker, pos)), None)
        if match:
            marker, idx = match
            out.append(idx)
            pos += len(marker)
        else:
            out.append(BYTE_TOKEN_BASE + raw[pos])
            pos += 1
    return out


def generate_tokenizer_fixtures(root: Path) -> None:
    cases: list[dict[str, Any]] = []
    raw_cases: list[tuple[str, bytes, str]] = [
        ("empty", b"", "empty input"),
        ("ascii", b"A z!", "ASCII and spaces"),
        ("whitespace", b" \t\r\n  ", "mixed whitespace"),
        ("nul-controls", b"A\x00B\x1fC", "NUL and C0 control"),
        ("utf8-composed", "café".encode(), "composed accent"),
        ("utf8-decomposed", "cafe\u0301".encode(), "decomposed accent"),
        ("emoji-zwj", "👩‍💻".encode(), "emoji ZWJ sequence"),
        ("cjk", "漢字かな".encode(), "CJK text"),
        ("rtl", "العربية".encode(), "right-to-left script"),
        ("invalid-overlong", bytes.fromhex("c0af"), "invalid UTF-8 overlong sequence"),
        ("invalid-continuation", bytes.fromhex("80bf"), "orphan UTF-8 continuation bytes"),
        ("invalid-truncated-4", bytes.fromhex("f09f92"), "truncated four-byte sequence"),
    ]
    for case_id, raw, note in raw_cases:
        cases.append({
            "id": case_id,
            "input_hex": raw.hex(),
            "input_utf8": raw.decode("utf-8", errors="strict") if _valid_utf8(raw) else None,
            "expected_ids_no_special": byte_ids(raw),
            "expected_roundtrip_hex": raw.hex(),
            "note": note,
        })
    write_jsonl(root / "fixtures" / "tokenizer" / "cases.jsonl", cases)

    special_inputs = [
        b"<s>", b"</s>", b"<|eot_id|>", b"x<|eot_id|>y", b"<|tool_call|>{}\n<|end_tool_call|>", b"<|unknown_marker|>"
    ]
    special_rows = []
    for i, raw in enumerate(special_inputs, 1):
        special_rows.append({
            "id": f"special-{i:02d}",
            "input_hex": raw.hex(),
            "parse_special_false": special_parse_ids(raw, False),
            "parse_special_true": special_parse_ids(raw, True),
            "add_bos_true_prefix": [1] + special_parse_ids(raw, True),
        })
    write_jsonl(root / "fixtures" / "tokenizer" / "special-token-cases.jsonl", special_rows)
    write_json(root / "fixtures" / "tokenizer" / "vocabulary-map.json", {
        "special_tokens": {token: idx for idx, token in enumerate(SPECIAL_TOKENS)},
        "byte_token_rule": "token_id = 7 + input_byte",
        "byte_token_base": BYTE_TOKEN_BASE,
        "vocabulary_size": BYTE_TOKEN_BASE + 256,
    })


def _valid_utf8(raw: bytes) -> bool:
    try:
        raw.decode("utf-8", errors="strict")
        return True
    except UnicodeDecodeError:
        return False


def generate_chat_fixtures(root: Path) -> None:
    chat_dir = root / "fixtures" / "chat"
    write_text(chat_dir / "default.jinja", DEFAULT_TEMPLATE + "\n")
    write_text(chat_dir / "named-strict.jinja", STRICT_TEMPLATE + "\n")
    write_text(chat_dir / "named-tools.jinja", TOOLS_TEMPLATE + "\n")
    cases = [
        {
            "id": "default-basic",
            "template": "default",
            "messages": [{"role": "user", "content": "Hello"}, {"role": "assistant", "content": "Hi"}],
            "tools": None,
            "add_generation_prompt": False,
            "expected_utf8": "<|user|>\nHello\n<|assistant|>\nHi\n",
        },
        {
            "id": "default-generation-prompt",
            "template": "default",
            "messages": [{"role": "system", "content": "Return one byte."}, {"role": "user", "content": "A"}],
            "tools": None,
            "add_generation_prompt": True,
            "expected_utf8": "<|system|>\nReturn one byte.\n<|user|>\nA\n<|assistant|>\n",
        },
        {
            "id": "named-strict",
            "template": "strict",
            "messages": [{"role": "user", "content": "x"}, {"role": "tool", "content": "{\"ok\":true}"}],
            "tools": None,
            "add_generation_prompt": True,
            "expected_utf8": "[user]x\n[tool]{\"ok\":true}\n[assistant]",
        },
        {
            "id": "named-tools",
            "template": "tools",
            "messages": [{"role": "user", "content": "Echo A"}],
            "tools": [{"type": "function", "function": {"name": "echo", "description": "Echo text", "parameters": {"type": "object", "properties": {"text": {"type": "string"}}, "required": ["text"], "additionalProperties": False}}}],
            "add_generation_prompt": True,
            "expected_utf8": "<tools>[{\"function\":{\"description\":\"Echo text\",\"name\":\"echo\",\"parameters\":{\"additionalProperties\":false,\"properties\":{\"text\":{\"type\":\"string\"}},\"required\":[\"text\"],\"type\":\"object\"}},\"type\":\"function\"}]</tools>\n<user>Echo A</user>\n<assistant>",
        },
    ]
    write_jsonl(chat_dir / "cases.jsonl", cases)
    malformed = [
        {"id": "unclosed-expression", "template_utf8": "{{ messages[0]['content'] ", "expected": "reject-template-parse"},
        {"id": "unknown-filter", "template_utf8": "{{ 'x' | pfir10_missing_filter }}", "expected": "reject-template-render"},
        {"id": "strict-unknown-role", "template": "strict", "messages": [{"role": "alien", "content": "x"}], "expected": "reject-role"},
    ]
    write_jsonl(chat_dir / "malformed-cases.jsonl", malformed)


def generate_structured_fixtures(root: Path) -> None:
    structured = root / "fixtures" / "structured"
    answer_schema = {
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "type": "object",
        "properties": {"answer": {"type": "integer", "const": 42}},
        "required": ["answer"],
        "additionalProperties": False,
    }
    tool_schema = {
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "type": "object",
        "properties": {"text": {"type": "string", "const": "A", "minLength": 1, "maxLength": 8}},
        "required": ["text"],
        "additionalProperties": False,
    }
    write_json(structured / "answer.schema.json", answer_schema)
    write_json(structured / "echo-tool.schema.json", tool_schema)
    write_text(structured / "grammars" / "answer.gbnf", 'root ::= "{" ws "\\\"answer\\\"" ws ":" ws "42" ws "}" ws\nws ::= ([ \\t\\n\\r] ws)?\n')
    write_text(structured / "grammars" / "abc-state.gbnf", 'root ::= "A" ("B" | "C") "\\n"\n')
    rows = [
        {"id": "answer-valid-compact", "schema": "answer.schema.json", "json_text": "{\"answer\":42}", "valid": True},
        {"id": "answer-valid-spaced", "schema": "answer.schema.json", "json_text": "{ \"answer\" : 42 }", "valid": True},
        {"id": "answer-wrong-const", "schema": "answer.schema.json", "json_text": "{\"answer\":41}", "valid": False},
        {"id": "answer-extra-key", "schema": "answer.schema.json", "json_text": "{\"answer\":42,\"x\":0}", "valid": False},
        {"id": "answer-trailing", "schema": "answer.schema.json", "json_text": "{\"answer\":42}x", "valid": False},
        {"id": "tool-valid", "schema": "echo-tool.schema.json", "json_text": "{\"text\":\"A\"}", "valid": True},
        {"id": "tool-empty", "schema": "echo-tool.schema.json", "json_text": "{\"text\":\"\"}", "valid": False},
    ]
    write_jsonl(structured / "cases.jsonl", rows)
    grammar_state = [
        {"id": "start", "accepted_prefix": "", "allowed_next_utf8": ["A"]},
        {"id": "after-a", "accepted_prefix": "A", "allowed_next_utf8": ["B", "C"]},
        {"id": "after-ab", "accepted_prefix": "AB", "allowed_next_utf8": ["\n"]},
        {"id": "after-ac", "accepted_prefix": "AC", "allowed_next_utf8": ["\n"]},
        {"id": "complete", "accepted_prefix": "AB\n", "allowed_next_utf8": []},
        {"id": "reject-ad", "accepted_prefix": "AD", "expected": "reject"},
    ]
    write_jsonl(structured / "grammar-state-cases.jsonl", grammar_state)


def generate_api_fixtures(root: Path) -> None:
    api = root / "fixtures" / "api"
    tool = {
        "type": "function",
        "function": {
            "name": "echo",
            "description": "Echo text",
            "parameters": {
                "type": "object",
                "properties": {"text": {"type": "string", "const": "A"}},
                "required": ["text"],
                "additionalProperties": False,
            },
        },
    }
    chat_req = {
        "model": "pfir10-tiny",
        "messages": [{"role": "user", "content": "Echo A"}],
        "tools": [tool],
        "tool_choice": {"type": "function", "function": {"name": "echo"}},
        "parallel_tool_calls": False,
        "temperature": 0,
        "seed": 1234,
        "stream": False,
    }
    stream_req = dict(chat_req); stream_req["stream"] = True; stream_req["stream_options"] = {"include_usage": True}
    structured_req = {
        "model": "pfir10-tiny",
        "messages": [{"role": "user", "content": "Return 42."}],
        "response_format": {"type": "json_schema", "json_schema": {"name": "answer", "strict": True, "schema": json.loads((root / "fixtures" / "structured" / "answer.schema.json").read_text())}},
        "temperature": 0,
        "seed": 1234,
        "stream": False,
    }
    write_json(api / "requests" / "chat-tool-required.json", chat_req)
    write_json(api / "requests" / "chat-tool-required-stream.json", stream_req)
    write_json(api / "requests" / "chat-structured-answer.json", structured_req)
    malformed_requests = [
        {"id": "messages-not-array", "request": {"model": "pfir10-tiny", "messages": "x"}, "expected": "reject-request"},
        {"id": "tool-choice-missing-tool", "request": {"model": "pfir10-tiny", "messages": [{"role": "user", "content": "x"}], "tools": [], "tool_choice": {"type": "function", "function": {"name": "missing"}}}, "expected": "reject-request"},
        {"id": "schema-invalid-type", "request": {"model": "pfir10-tiny", "messages": [{"role": "user", "content": "x"}], "response_format": {"type": "json_schema", "json_schema": {"name": "bad", "strict": True, "schema": {"type": "pfir10-not-a-json-schema-type"}}}}, "expected": "reject-request"},
        {"id": "stream-options-without-stream", "request": {"model": "pfir10-tiny", "messages": [{"role": "user", "content": "x"}], "stream": False, "stream_options": {"include_usage": True}}, "expected": "reject-or-ignore-extension", "fork_variance": True},
    ]
    for row in malformed_requests:
        write_json(api / "requests" / "malformed" / (row["id"] + ".json"), row["request"])
    write_jsonl(api / "requests" / "malformed-expected.jsonl", [{k: v for k, v in row.items() if k != "request"} for row in malformed_requests])
    expected_tool = {
        "choices": [{
            "index": 0,
            "finish_reason": "tool_calls",
            "message": {
                "role": "assistant",
                "content": None,
                "tool_calls": [{"id": "CALL_ID", "type": "function", "function": {"name": "echo", "arguments": "{\"text\":\"A\"}"}}],
            },
        }],
        "object": "chat.completion",
    }
    write_json(api / "expected" / "chat-tool-required.canonical.json", expected_tool)
    write_json(api / "expected" / "chat-structured-answer.canonical.json", {"choices": [{"index": 0, "finish_reason": "stop", "message": {"role": "assistant", "content": "{\"answer\":42}"}}], "object": "chat.completion"})

    stream_dir = api / "streaming"
    canonical_events = [
        {"id": "STATIC", "object": "chat.completion.chunk", "created": 0, "model": "pfir10", "choices": [{"index": 0, "delta": {"role": "assistant"}, "finish_reason": None}]},
        {"id": "STATIC", "object": "chat.completion.chunk", "created": 0, "model": "pfir10", "choices": [{"index": 0, "delta": {"content": "A"}, "finish_reason": None}]},
        {"id": "STATIC", "object": "chat.completion.chunk", "created": 0, "model": "pfir10", "choices": [{"index": 0, "delta": {}, "finish_reason": "stop"}]},
    ]
    canonical_sse = "".join("data: " + canonical_json(e) + "\n\n" for e in canonical_events) + "data: [DONE]\n\n"
    write_text(stream_dir / "canonical.sse", canonical_sse)
    canonical_bytes = canonical_sse.encode()
    write_json(stream_dir / "canonical.chunks.json", {"encoding": "byte_ranges", "chunks": [[0, 1], [1, 7], [7, 31], [31, len(canonical_bytes) - 2], [len(canonical_bytes) - 2, len(canonical_bytes)]]})

    malformed_sse = (
        "data: " + canonical_json(canonical_events[0]) + "\n\n" +
        "data: {not-json}\n\n" +
        "data: " + canonical_json(canonical_events[1]) + "\n\n" +
        "data: [DONE]\n\n" +
        "data: " + canonical_json(canonical_events[2]) + "\n\n"
    )
    write_text(stream_dir / "malformed-json-and-post-done.sse", malformed_sse)

    emoji_event = {"id": "STATIC", "object": "chat.completion.chunk", "created": 0, "model": "pfir10", "choices": [{"index": 0, "delta": {"content": "👩‍💻"}, "finish_reason": None}]}
    emoji_sse = "data: " + canonical_json(emoji_event) + "\n\ndata: [DONE]\n\n"
    emoji_raw = emoji_sse.encode("utf-8")
    emoji_start = emoji_raw.index("👩".encode("utf-8"))
    write_bytes(stream_dir / "utf8-split.sse", emoji_raw)
    write_json(stream_dir / "utf8-split.chunks.json", {"encoding": "byte_ranges", "chunks": [[0, emoji_start + 1], [emoji_start + 1, emoji_start + 3], [emoji_start + 3, len(emoji_raw)]]})

    tool_call_events = [
        {"id": "STATIC", "object": "chat.completion.chunk", "created": 0, "model": "pfir10", "choices": [{"index": 0, "delta": {"role": "assistant", "tool_calls": [{"index": 0, "id": "CALL_ID", "type": "function", "function": {"name": "echo", "arguments": ""}}]}, "finish_reason": None}]},
        {"id": "STATIC", "object": "chat.completion.chunk", "created": 0, "model": "pfir10", "choices": [{"index": 0, "delta": {"tool_calls": [{"index": 0, "function": {"arguments": "{\"text\":"}}]}, "finish_reason": None}]},
        {"id": "STATIC", "object": "chat.completion.chunk", "created": 0, "model": "pfir10", "choices": [{"index": 0, "delta": {"tool_calls": [{"index": 0, "function": {"arguments": "\"A\"}"}}]}, "finish_reason": None}]},
        {"id": "STATIC", "object": "chat.completion.chunk", "created": 0, "model": "pfir10", "choices": [{"index": 0, "delta": {}, "finish_reason": "tool_calls"}]},
    ]
    tool_call_sse = "".join("data: " + canonical_json(e) + "\n\n" for e in tool_call_events) + "data: [DONE]\n\n"
    write_text(stream_dir / "tool-call.sse", tool_call_sse)
    tool_call_raw = tool_call_sse.encode("utf-8")
    split_at = tool_call_raw.index(b'arguments') + 12
    write_json(stream_dir / "tool-call.chunks.json", {"encoding": "byte_ranges", "chunks": [[0, 2], [2, split_at], [split_at, split_at + 1], [split_at + 1, len(tool_call_raw)]]})

    structured_events = [
        {"id": "STATIC", "object": "chat.completion.chunk", "created": 0, "model": "pfir10", "choices": [{"index": 0, "delta": {"role": "assistant", "content": "{\"answer\":"}, "finish_reason": None}]},
        {"id": "STATIC", "object": "chat.completion.chunk", "created": 0, "model": "pfir10", "choices": [{"index": 0, "delta": {"content": "42}"}, "finish_reason": None}]},
        {"id": "STATIC", "object": "chat.completion.chunk", "created": 0, "model": "pfir10", "choices": [{"index": 0, "delta": {}, "finish_reason": "stop"}]},
    ]
    structured_sse = "".join("data: " + canonical_json(e) + "\n\n" for e in structured_events) + "data: [DONE]\n\n"
    write_text(stream_dir / "structured-output.sse", structured_sse)

    expected_rows = [
        {"id": "canonical", "file": "canonical.sse", "chunks": "canonical.chunks.json", "events": canonical_events, "done": True, "malformed_records_skipped": 0, "post_done_ignored": 0},
        {"id": "malformed", "file": "malformed-json-and-post-done.sse", "chunks": None, "events": canonical_events[:2], "done": True, "malformed_records_skipped": 1, "post_done_ignored": 1},
        {"id": "utf8-split", "file": "utf8-split.sse", "chunks": "utf8-split.chunks.json", "events": [emoji_event], "done": True, "malformed_records_skipped": 0, "post_done_ignored": 0},
        {"id": "tool-call", "file": "tool-call.sse", "chunks": "tool-call.chunks.json", "events": tool_call_events, "done": True, "malformed_records_skipped": 0, "post_done_ignored": 0},
        {"id": "structured-output", "file": "structured-output.sse", "chunks": None, "events": structured_events, "done": True, "malformed_records_skipped": 0, "post_done_ignored": 0},
    ]
    write_jsonl(stream_dir / "expected.jsonl", expected_rows)


def generate_malformed_fixtures(root: Path) -> None:
    malformed = root / "fixtures" / "malformed"
    valid = b'{"text":"A\\u0000' + "👩‍💻".encode("utf-8") + b'","n":42}'
    selected_cuts = sorted(set([0, 1, 2, 5, 9, 12, len(valid) - 1, len(valid)] + [i for i, b in enumerate(valid) if b >= 0x80]))
    rows = []
    for cut in selected_cuts:
        candidate = valid[:cut]
        rows.append({
            "id": f"json-cut-{cut:03d}",
            "base_sha256": sha256_bytes(valid),
            "cut_offset": cut,
            "bytes_hex": candidate.hex(),
            "expected": "accept" if cut == len(valid) else "reject-incomplete-json",
        })
    write_jsonl(malformed / "json-boundaries.jsonl", rows)
    write_bytes(malformed / "json-boundaries.base.bin", valid)

    utf8_rows = [
        {"id": "valid-ascii", "bytes_hex": b"A".hex(), "valid_utf8": True},
        {"id": "valid-emoji", "bytes_hex": "💾".encode().hex(), "valid_utf8": True},
        {"id": "overlong-slash", "bytes_hex": "c0af", "valid_utf8": False},
        {"id": "surrogate", "bytes_hex": "eda080", "valid_utf8": False},
        {"id": "too-high", "bytes_hex": "f4908080", "valid_utf8": False},
        {"id": "orphan-continuation", "bytes_hex": "80", "valid_utf8": False},
        {"id": "truncated-3", "bytes_hex": "e282", "valid_utf8": False},
        {"id": "truncated-4", "bytes_hex": "f09f92", "valid_utf8": False},
    ]
    write_jsonl(malformed / "utf8-boundaries.jsonl", utf8_rows)


def generate_sampler_state_fixtures(root: Path) -> None:
    sampler = root / "fixtures" / "sampler"
    vectors = [
        {"id": "temperature-1", "operation": {"name": "temperature", "value": 1.0}, "input_probabilities": [0.1, 0.2, 0.3, 0.4], "expected_probabilities": [0.1, 0.2, 0.3, 0.4], "atol": 1e-5},
        {"id": "temperature-0", "operation": {"name": "temperature", "value": 0.0}, "input_probabilities": [0.1, 0.2, 0.3, 0.4], "expected_probabilities": [0.0, 0.0, 0.0, 1.0], "atol": 1e-5},
        {"id": "top-k-3", "operation": {"name": "top_k", "value": 3}, "input_probabilities": [0.1, 0.2, 0.3, 0.4], "expected_token_order": [3, 2, 1], "expected_probabilities": [0.44444, 0.33333, 0.22222], "atol": 1e-5},
        {"id": "top-p-0.7", "operation": {"name": "top_p", "value": 0.7}, "input_probabilities": [0.1, 0.2, 0.3, 0.4], "expected_token_order": [3, 2], "expected_probabilities": [0.571429, 0.428571], "atol": 1e-5},
        {"id": "min-p-0.51", "operation": {"name": "min_p", "value": 0.51}, "input_probabilities": [0.1, 0.2, 0.3, 0.4], "expected_token_order": [2, 3], "expected_probabilities": [0.4285714285714286, 0.5714285714285714], "atol": 1e-5},
        {"id": "penalty-repeat", "operation": {"name": "penalties", "repeat_penalty": 50.0, "frequency": 0.0, "presence": 0.0, "accepted_tokens": [0, 1, 2]}, "input_probabilities": [0.2, 0.2, 0.2, 0.2, 0.2], "expected_probabilities": [0.0, 0.0, 0.0, 0.5, 0.5], "atol": 1e-5},
    ]
    write_jsonl(sampler / "probability-vectors.jsonl", vectors)
    write_jsonl(sampler / "state-properties.jsonl", [
        {"id": "sampler-clone", "recipe": ["create fixed-seed distribution sampler", "accept token IDs 7,8,9", "clone sampler", "sample 16 times from identical candidate arrays"], "oracle": "both token sequences are exactly equal"},
        {"id": "sampler-reset", "recipe": ["create fixed-seed distribution sampler", "sample 16 tokens", "reset", "repeat from identical candidate arrays"], "oracle": "post-reset sequence equals initial sequence if reset contract includes RNG; otherwise explicit expected-reject"},
        {"id": "grammar-clone", "recipe": ["initialize abc-state.gbnf", "accept byte token A", "clone grammar sampler", "query allowed tokens"], "oracle": "both allowed token-ID sets equal [73,74] under byte-token rule"},
    ])
    write_jsonl(sampler / "rng-properties.jsonl", [
        {"id": "seed-replay-within-candidate", "seed": 1234, "candidate_probabilities": [0.1, 0.2, 0.3, 0.4], "draws": 64, "oracle": "two fresh samplers in the same candidate produce identical token-ID sequences", "cross_fork_sequence_oracle": False},
        {"id": "seed-change-sensitivity", "seeds": [1234, 1235], "candidate_probabilities": [0.1, 0.2, 0.3, 0.4], "draws": 64, "oracle": "sequences should not be identical; failure is diagnostic, not a sole conformance failure"},
    ])

    state = root / "fixtures" / "state"
    write_jsonl(state / "save-restore-scenarios.jsonl", [
        {"id": "full-state-continuation", "model": "../gguf/pfir10-tiny-llama-f32-v3.gguf", "prompt_hex": b"State A".hex(), "seed": 1234, "save_after_prompt": True, "continuation_tokens": 8, "oracle": {"token_ids": "exact", "logits": "numeric_f32_restore"}},
        {"id": "sequence-isolation", "model": "../gguf/pfir10-tiny-llama-f32-v3.gguf", "sequences": 2, "prompt_hex": b"ABCD".hex(), "operation": "remove sequence 0", "oracle": "serialized state bytes for sequence 1 are unchanged within the same process"},
        {"id": "fragmented-kv-restore", "model": "../gguf/pfir10-tiny-llama-f32-v3.gguf", "operations": ["decode seq0 positions 0..7", "remove positions 2..4", "save", "restore fresh context", "replay position 2"], "oracle": {"token_ids": "exact", "logits": "numeric_f32_restore"}},
    ])
    write_jsonl(state / "recurrent-scenarios.jsonl", [
        {"id": "nonrecurrent-guard", "model": "../gguf/pfir10-tiny-llama-f32-v3.gguf", "oracle": "report not-applicable without mutating state"},
        {"id": "qwen35-rollback-recipe", "model_recipe": "../../recipes/portable-recurrent-model.md", "prompt_tokens": [1,2,3,4,5,6,7,8,9], "rollback_last_position": True, "oracle": {"restored_logits": "numeric_f32_restore", "dirty_context_restored_logits": "numeric_f32_restore"}},
    ])
    write_jsonl(state / "speculative-scenarios.jsonl", [
        {"id": "ngram-simple-no-sidecar", "model": "../gguf/pfir10-tiny-llama-f32-v3.gguf", "spec_type": "ngram-simple", "draft_n_max": 4, "prompt_hex": b"ABABAB".hex(), "seed": 1234, "oracle": "final accepted token IDs equal non-speculative greedy token IDs; draft counters are diagnostic"},
        {"id": "mtp-sidecar-open", "model_recipe": "../../recipes/portable-mtp-sidecar.md", "spec_type": "draft-mtp", "oracle": "open until a qualified minimal sidecar exists"},
    ])


def generate_fork_specific(root: Path) -> None:
    write_json(root / "fixtures" / "fork-specific" / "rocmfpx-type-map.json", {
        "source_commit": "61f2f2d7bc4955e9bca821095ef69125837133b5",
        "enum": {
            "GGML_TYPE_Q4_0_ROCMFP4": 100,
            "GGML_TYPE_Q4_0_ROCMFP4_FAST": 101,
            "GGML_TYPE_Q6_0_ROCMFPX": 102,
            "GGML_TYPE_Q8_0_ROCMFPX": 103,
            "GGML_TYPE_Q3_0_ROCMFPX": 104,
            "GGML_TYPE_TURBO3_0": 105,
            "GGML_TYPE_TURBO4_0": 106,
            "GGML_TYPE_Q2_0_ROCMFPX": 107,
            "GGML_TYPE_COUNT": 108,
        },
        "claim_label": "VERIFIED-SOURCE",
        "qualification": "The numeric IDs are source-verified. The bundled type-106 payload is a dispatch probe, not a qualified TurboQuant numerical artifact.",
    })
    write_jsonl(root / "fixtures" / "fork-specific" / "rocmfpx-turboquant-cpu-vectors.recipe.jsonl", [
        {"id": "turbo4-zero-block", "ggml_type": 106, "logical_values": 32, "encoded_bytes_hex": "00" * 18, "oracle": "decode to finite values; exact numerical values open until source algorithm is independently transcribed and qualified", "status": "unexecuted-evidence"},
        {"id": "turbo3-zero-block", "ggml_type": 105, "logical_values": 32, "encoded_bytes_hex": "00" * 14, "oracle": "decode to finite values; exact numerical values open until source algorithm is independently transcribed and qualified", "status": "unexecuted-evidence"},
    ])


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    root = args.root.resolve()
    generate_structured_fixtures(root)
    ggufs = generate_ggufs(root)
    generate_tokenizer_fixtures(root)
    generate_chat_fixtures(root)
    generate_api_fixtures(root)
    generate_malformed_fixtures(root)
    generate_sampler_state_fixtures(root)
    generate_fork_specific(root)
    generated_files = [
        {"path": str(path.relative_to(root)), "sha256": sha256_bytes(path.read_bytes()), "length": path.stat().st_size}
        for path in sorted((root / "fixtures").rglob("*")) if path.is_file()
    ]
    write_json(root / "qualification" / "generated-assets.json", {
        "generator": "recipes/generate_assets.py",
        "generator_sha256": sha256_bytes(Path(__file__).read_bytes()),
        "candidate_binaries_executed": False,
        "generated_files": generated_files,
        "ggufs": ggufs,
    })
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
