#!/usr/bin/env python3
"""Generate and validate the non-authoritative HaloFPX GitHub Wiki mirror.

The in-repository Wiki is canonical.  This tool reads an exact Git commit,
uses the committed frozen path map, rewrites links for the separate GitHub
Wiki repository, and emits a deterministic convenience mirror.
"""

from __future__ import annotations

import argparse
import base64
import ctypes
import errno
import hashlib
import html
import importlib
import importlib.metadata
import importlib.util
import io
import json
import os
import posixpath
import re
import stat
import subprocess
import sys
import tempfile
import unicodedata
from collections import Counter
from dataclasses import dataclass
from html.parser import HTMLParser
from pathlib import Path, PurePosixPath
from typing import Iterable, Mapping, Sequence
from urllib.parse import quote, unquote, urlsplit

SCHEMA = "halofpx.github-wiki-mirror.v1"
MAP_SCHEMA = "halofpx.github-wiki-page-map.v1"
GENERATOR_VERSION = "1.0.0"
RUNTIME_DISTRIBUTIONS = {
    "markdown-it-py": {
        "import_root": "markdown_it",
        "record_root_sha256": "a7ad198841b5d867669c3f4b87791229f7cef1f087ef4c4dc31d63588061b0f0",
        "version": "4.2.0",
    },
    "mdurl": {
        "import_root": "mdurl",
        "record_root_sha256": "3b8f307fe19112f68b6ff0622ae28d77a674fcafb5d33ab7f0542bdf5e6483f2",
        "version": "0.1.2",
    },
}
GIT_ROUTING_ENVIRONMENT = frozenset(
    {
        "GIT_ALTERNATE_OBJECT_DIRECTORIES",
        "GIT_CEILING_DIRECTORIES",
        "GIT_COMMON_DIR",
        "GIT_CONFIG",
        "GIT_CONFIG_COUNT",
        "GIT_CONFIG_GLOBAL",
        "GIT_CONFIG_NOSYSTEM",
        "GIT_CONFIG_PARAMETERS",
        "GIT_CONFIG_SYSTEM",
        "GIT_DIR",
        "GIT_DISCOVERY_ACROSS_FILESYSTEM",
        "GIT_EXEC_PATH",
        "GIT_GRAFT_FILE",
        "GIT_ICASE_PATHSPECS",
        "GIT_INDEX_FILE",
        "GIT_LITERAL_PATHSPECS",
        "GIT_NAMESPACE",
        "GIT_NOGLOB_PATHSPECS",
        "GIT_OBJECT_DIRECTORY",
        "GIT_PREFIX",
        "GIT_QUARANTINE_PATH",
        "GIT_REPLACE_REF_BASE",
        "GIT_SHALLOW_FILE",
        "GIT_WORK_TREE",
    }
)
CANONICAL_ROOT = PurePosixPath("project/wiki/HaloFPX_Wiki")
REPOSITORY = "JCFrags/HaloFPX"
REPOSITORY_URL = f"https://github.com/{REPOSITORY}"
LICENSE_PATH = PurePosixPath("LICENSES_AND_PROVENANCE.md")
SPECIAL_OUTPUTS = frozenset({"_Sidebar.md", "_Footer.md", "mirror-manifest.json"})
STANDARD_SECTION_PAGES = frozenset(
    {
        "README.md",
        "design_implications.md",
        "facts_and_constraints.md",
        "open_questions.md",
        "procedures_and_checks.md",
        "sources.md",
    }
)
ALLOWED_EXTERNAL_SCHEMES = frozenset({"http", "https", "mailto"})
WINDOWS_RESERVED_COMPONENT = re.compile(
    r"(?i)^(?:con|prn|aux|nul|com[1-9]|lpt[1-9])(?:\.|$)"
)
EXPLICIT_FRAGMENT_ALIASES = {
    (
        "04_Hardware_and_OS_Platform/22_Power_Thermals_Cooling_and_Sustained_Clocks/facts_and_constraints.md",
        "sources.md#s22-02",
    ): "s22-02",
    (
        "04_Hardware_and_OS_Platform/23_Linux_Kernel_amdgpu_Firmware_ROCm_and_Mesa_Compatibility_Matrix/README.md",
        "sources.md#s23-l02",
    ): "s23-l02--pre-incident-target-os-and-deployment-inventory",
    (
        "04_Hardware_and_OS_Platform/23_Linux_Kernel_amdgpu_Firmware_ROCm_and_Mesa_Compatibility_Matrix/README.md",
        "sources.md#s23-l03",
    ): "s23-l03--post-incident-recovered-service-authority",
    (
        "04_Hardware_and_OS_Platform/23_Linux_Kernel_amdgpu_Firmware_ROCm_and_Mesa_Compatibility_Matrix/facts_and_constraints.md",
        "sources.md#s23-l03",
    ): "s23-l03--post-incident-recovered-service-authority",
}
MARKDOWN: object | None = None


class MirrorError(RuntimeError):
    """A closed validation or generation failure."""


def _validate_portable_path(path: PurePosixPath, raw: str, role: str) -> None:
    if (
        path.is_absolute()
        or ".." in path.parts
        or path.as_posix() != raw
        or unicodedata.normalize("NFC", raw) != raw
        or "\\" in raw
        or len(raw.encode("utf-8")) > 240
        or any(unicodedata.category(character) in {"Cc", "Cf", "Cs"} for character in raw)
    ):
        raise MirrorError(f"unsafe or noncanonical {role} path: {raw!r}")
    for component in path.parts:
        if (
            component in {"", "."}
            or component.endswith((".", " "))
            or any(character in '<>:"|?*' for character in component)
            or len(component.encode("utf-8")) > 120
            or WINDOWS_RESERVED_COMPONENT.match(component)
        ):
            raise MirrorError(f"nonportable {role} path component: {raw!r}")


def validate_runtime_dependencies() -> None:
    """Validate the complete parser stack without executing third-party code."""

    for distribution, contract in RUNTIME_DISTRIBUTIONS.items():
        expected = contract["version"]
        try:
            actual = importlib.metadata.version(distribution)
            installed = importlib.metadata.distribution(distribution)
        except importlib.metadata.PackageNotFoundError as error:
            raise MirrorError(
                f"required distribution is not installed: {distribution}=={expected}"
            ) from error
        if actual != expected or installed.version != expected:
            raise MirrorError(
                f"unaudited parser dependency: {distribution}=={actual}; expected {expected}"
            )

        root = Path(installed.locate_file("")).resolve()
        record_rows: list[tuple[str, int, str, str]] = []
        verified_files: set[Path] = set()
        distribution_files = installed.files
        if not distribution_files:
            raise MirrorError(f"installed parser distribution has no RECORD authority: {distribution}")
        for entry in distribution_files:
            path_text = str(entry).replace("\\", "/")
            if (
                path_text.startswith("../")
                or "/__pycache__/" in path_text
                or path_text.endswith(("/INSTALLER", "/RECORD", "/REQUESTED", "/direct_url.json"))
            ):
                continue
            if entry.hash is None or entry.size is None or entry.hash.mode != "sha256":
                raise MirrorError(
                    f"installed parser record lacks SHA-256/size authority: {distribution}:{path_text}"
                )
            path = Path(installed.locate_file(entry)).resolve()
            if not path.is_relative_to(root) or not path.is_file():
                raise MirrorError(
                    f"installed parser record escapes or is missing: {distribution}:{path_text}"
                )
            data = path.read_bytes()
            padding = "=" * (-len(entry.hash.value) % 4)
            expected_digest = base64.urlsafe_b64decode(entry.hash.value + padding).hex()
            if len(data) != entry.size or hashlib.sha256(data).hexdigest() != expected_digest:
                raise MirrorError(f"installed parser file differs from RECORD: {distribution}:{path_text}")
            record_rows.append((path_text, entry.size, entry.hash.mode, entry.hash.value))
            verified_files.add(path)

        record_root = hashlib.sha256()
        for path_text, size, mode, value in sorted(record_rows):
            record_root.update(path_text.encode("utf-8"))
            record_root.update(b"\0")
            record_root.update(str(size).encode("ascii"))
            record_root.update(b"\0")
            record_root.update(mode.encode("ascii"))
            record_root.update(b":")
            record_root.update(value.encode("ascii"))
            record_root.update(b"\n")
        if record_root.hexdigest() != contract["record_root_sha256"]:
            raise MirrorError(f"installed parser RECORD set is not the audited wheel: {distribution}")

        try:
            spec = importlib.util.find_spec(contract["import_root"])
        except (AttributeError, ImportError, ValueError) as error:
            raise MirrorError(
                f"cannot locate audited parser import without executing it: {distribution}"
            ) from error
        if spec is None or spec.origin is None:
            raise MirrorError(f"cannot locate audited parser import: {distribution}")
        module_file = Path(spec.origin).resolve()
        if module_file not in verified_files:
            raise MirrorError(
                f"parser import resolves outside the audited distribution: {distribution}:{module_file}"
            )


def _markdown_parser() -> object:
    """Import and construct the parser only after both distributions validate."""

    global MARKDOWN
    if MARKDOWN is None:
        validate_runtime_dependencies()
        importlib.import_module("mdurl")
        markdown_it = importlib.import_module("markdown_it")
        markdown_type = getattr(markdown_it, "MarkdownIt", None)
        if markdown_type is None:
            raise MirrorError("audited markdown-it-py import does not expose MarkdownIt")
        MARKDOWN = markdown_type(
            "commonmark",
            {"html": True, "inline_definitions": True, "store_labels": True},
        )
    return MARKDOWN


@dataclass(frozen=True)
class SourceFile:
    path: PurePosixPath
    mode: str
    git_blob: str
    size: int
    data: bytes

    @property
    def sha256(self) -> str:
        return hashlib.sha256(self.data).hexdigest()


@dataclass(frozen=True)
class SourceSnapshot:
    repo_root: Path
    source_ref: str
    commit: str
    wiki_tree: str
    files: tuple[SourceFile, ...]
    repo_files: frozenset[PurePosixPath]
    repo_dirs: frozenset[PurePosixPath]

    @property
    def by_path(self) -> dict[PurePosixPath, SourceFile]:
        return {item.path: item for item in self.files}


@dataclass(frozen=True)
class MappingEntry:
    source: PurePosixPath
    destination: PurePosixPath
    kind: str


@dataclass(frozen=True)
class FrozenMap:
    entries: tuple[MappingEntry, ...]
    canonical_bytes: bytes
    expected_source_file_count: int
    expected_markdown_page_count: int
    expected_asset_count: int

    @property
    def by_source(self) -> dict[PurePosixPath, MappingEntry]:
        return {entry.source: entry for entry in self.entries}

    @property
    def sha256(self) -> str:
        return hashlib.sha256(self.canonical_bytes).hexdigest()


@dataclass(frozen=True)
class RenderedFile:
    destination: PurePosixPath
    data: bytes
    kind: str
    source: SourceFile | None = None

    @property
    def sha256(self) -> str:
        return hashlib.sha256(self.data).hexdigest()


@dataclass(frozen=True)
class RenderedMirror:
    files: tuple[RenderedFile, ...]

    @property
    def by_destination(self) -> dict[PurePosixPath, RenderedFile]:
        return {item.destination: item for item in self.files}


def _run_git(repo_root: Path, *args: str, input_bytes: bytes | None = None) -> bytes:
    command = ["git", "--no-replace-objects", "-C", str(repo_root), *args]
    environment = os.environ.copy()
    for name in tuple(environment):
        if (
            name in GIT_ROUTING_ENVIRONMENT
            or name.startswith("GIT_CONFIG_KEY_")
            or name.startswith("GIT_CONFIG_VALUE_")
        ):
            environment.pop(name)
    environment["GIT_NO_LAZY_FETCH"] = "1"
    environment["GIT_NO_REPLACE_OBJECTS"] = "1"
    environment["GIT_TERMINAL_PROMPT"] = "0"
    result = subprocess.run(
        command,
        input=input_bytes,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=environment,
        check=False,
    )
    if result.returncode != 0:
        detail = result.stderr.decode("utf-8", errors="replace").strip()
        raise MirrorError(f"git command failed ({' '.join(command)}): {detail}")
    return result.stdout


def find_repo_root(start: Path) -> Path:
    output = _run_git(start.resolve(), "rev-parse", "--show-toplevel")
    return Path(output.decode("utf-8").strip()).resolve()


def _read_git_blobs(repo_root: Path, object_ids: Sequence[str]) -> dict[str, bytes]:
    request = "".join(f"{object_id}\n" for object_id in object_ids).encode("ascii")
    response = _run_git(repo_root, "cat-file", "--batch", input_bytes=request)
    stream = io.BytesIO(response)
    blobs: dict[str, bytes] = {}
    for expected in object_ids:
        header = stream.readline().decode("ascii", errors="strict").rstrip("\n")
        parts = header.split(" ")
        if len(parts) != 3 or parts[1] != "blob":
            raise MirrorError(f"unexpected git cat-file header for {expected}: {header!r}")
        object_id, _kind, size_text = parts
        size = int(size_text)
        payload = stream.read(size)
        delimiter = stream.read(1)
        if len(payload) != size or delimiter != b"\n":
            raise MirrorError(f"truncated git cat-file payload for {expected}")
        if object_id != expected:
            raise MirrorError(f"git cat-file order changed: expected {expected}, got {object_id}")
        blobs[object_id] = payload
    if stream.read(1):
        raise MirrorError("unexpected trailing git cat-file output")
    return blobs


def load_snapshot(repo_root: Path, source_ref: str) -> SourceSnapshot:
    repo_root = repo_root.resolve()
    commit = _run_git(repo_root, "rev-parse", f"{source_ref}^{{commit}}").decode().strip()
    wiki_tree = _run_git(
        repo_root, "rev-parse", f"{commit}:{CANONICAL_ROOT.as_posix()}"
    ).decode().strip()
    raw_tree = _run_git(repo_root, "ls-tree", "-r", "-z", "--long", wiki_tree)
    rows: list[tuple[PurePosixPath, str, str, int]] = []
    for record in raw_tree.split(b"\0"):
        if not record:
            continue
        metadata, raw_path = record.split(b"\t", 1)
        parts = metadata.decode("ascii").split()
        if len(parts) != 4:
            raise MirrorError(f"unexpected ls-tree record: {record!r}")
        mode, kind, object_id, size_text = parts
        if mode != "100644" or kind != "blob":
            raise MirrorError(
                f"unsupported canonical Wiki entry {raw_path!r}: mode={mode}, type={kind}"
            )
        path = PurePosixPath(raw_path.decode("utf-8"))
        rows.append((path, mode, object_id, int(size_text)))
    rows.sort(key=lambda row: row[0].as_posix())
    if not rows:
        raise MirrorError(f"no files found at {commit}:{CANONICAL_ROOT}")
    blobs = _read_git_blobs(repo_root, [row[2] for row in rows])
    files: list[SourceFile] = []
    for path, mode, object_id, size in rows:
        data = blobs[object_id]
        if len(data) != size:
            raise MirrorError(f"Git blob size mismatch for {path}: {len(data)} != {size}")
        files.append(SourceFile(path, mode, object_id, size, data))

    raw_repo_paths = _run_git(repo_root, "ls-tree", "-r", "-z", "--name-only", commit)
    repo_files = frozenset(
        PurePosixPath(item.decode("utf-8"))
        for item in raw_repo_paths.split(b"\0")
        if item
    )
    repo_dirs: set[PurePosixPath] = set()
    for path in repo_files:
        parent = path.parent
        while parent != PurePosixPath("."):
            repo_dirs.add(parent)
            parent = parent.parent
    return SourceSnapshot(
        repo_root=repo_root,
        source_ref=source_ref,
        commit=commit,
        wiki_tree=wiki_tree,
        files=tuple(files),
        repo_files=repo_files,
        repo_dirs=frozenset(repo_dirs),
    )


def _slug(value: str) -> str:
    value = value.replace("_", "-")
    value = re.sub(r"[^A-Za-z0-9.-]+", "-", value)
    value = re.sub(r"-{2,}", "-", value).strip("-.")
    if not value:
        raise MirrorError("a source path produced an empty Wiki slug")
    return value


def derive_mapping(snapshot: SourceSnapshot) -> dict[str, object]:
    entries: list[dict[str, str]] = []
    seen_section_ids: set[str] = set()
    for source in snapshot.files:
        parts = source.path.parts
        if source.path.suffix.lower() != ".md":
            destination = PurePosixPath("assets") / source.path
            entries.append(
                {
                    "source": source.path.as_posix(),
                    "destination": destination.as_posix(),
                    "kind": "asset",
                }
            )
            continue
        if len(parts) == 1:
            destination_name = (
                "Home.md" if source.path.name == "README.md" else f"R-{_slug(source.path.stem)}.md"
            )
        elif len(parts) == 2 and source.path.name == "README.md":
            category_match = re.fullmatch(r"(\d{2})_(.+)", parts[0])
            if category_match is None:
                raise MirrorError(f"invalid category path: {source.path}")
            destination_name = f"C{category_match.group(1)}-{_slug(category_match.group(2))}.md"
        elif len(parts) == 3 and source.path.name in STANDARD_SECTION_PAGES:
            category_match = re.fullmatch(r"(\d{2})_(.+)", parts[0])
            section_match = re.fullmatch(r"(\d{2})_(.+)", parts[1])
            if category_match is None or section_match is None:
                raise MirrorError(f"invalid section path: {source.path}")
            section_id = section_match.group(1)
            if source.path.name == "README.md":
                if section_id in seen_section_ids:
                    raise MirrorError(f"duplicate section ID: {section_id}")
                seen_section_ids.add(section_id)
            prefix = f"S{section_id}-{_slug(section_match.group(2))}"
            destination_name = (
                f"{prefix}.md"
                if source.path.name == "README.md"
                else f"{prefix}--{_slug(source.path.stem)}.md"
            )
        else:
            raise MirrorError(
                f"Markdown path is outside the frozen root/category/section shape: {source.path}"
            )
        entries.append(
            {
                "source": source.path.as_posix(),
                "destination": destination_name,
                "kind": "page",
            }
        )
    entries.sort(key=lambda entry: entry["source"])
    page_count = sum(entry["kind"] == "page" for entry in entries)
    asset_count = sum(entry["kind"] == "asset" for entry in entries)
    return {
        "schema": MAP_SCHEMA,
        "canonical_root": CANONICAL_ROOT.as_posix(),
        "policy": "Frozen source-path to destination mapping. Preserve existing destinations across title or source-path changes.",
        "expected_source_file_count": len(entries),
        "expected_markdown_page_count": page_count,
        "expected_asset_count": asset_count,
        "entries": entries,
    }


def _canonical_json_bytes(value: object) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True, ensure_ascii=False) + "\n").encode("utf-8")


def load_frozen_map(path: Path) -> FrozenMap:
    try:
        raw_bytes = path.read_bytes()
        value = json.loads(raw_bytes.decode("utf-8", errors="strict"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise MirrorError(f"cannot read frozen page map {path}: {error}") from error
    if not isinstance(value, dict) or value.get("schema") != MAP_SCHEMA:
        raise MirrorError(f"unsupported frozen page-map schema in {path}")
    expected_keys = {
        "canonical_root",
        "entries",
        "expected_asset_count",
        "expected_markdown_page_count",
        "expected_source_file_count",
        "policy",
        "schema",
    }
    if set(value) != expected_keys:
        raise MirrorError(f"unexpected frozen page-map fields in {path}")
    canonical_bytes = _canonical_json_bytes(value)
    if raw_bytes != canonical_bytes:
        raise MirrorError(f"frozen page map is not in its one canonical JSON encoding: {path}")
    if value.get("canonical_root") != CANONICAL_ROOT.as_posix():
        raise MirrorError(f"wrong canonical root in frozen page map {path}")
    raw_entries = value.get("entries")
    if not isinstance(raw_entries, list):
        raise MirrorError("frozen page map entries must be a list")
    entries: list[MappingEntry] = []
    for index, raw in enumerate(raw_entries):
        if not isinstance(raw, dict) or set(raw) != {"source", "destination", "kind"}:
            raise MirrorError(f"invalid frozen page-map entry {index}")
        kind = raw["kind"]
        if kind not in {"page", "asset"}:
            raise MirrorError(f"invalid map kind at entry {index}: {kind!r}")
        source = PurePosixPath(raw["source"])
        destination = PurePosixPath(raw["destination"])
        _validate_portable_path(source, raw["source"], f"map source entry {index}")
        _validate_portable_path(
            destination, raw["destination"], f"map destination entry {index}"
        )
        if kind == "page" and (
            source.suffix.lower() != ".md"
            or destination.suffix != ".md"
            or len(destination.parts) != 1
        ):
            raise MirrorError(f"page-map entry {index} does not map Markdown to Markdown")
        if kind == "asset" and (
            source.suffix.lower() == ".md" or destination.parts[:1] != ("assets",)
        ):
            raise MirrorError(f"asset-map entry {index} has an invalid path")
        entries.append(MappingEntry(source, destination, kind))

    source_names = [entry.source.as_posix() for entry in entries]
    destination_names = [entry.destination.as_posix() for entry in entries]
    if len(source_names) != len(set(source_names)):
        raise MirrorError("frozen page map contains duplicate source paths")
    if source_names != sorted(source_names):
        raise MirrorError("frozen page map entries are not sorted by source path")
    if len(destination_names) != len({name.casefold() for name in destination_names}):
        raise MirrorError("frozen page map contains case-insensitive destination collisions")
    if any(name in SPECIAL_OUTPUTS for name in destination_names):
        raise MirrorError("frozen page map collides with a generated control file")

    expected_file_count = value.get("expected_source_file_count")
    expected_page_count = value.get("expected_markdown_page_count")
    expected_asset_count = value.get("expected_asset_count")
    if not all(isinstance(item, int) and item >= 0 for item in (expected_file_count, expected_page_count, expected_asset_count)):
        raise MirrorError("frozen page-map counts must be non-negative integers")
    if expected_file_count != len(entries):
        raise MirrorError("frozen page-map source count does not match its entries")
    if expected_page_count != sum(entry.kind == "page" for entry in entries):
        raise MirrorError("frozen page-map page count does not match its entries")
    if expected_asset_count != sum(entry.kind == "asset" for entry in entries):
        raise MirrorError("frozen page-map asset count does not match its entries")
    return FrozenMap(
        entries=tuple(entries),
        canonical_bytes=canonical_bytes,
        expected_source_file_count=expected_file_count,
        expected_markdown_page_count=expected_page_count,
        expected_asset_count=expected_asset_count,
    )


def validate_map_against_snapshot(frozen_map: FrozenMap, snapshot: SourceSnapshot) -> None:
    source_paths = {item.path for item in snapshot.files}
    mapped_paths = {entry.source for entry in frozen_map.entries}
    missing = sorted(path.as_posix() for path in source_paths - mapped_paths)
    stale = sorted(path.as_posix() for path in mapped_paths - source_paths)
    if missing or stale:
        raise MirrorError(f"frozen map/source coverage mismatch; missing={missing}, stale={stale}")
    if len(snapshot.files) != frozen_map.expected_source_file_count:
        raise MirrorError(
            f"source file count changed: {len(snapshot.files)} != {frozen_map.expected_source_file_count}"
        )
    by_path = snapshot.by_path
    for entry in frozen_map.entries:
        is_markdown = by_path[entry.source].path.suffix.lower() == ".md"
        if (entry.kind == "page") != is_markdown:
            raise MirrorError(f"map kind disagrees with source extension: {entry.source}")


def _extract_links(markdown: str) -> list[str]:
    links: list[str] = []

    def visit(tokens: Iterable[object]) -> None:
        for token in tokens:
            token_type = getattr(token, "type")
            if token_type == "link_open":
                href = token.attrGet("href")
                if href is not None:
                    links.append(href)
            elif token_type == "image":
                src = token.attrGet("src")
                if src is not None:
                    links.append(src)
            children = getattr(token, "children", None)
            if children:
                visit(children)

    visit(_markdown_parser().parse(markdown))
    return links


def _extract_rewriteable_links(markdown: str) -> list[str]:
    links: list[str] = []
    reference_occurrences: Counter[str] = Counter()
    definition_urls: dict[str, str] = {}

    def visit(tokens: Iterable[object]) -> None:
        for token in tokens:
            token_type = getattr(token, "type")
            if token_type == "link_open" and getattr(token, "markup", "") != "autolink":
                href = token.attrGet("href")
                if href is not None:
                    links.append(href)
                label = getattr(token, "meta", {}).get("label")
                if label:
                    reference_occurrences[label] += 1
            elif token_type == "image":
                src = token.attrGet("src")
                if src is not None:
                    links.append(src)
                label = getattr(token, "meta", {}).get("label")
                if label:
                    reference_occurrences[label] += 1
            elif token_type == "definition":
                label = getattr(token, "meta", {}).get("id")
                url = getattr(token, "meta", {}).get("url")
                if label and url:
                    definition_urls[label] = url
            children = getattr(token, "children", None)
            if children:
                visit(children)

    visit(_markdown_parser().parse(markdown))
    for label, url in definition_urls.items():
        for _occurrence in range(reference_occurrences[label]):
            try:
                links.remove(url)
            except ValueError as error:
                raise MirrorError(
                    f"reference occurrence ledger is missing definition URL: {label} -> {url}"
                ) from error
        links.append(url)
    return links


def _parse_markdown(markdown: str) -> tuple[list[object], dict[str, object]]:
    environment: dict[str, object] = {}
    tokens = _markdown_parser().parse(markdown, environment)
    return tokens, environment


def _line_offsets(text: str) -> list[int]:
    offsets = [0]
    for match in re.finditer("\n", text):
        offsets.append(match.end())
    offsets.append(len(text))
    return offsets


def _protected_block_ranges(text: str) -> list[tuple[int, int]]:
    offsets = _line_offsets(text)
    ranges: list[tuple[int, int]] = []
    for token in _markdown_parser().parse(text):
        if token.type not in {"fence", "code_block", "html_block"} or token.map is None:
            continue
        start_line, end_line = token.map
        start = offsets[min(start_line, len(offsets) - 1)]
        end = offsets[min(end_line, len(offsets) - 1)]
        ranges.append((start, end))
    ranges.sort()
    return ranges


def _range_containing(position: int, ranges: Sequence[tuple[int, int]], start_index: int) -> tuple[int, tuple[int, int] | None]:
    index = start_index
    while index < len(ranges) and ranges[index][1] <= position:
        index += 1
    if index < len(ranges) and ranges[index][0] <= position < ranges[index][1]:
        return index, ranges[index]
    return index, None


def _find_matching_backticks(text: str, start: int, run_length: int) -> int | None:
    position = start + run_length
    while position < len(text):
        next_tick = text.find("`", position)
        if next_tick < 0:
            return None
        end = next_tick
        while end < len(text) and text[end] == "`":
            end += 1
        if end - next_tick == run_length:
            return end
        position = end
    return None


def _parse_inline_destination(text: str, start: int) -> tuple[int, int] | None:
    position = start
    while position < len(text) and text[position] in " \t\r\n":
        position += 1
    if position >= len(text):
        return None
    if text[position] == ")":
        return (position, position)
    if text[position] == "<":
        destination_start = position + 1
        position = destination_start
        while position < len(text):
            if text[position] == "\\":
                position += 2
                continue
            if text[position] == "\n":
                return None
            if text[position] == ">":
                destination_end = position
                position += 1
                break
            position += 1
        else:
            return None
    else:
        destination_start = position
        depth = 0
        while position < len(text):
            character = text[position]
            if character == "\\":
                position += 2
                continue
            if character in " \t\r\n":
                break
            if character == "(":
                depth += 1
                if depth > 32:
                    return None
            elif character == ")":
                if depth == 0:
                    return (destination_start, position)
                depth -= 1
            elif ord(character) < 0x20:
                return None
            position += 1
        destination_end = position
        if depth != 0:
            return None
    while position < len(text) and text[position] in " \t\r\n":
        position += 1
    if position < len(text) and text[position] == ")":
        return (destination_start, destination_end)
    if position >= len(text) or text[position] not in {'"', "'", "("}:
        return None
    opener = text[position]
    closer = ")" if opener == "(" else opener
    position += 1
    while position < len(text):
        if text[position] == "\\":
            position += 2
            continue
        if text[position] == closer:
            position += 1
            break
        if text[position] == "\n" and opener != "(":
            return None
        position += 1
    else:
        return None
    while position < len(text) and text[position] in " \t\r\n":
        position += 1
    if position >= len(text) or text[position] != ")":
        return None
    return (destination_start, destination_end)


REFERENCE_DEFINITION = re.compile(
    r"(?m)^(?P<prefix> {0,3}\[[^\]\n]+\]:[ \t]*)(?:(?P<angle><(?P<angle_dest>[^>\n]*)>)|(?P<plain>[^ \t\r\n]+))"
)


def _starts_with_unescaped_open_bracket(text: str, close_position: int) -> bool:
    position = close_position - 1
    while position >= 0:
        if text[position] == "\\":
            position -= 2
            continue
        if text[position] == "[":
            backslashes = 0
            cursor = position - 1
            while cursor >= 0 and text[cursor] == "\\":
                backslashes += 1
                cursor -= 1
            return backslashes % 2 == 0
        if text[position] == "]" or text[position] == "\n":
            return False
        position -= 1
    return False


def _inline_destination_spans(text: str) -> list[tuple[int, int, str]]:
    """Find syntactically eligible inline-link destinations without touching HTML/code."""

    protected = _protected_block_ranges(text)
    spans: list[tuple[int, int, str]] = []
    position = 0
    protected_index = 0
    html_state: str | None = None
    while position < len(text):
        protected_index, containing = _range_containing(position, protected, protected_index)
        if containing is not None:
            position = containing[1]
            continue
        if html_state == "comment":
            end = text.find("-->", position)
            position = len(text) if end < 0 else end + 3
            html_state = None
            continue
        if html_state == "tag":
            quote_character: str | None = None
            while position < len(text):
                character = text[position]
                if quote_character:
                    if character == quote_character:
                        quote_character = None
                elif character in {'"', "'"}:
                    quote_character = character
                elif character == ">":
                    position += 1
                    html_state = None
                    break
                position += 1
            continue
        if text.startswith("<!--", position):
            html_state = "comment"
            position += 4
            continue
        character = text[position]
        if (
            character == "<"
            and position + 1 < len(text)
            and (text[position + 1].isalpha() or text[position + 1] in "!/?")
        ):
            html_state = "tag"
            position += 1
            continue
        if character == "\\":
            position += 2
            continue
        if character == "`":
            end_run = position
            while end_run < len(text) and text[end_run] == "`":
                end_run += 1
            closing = _find_matching_backticks(text, position, end_run - position)
            position = closing if closing is not None else end_run
            continue
        if (
            character == "]"
            and position + 1 < len(text)
            and text[position + 1] == "("
            and _starts_with_unescaped_open_bracket(text, position)
        ):
            span = _parse_inline_destination(text, position + 2)
            if span is not None:
                start, end = span
                raw_destination = text[start:end]
                normalized = _normalized_single_link(raw_destination)
                if normalized is not None:
                    spans.append((start, end, normalized))
                position = max(end, position + 2)
                continue
        position += 1
    if html_state is not None:
        raise MirrorError("unterminated inline HTML while locating link spans")
    return spans


def _reference_destination_spans(text: str) -> list[tuple[int, int, str]]:
    tokens, environment = _parse_markdown(text)
    lines = text.splitlines(keepends=True)
    line_offsets = _line_offsets(text)
    spans: list[tuple[int, int, str]] = []
    definitions = [token for token in tokens if getattr(token, "type") == "definition"]
    expected_references = environment.get("references", {})
    if not isinstance(expected_references, dict) or len(definitions) != len(expected_references):
        raise MirrorError("duplicate or untracked CommonMark reference definitions are not admitted")
    for token in definitions:
        if token.map is None:
            raise MirrorError("CommonMark reference definition lacks a source line map")
        start_line, end_line = token.map
        if end_line - start_line != 1:
            raise MirrorError("multiline/container reference definitions are not admitted")
        line = lines[start_line]
        match = REFERENCE_DEFINITION.fullmatch(line.rstrip("\r\n"))
        if match is None:
            raise MirrorError("reference definition is outside the admitted one-line grammar")
        if match.group("angle") is not None:
            local_start, local_end = match.span("angle_dest")
        else:
            local_start, local_end = match.span("plain")
        start = line_offsets[start_line] + local_start
        end = line_offsets[start_line] + local_end
        raw_destination = text[start:end]
        normalized = _normalized_single_link(raw_destination)
        expected = token.meta.get("url")
        if normalized is None or normalized != expected:
            raise MirrorError(
                f"reference destination span disagrees with CommonMark: {raw_destination!r} != {expected!r}"
            )
        spans.append((start, end, normalized))
    return spans


def _normalized_single_link(destination: str) -> str | None:
    escaped = destination.replace("\\", "\\\\").replace(")", "\\)")
    links = _extract_links(f"[x]({escaped})")
    return links[0] if len(links) == 1 else None


def _append_query_fragment(value: str, query: str, fragment: str) -> str:
    if query:
        value += f"?{query}"
    if fragment:
        value += f"#{fragment}"
    return value


def _repo_url(snapshot: SourceSnapshot, path: PurePosixPath, is_directory: bool, query: str, fragment: str) -> str:
    route = "tree" if is_directory else "blob"
    encoded_path = quote(path.as_posix(), safe="/")
    value = f"{REPOSITORY_URL}/{route}/{snapshot.commit}/{encoded_path}"
    return _append_query_fragment(value, query, fragment)


def _resolved_page_fragment(
    fragment: str,
    target_entry: MappingEntry,
    referring_page: PurePosixPath,
    snapshot: SourceSnapshot,
    original_destination: str,
) -> str:
    """Return an exact target anchor, admitting only audited explicit aliases."""

    if not fragment:
        return ""
    source = snapshot.by_path[target_entry.source]
    try:
        target_text = source.data.decode("utf-8", errors="strict")
    except UnicodeError as error:
        raise MirrorError(f"Markdown source is not UTF-8: {target_entry.source}") from error
    decoded = unquote(fragment)
    anchors = _heading_anchors(target_text)
    exact = [anchor for anchor in anchors if anchor.casefold() == decoded.casefold()]
    if len(exact) == 1:
        return quote(exact[0], safe="-._~")
    alias = EXPLICIT_FRAGMENT_ALIASES.get(
        (referring_page.as_posix(), original_destination)
    )
    if alias is not None and alias.casefold() in {
        anchor.casefold() for anchor in anchors
    }:
        return quote(alias, safe="-._~")
    raise MirrorError(
        f"source anchor does not resolve from {referring_page} to "
        f"{target_entry.source}#{fragment}; no exact anchor or audited alias"
    )


def rewrite_destination(
    destination: str,
    source_page: PurePosixPath,
    snapshot: SourceSnapshot,
    frozen_map: FrozenMap,
) -> str:
    parsed = urlsplit(destination)
    if parsed.scheme:
        if parsed.scheme.lower() not in ALLOWED_EXTERNAL_SCHEMES:
            raise MirrorError(f"unsupported link scheme in {source_page}: {destination}")
        return destination
    if parsed.netloc or destination.startswith("//"):
        raise MirrorError(f"protocol-relative link is not admitted in {source_page}: {destination}")
    if not parsed.path:
        if not parsed.fragment:
            return destination
        current_entry = frozen_map.by_source.get(source_page)
        if current_entry is None or current_entry.kind != "page":
            raise MirrorError(f"self-fragment source is not a mapped page: {source_page}")
        fragment = _resolved_page_fragment(
            parsed.fragment, current_entry, source_page, snapshot, destination
        )
        return _append_query_fragment("", parsed.query, fragment)
    if "\\" in parsed.path or parsed.path.startswith("/"):
        raise MirrorError(f"unsafe or root-relative link in {source_page}: {destination}")
    try:
        decoded_path = unquote(parsed.path, encoding="utf-8", errors="strict")
    except UnicodeError as error:
        raise MirrorError(
            f"link path is not valid percent-encoded UTF-8 in {source_page}: {destination}"
        ) from error
    if decoded_path.startswith("/") or "\\" in decoded_path:
        raise MirrorError(f"decoded link path is unsafe in {source_page}: {destination}")
    source_repo_path = CANONICAL_ROOT / source_page
    resolved_text = posixpath.normpath(
        posixpath.join(source_repo_path.parent.as_posix(), decoded_path)
    )
    if resolved_text == ".." or resolved_text.startswith("../"):
        raise MirrorError(f"link escapes the repository in {source_page}: {destination}")
    resolved = PurePosixPath(resolved_text)
    map_by_source = frozen_map.by_source
    canonical_prefix = CANONICAL_ROOT.as_posix() + "/"
    if resolved.as_posix().startswith(canonical_prefix):
        relative = PurePosixPath(resolved.as_posix()[len(canonical_prefix) :])
        entry = map_by_source.get(relative)
        if entry is not None and entry.kind == "page":
            route = quote(entry.destination.with_suffix("").as_posix(), safe="/-._~")
            fragment = _resolved_page_fragment(
                parsed.fragment, entry, source_page, snapshot, destination
            )
            return _append_query_fragment(route, parsed.query, fragment)
        if entry is not None and entry.kind == "asset":
            return _repo_url(snapshot, resolved, False, parsed.query, parsed.fragment)
        section_index = map_by_source.get(relative / "README.md")
        if section_index is not None and section_index.kind == "page":
            route = quote(section_index.destination.with_suffix("").as_posix(), safe="/-._~")
            fragment = _resolved_page_fragment(
                parsed.fragment, section_index, source_page, snapshot, destination
            )
            return _append_query_fragment(route, parsed.query, fragment)
        if resolved in snapshot.repo_dirs:
            return _repo_url(snapshot, resolved, True, parsed.query, parsed.fragment)
        raise MirrorError(f"unmapped canonical Wiki link in {source_page}: {destination}")
    if resolved in snapshot.repo_files:
        return _repo_url(snapshot, resolved, False, parsed.query, parsed.fragment)
    if resolved in snapshot.repo_dirs:
        return _repo_url(snapshot, resolved, True, parsed.query, parsed.fragment)
    raise MirrorError(f"missing repository link target in {source_page}: {destination}")


def rewrite_markdown_body(
    text: str,
    source_page: PurePosixPath,
    snapshot: SourceSnapshot,
    frozen_map: FrozenMap,
) -> str:
    for token in _markdown_parser().parse(text):
        stack = [token]
        while stack:
            current = stack.pop()
            if getattr(current, "type") in {"html_block", "html_inline"} and re.search(
                r"(?i)\b(?:href|src)\s*=", getattr(current, "content", "")
            ):
                raise MirrorError(
                    f"raw HTML href/src is outside the admitted link grammar in {source_page}"
                )
            stack.extend(getattr(current, "children", None) or [])
    source_links = Counter(_extract_links(text))
    rewriteable_source_links = Counter(_extract_rewriteable_links(text))
    expected_links = Counter(
        rewrite_destination(link, source_page, snapshot, frozen_map)
        for link in source_links.elements()
    )
    replacements: list[tuple[int, int, str, str]] = []
    source_spans = _inline_destination_spans(text) + _reference_destination_spans(text)
    span_links = Counter(item[2] for item in source_spans)
    if span_links != rewriteable_source_links:
        missing = rewriteable_source_links - span_links
        unexpected = span_links - rewriteable_source_links
        raise MirrorError(
            f"CommonMark/source-span disagreement in {source_page}; "
            f"missing={dict(missing)}, unexpected={dict(unexpected)}"
        )
    for start, end, normalized in source_spans:
        rewritten = rewrite_destination(normalized, source_page, snapshot, frozen_map)
        if rewritten != normalized:
            replacements.append((start, end, rewritten, "semantic-link"))

    replacements.sort(key=lambda item: (item[0], item[1]))
    for previous, current in zip(replacements, replacements[1:]):
        if previous[1] > current[0]:
            raise MirrorError(
                f"overlapping link rewrites in {source_page}: {previous} and {current}"
            )
    output = text
    for start, end, replacement, _kind in reversed(replacements):
        output = output[:start] + replacement + output[end:]
    actual_links = Counter(_extract_links(output))
    if actual_links != expected_links:
        missing = expected_links - actual_links
        unexpected = actual_links - expected_links
        raise MirrorError(
            f"CommonMark link rewrite mismatch in {source_page}; missing={dict(missing)}, unexpected={dict(unexpected)}"
        )
    return output


def _canonical_source_url(snapshot: SourceSnapshot, source: PurePosixPath) -> str:
    return _repo_url(snapshot, CANONICAL_ROOT / source, False, "", "")


def _authority_banner(snapshot: SourceSnapshot, source: PurePosixPath) -> str:
    source_url = _canonical_source_url(snapshot, source)
    license_url = _repo_url(snapshot, LICENSE_PATH, False, "", "")
    return (
        "<!-- GENERATED BY github_wiki_mirror.py; DO NOT EDIT IN THE GITHUB WIKI -->\n\n"
        "> [!IMPORTANT]\n"
        "> This page is a generated convenience mirror, not project authority. "
        f"Edit the [canonical source]({source_url}) and regenerate from commit "
        f"`{snapshot.commit}` (Wiki tree `{snapshot.wiki_tree}`). "
        f"The private project has [no blanket MIT license]({license_url}).\n\n"
    )


def _split_front_matter(text: str, source: PurePosixPath) -> tuple[str, str]:
    """Separate delimiter-valid leading metadata so it is retained but not rendered."""

    if text.startswith("\ufeff"):
        raise MirrorError(f"UTF-8 BOM is not admitted in Markdown source: {source}")
    if not text.startswith(("---\n", "---\r\n")):
        return "", text
    lines = text.splitlines(keepends=True)
    end_index: int | None = None
    for index, line in enumerate(lines[1:], 1):
        if line.rstrip("\r\n") in {"---", "..."}:
            end_index = index
            break
    if end_index is None:
        raise MirrorError(f"unterminated YAML front matter: {source}")
    front_matter = "".join(lines[: end_index + 1])
    body = "".join(lines[end_index + 1 :])
    if not body.startswith(("\n", "\r\n")):
        raise MirrorError(f"YAML front matter is not followed by a blank line: {source}")
    return front_matter, body


def _render_source_page(
    snapshot: SourceSnapshot,
    source: SourceFile,
    frozen_map: FrozenMap,
) -> tuple[bytes, bytes]:
    try:
        text = source.data.decode("utf-8", errors="strict")
    except UnicodeError as error:
        raise MirrorError(f"Markdown source is not UTF-8: {source.path}") from error
    front_matter, body = _split_front_matter(text, source.path)
    rewritten_body = rewrite_markdown_body(body, source.path, snapshot, frozen_map)
    output = (_authority_banner(snapshot, source.path) + rewritten_body).encode("utf-8")
    return output, front_matter.encode("utf-8")


def _plain_inline_text(token: object) -> str:
    children = getattr(token, "children", None) or []
    chunks: list[str] = []
    for child in children:
        if child.type in {"text", "code_inline"}:
            chunks.append(child.content)
        elif child.type == "image":
            chunks.append(child.content)
    return "".join(chunks).strip()


def _first_h1(markdown: str, fallback: str) -> str:
    tokens = _markdown_parser().parse(markdown)
    for index, token in enumerate(tokens[:-1]):
        if token.type == "heading_open" and token.tag == "h1" and tokens[index + 1].type == "inline":
            value = _plain_inline_text(tokens[index + 1])
            return value or fallback
    return fallback


def _page_route(destination: PurePosixPath) -> str:
    return quote(destination.with_suffix("").as_posix(), safe="/-._~")


def _render_sidebar(snapshot: SourceSnapshot, frozen_map: FrozenMap) -> bytes:
    by_source = snapshot.by_path
    map_by_source = frozen_map.by_source
    root_order = [
        PurePosixPath("README.md"),
        PurePosixPath("architecture-overview.md"),
        PurePosixPath("evidence-map.md"),
        PurePosixPath("decision-map.md"),
        PurePosixPath("glossary.md"),
        PurePosixPath("archive-index.md"),
    ]
    lines = [
        "<!-- GENERATED BY github_wiki_mirror.py; DO NOT EDIT IN THE GITHUB WIKI -->",
        "",
        "# HaloFPX Wiki mirror",
        "",
        "Generated navigation. The in-repository Wiki is canonical.",
        "",
    ]
    for source_path in root_order:
        entry = map_by_source[source_path]
        title = _first_h1(by_source[source_path].data.decode("utf-8"), source_path.stem)
        lines.append(f"- [{title}]({_page_route(entry.destination)})")
    category_readmes = sorted(
        (
            entry
            for entry in frozen_map.entries
            if entry.kind == "page" and len(entry.source.parts) == 2
        ),
        key=lambda entry: entry.source.as_posix(),
    )
    for category in category_readmes:
        title = _first_h1(by_source[category.source].data.decode("utf-8"), category.source.parent.name)
        lines.extend(["", f"## [{title}]({_page_route(category.destination)})", ""])
        section_readmes = sorted(
            (
                entry
                for entry in frozen_map.entries
                if entry.kind == "page"
                and len(entry.source.parts) == 3
                and entry.source.name == "README.md"
                and entry.source.parts[0] == category.source.parts[0]
            ),
            key=lambda entry: entry.source.as_posix(),
        )
        for section in section_readmes:
            section_title = _first_h1(
                by_source[section.source].data.decode("utf-8"), section.source.parent.name
            )
            lines.append(f"- [{section_title}]({_page_route(section.destination)})")
    return ("\n".join(lines) + "\n").encode("utf-8")


def _render_footer(snapshot: SourceSnapshot) -> bytes:
    canonical_url = _repo_url(snapshot, CANONICAL_ROOT, True, "", "")
    license_url = _repo_url(snapshot, LICENSE_PATH, False, "", "")
    value = (
        "<!-- GENERATED BY github_wiki_mirror.py; DO NOT EDIT IN THE GITHUB WIKI -->\n\n"
        f"Generated from canonical Wiki tree [`{snapshot.wiki_tree}`]({canonical_url}) at "
        f"source commit `{snapshot.commit}`. The GitHub Wiki is a convenience mirror only. "
        f"See [license and provenance]({license_url}); no blanket MIT grant applies.\n"
    )
    return value.encode("utf-8")


def _normalized_generator_sha256() -> str:
    data = Path(__file__).read_bytes().replace(b"\r\n", b"\n")
    return hashlib.sha256(data).hexdigest()


def _records_root(records: Iterable[tuple[str, int, str]]) -> str:
    digest = hashlib.sha256()
    for path, size, sha256 in sorted(records):
        digest.update(path.encode("utf-8"))
        digest.update(b"\0")
        digest.update(str(size).encode("ascii"))
        digest.update(b"\0")
        digest.update(sha256.encode("ascii"))
        digest.update(b"\n")
    return digest.hexdigest()


def _link_audit(
    snapshot: SourceSnapshot,
    frozen_map: FrozenMap,
    rendered_files: Sequence[RenderedFile],
) -> dict[str, object]:
    source_occurrences = 0
    source_unique: set[str] = set()
    repairs: Counter[tuple[str, str, str]] = Counter()
    for entry in frozen_map.entries:
        if entry.kind != "page":
            continue
        source = snapshot.by_path[entry.source]
        text = source.data.decode("utf-8", errors="strict")
        for original in _extract_links(text):
            source_occurrences += 1
            source_unique.add(original)
            rewritten = rewrite_destination(original, entry.source, snapshot, frozen_map)
            before_fragment = unquote(urlsplit(original).fragment)
            after_fragment = unquote(urlsplit(rewritten).fragment)
            if before_fragment.casefold() != after_fragment.casefold():
                repairs[(entry.source.as_posix(), original, rewritten)] += 1

    generated_occurrences = 0
    local_page_occurrences = 0
    pinned_repository_occurrences = 0
    external_occurrences = 0
    pinned_prefixes = (
        f"{REPOSITORY_URL}/blob/{snapshot.commit}/",
        f"{REPOSITORY_URL}/tree/{snapshot.commit}/",
    )
    for item in rendered_files:
        if item.destination.suffix.lower() != ".md":
            continue
        for link in _extract_links(item.data.decode("utf-8", errors="strict")):
            generated_occurrences += 1
            parsed = urlsplit(link)
            if not parsed.scheme:
                local_page_occurrences += 1
            elif link.startswith(pinned_prefixes):
                pinned_repository_occurrences += 1
            else:
                external_occurrences += 1

    return {
        "explicit_fragment_aliases_applied": [
            {
                "occurrences": occurrences,
                "original": original,
                "rewritten": rewritten,
                "source_path": source_path,
            }
            for (source_path, original, rewritten), occurrences in sorted(repairs.items())
        ],
        "generated_external_occurrence_count": external_occurrences,
        "generated_link_occurrence_count": generated_occurrences,
        "generated_local_page_occurrence_count": local_page_occurrences,
        "generated_pinned_repository_occurrence_count": pinned_repository_occurrences,
        "source_link_occurrence_count": source_occurrences,
        "source_unique_destination_count": len(source_unique),
    }


def _build_manifest(
    snapshot: SourceSnapshot,
    frozen_map: FrozenMap,
    rendered_files: Sequence[RenderedFile],
    front_matter_by_source: Mapping[PurePosixPath, bytes],
) -> bytes:
    source_root = _records_root(
        (item.path.as_posix(), item.size, item.sha256) for item in snapshot.files
    )
    output_root = _records_root(
        (item.destination.as_posix(), len(item.data), item.sha256)
        for item in rendered_files
    )
    file_records: list[dict[str, object]] = []
    for item in sorted(rendered_files, key=lambda value: value.destination.as_posix()):
        record: dict[str, object] = {
            "destination_path": item.destination.as_posix(),
            "kind": item.kind,
            "output_sha256": item.sha256,
            "output_size": len(item.data),
            "source_git_blob": None,
            "source_path": None,
            "source_sha256": None,
            "source_size": None,
        }
        if item.source is not None:
            record.update(
                {
                    "source_git_blob": item.source.git_blob,
                    "source_path": item.source.path.as_posix(),
                    "source_sha256": item.source.sha256,
                    "source_size": item.source.size,
                }
            )
            if item.kind == "source-page":
                front_matter = front_matter_by_source[item.source.path]
                record["source_front_matter_sha256"] = hashlib.sha256(
                    front_matter
                ).hexdigest()
                record["source_front_matter_size"] = len(front_matter)
        file_records.append(record)
    manifest = {
        "schema": SCHEMA,
        "authority": {
            "canonical": f"{REPOSITORY_URL}/tree/{snapshot.commit}/{CANONICAL_ROOT.as_posix()}",
            "mirror_role": "generated-convenience-copy",
            "web_edits_allowed": False,
            "license_and_provenance": f"{REPOSITORY_URL}/blob/{snapshot.commit}/{LICENSE_PATH.as_posix()}",
        },
        "generator": {
            "name": "github_wiki_mirror.py",
            "version": GENERATOR_VERSION,
            "sha256_lf_normalized": _normalized_generator_sha256(),
            "commonmark_parser": {
                "distribution": "markdown-it-py",
                "installed_record_set_sha256": RUNTIME_DISTRIBUTIONS["markdown-it-py"][
                    "record_root_sha256"
                ],
                "version": RUNTIME_DISTRIBUTIONS["markdown-it-py"]["version"],
                "wheel_sha256": "9f7ebbcd14fe59494226453aed97c1070d83f8d24b6fc3a3bcf9a38092641c4a",
            },
            "url_dependency": {
                "distribution": "mdurl",
                "installed_record_set_sha256": RUNTIME_DISTRIBUTIONS["mdurl"][
                    "record_root_sha256"
                ],
                "version": RUNTIME_DISTRIBUTIONS["mdurl"]["version"],
                "wheel_sha256": "84008a41e51615a49fc9966191ff91509e3c40b939176e643fd50a5c2196b8f8",
            },
        },
        "mapping": {
            "schema": MAP_SCHEMA,
            "sha256": frozen_map.sha256,
            "source_entry_count": len(frozen_map.entries),
        },
        "links": _link_audit(snapshot, frozen_map, rendered_files),
        "source": {
            "canonical_root": CANONICAL_ROOT.as_posix(),
            "commit": snapshot.commit,
            "git_blob_bytes": sum(item.size for item in snapshot.files),
            "markdown_page_count": sum(item.path.suffix.lower() == ".md" for item in snapshot.files),
            "non_markdown_asset_count": sum(item.path.suffix.lower() != ".md" for item in snapshot.files),
            "repository": REPOSITORY,
            "source_file_count": len(snapshot.files),
            "source_set_sha256": source_root,
            "wiki_tree": snapshot.wiki_tree,
        },
        "output": {
            "file_count_including_manifest": len(rendered_files) + 1,
            "generated_control_page_count": 2,
            "output_set_sha256_excluding_manifest": output_root,
            "rendered_markdown_file_count": sum(
                item.destination.suffix.lower() == ".md" for item in rendered_files
            ),
            "source_asset_count": sum(item.kind == "source-asset" for item in rendered_files),
            "source_page_count": sum(item.kind == "source-page" for item in rendered_files),
        },
        "files": file_records,
    }
    return _canonical_json_bytes(manifest)


def build_mirror(snapshot: SourceSnapshot, frozen_map: FrozenMap) -> RenderedMirror:
    _markdown_parser()
    validate_map_against_snapshot(frozen_map, snapshot)
    map_by_source = frozen_map.by_source
    rendered: list[RenderedFile] = []
    front_matter_by_source: dict[PurePosixPath, bytes] = {}
    for source in snapshot.files:
        entry = map_by_source[source.path]
        if entry.kind == "asset":
            rendered.append(RenderedFile(entry.destination, source.data, "source-asset", source))
            continue
        output, front_matter = _render_source_page(snapshot, source, frozen_map)
        front_matter_by_source[source.path] = front_matter
        rendered.append(RenderedFile(entry.destination, output, "source-page", source))
    rendered.append(
        RenderedFile(PurePosixPath("_Sidebar.md"), _render_sidebar(snapshot, frozen_map), "generated-control")
    )
    rendered.append(
        RenderedFile(PurePosixPath("_Footer.md"), _render_footer(snapshot), "generated-control")
    )
    manifest = _build_manifest(snapshot, frozen_map, rendered, front_matter_by_source)
    rendered.append(
        RenderedFile(PurePosixPath("mirror-manifest.json"), manifest, "generated-manifest")
    )
    mirror = RenderedMirror(tuple(sorted(rendered, key=lambda item: item.destination.as_posix())))
    validate_rendered_mirror(mirror, snapshot, frozen_map)
    return mirror


def _heading_anchors(markdown: str) -> set[str]:
    anchors: set[str] = set()
    casefolded_anchors: set[str] = set()
    next_suffix: Counter[str] = Counter()
    tokens = _markdown_parser().parse(markdown)
    for index, token in enumerate(tokens[:-1]):
        if token.type != "heading_open" or tokens[index + 1].type != "inline":
            continue
        title = html.unescape(_plain_inline_text(tokens[index + 1])).lower()
        title = re.sub(r"<[^>]*>", "", title)
        title = re.sub(r"[^\w\- ]", "", title, flags=re.UNICODE)
        base_slug = title.replace(" ", "-")
        slug = base_slug
        while slug.casefold() in casefolded_anchors:
            next_suffix[base_slug] += 1
            slug = f"{base_slug}-{next_suffix[base_slug]}"
        anchors.add(slug)
        casefolded_anchors.add(slug.casefold())

    class AnchorCollector(HTMLParser):
        def handle_starttag(
            self, _tag: str, attributes: list[tuple[str, str | None]]
        ) -> None:
            for name, value in attributes:
                if name.casefold() in {"id", "name"} and value:
                    anchor = html.unescape(value)
                    if anchor.casefold() in casefolded_anchors:
                        raise MirrorError(f"duplicate rendered anchor: {anchor}")
                    anchors.add(anchor)
                    casefolded_anchors.add(anchor.casefold())

    collector = AnchorCollector(convert_charrefs=True)

    def visit_html(items: Iterable[object]) -> None:
        for item in items:
            if getattr(item, "type") in {"html_block", "html_inline"}:
                collector.feed(getattr(item, "content"))
            children = getattr(item, "children", None)
            if children:
                visit_html(children)

    visit_html(tokens)
    collector.close()
    return anchors


def validate_rendered_mirror(
    mirror: RenderedMirror,
    snapshot: SourceSnapshot,
    frozen_map: FrozenMap,
) -> None:
    by_destination = mirror.by_destination
    if len(by_destination) != len(mirror.files):
        raise MirrorError("rendered mirror contains duplicate destination paths")
    casefolded = [path.as_posix().casefold() for path in by_destination]
    if len(casefolded) != len(set(casefolded)):
        raise MirrorError("rendered mirror contains case-insensitive path collisions")
    expected = {entry.destination for entry in frozen_map.entries} | {
        PurePosixPath(name) for name in SPECIAL_OUTPUTS
    }
    actual = set(by_destination)
    if actual != expected:
        raise MirrorError(
            f"rendered output coverage mismatch; missing={sorted(map(str, expected-actual))}, extra={sorted(map(str, actual-expected))}"
        )
    for entry in frozen_map.entries:
        source = snapshot.by_path[entry.source]
        rendered = by_destination[entry.destination]
        if entry.kind == "asset" and rendered.data != source.data:
            raise MirrorError(f"support asset changed bytes: {entry.source}")
        if entry.kind == "page":
            marker = b"generated convenience mirror, not project authority"
            if marker not in rendered.data:
                raise MirrorError(f"authority banner missing from {entry.destination}")

    page_routes = {
        entry.destination.with_suffix("").as_posix(): entry.destination
        for entry in frozen_map.entries
        if entry.kind == "page"
    }
    page_routes.update({"_Sidebar": PurePosixPath("_Sidebar.md"), "_Footer": PurePosixPath("_Footer.md")})
    anchors: dict[PurePosixPath, set[str]] = {}
    for destination, item in by_destination.items():
        if destination.suffix.lower() == ".md":
            try:
                text = item.data.decode("utf-8", errors="strict")
            except UnicodeError as error:
                raise MirrorError(f"rendered Markdown is not UTF-8: {destination}") from error
            anchors[destination] = _heading_anchors(text)
    for destination, item in by_destination.items():
        if destination.suffix.lower() != ".md":
            continue
        text = item.data.decode("utf-8")
        for link in _extract_links(text):
            parsed = urlsplit(link)
            if parsed.scheme:
                if parsed.scheme.lower() not in ALLOWED_EXTERNAL_SCHEMES:
                    raise MirrorError(f"unsupported generated link scheme in {destination}: {link}")
                continue
            if parsed.netloc or parsed.path.startswith("/") or "\\" in parsed.path or ".." in PurePosixPath(parsed.path).parts:
                raise MirrorError(f"unsafe generated link in {destination}: {link}")
            target = destination
            if parsed.path:
                try:
                    route = unquote(
                        parsed.path, encoding="utf-8", errors="strict"
                    ).removesuffix(".md")
                except UnicodeError as error:
                    raise MirrorError(
                        f"generated link is not valid percent-encoded UTF-8 in {destination}: {link}"
                    ) from error
                target = page_routes.get(route)
                if target is None:
                    raise MirrorError(f"generated Wiki page link does not resolve in {destination}: {link}")
            if parsed.fragment:
                fragment = unquote(parsed.fragment).casefold()
                if fragment not in {value.casefold() for value in anchors.get(target, set())}:
                    raise MirrorError(f"generated anchor does not resolve in {destination}: {link}")

    manifest = json.loads(by_destination[PurePosixPath("mirror-manifest.json")].data)
    if manifest.get("schema") != SCHEMA:
        raise MirrorError("rendered mirror manifest has the wrong schema")
    if manifest["source"]["commit"] != snapshot.commit or manifest["source"]["wiki_tree"] != snapshot.wiki_tree:
        raise MirrorError("rendered mirror manifest source identity mismatch")
    if manifest["source"]["source_file_count"] != len(snapshot.files):
        raise MirrorError("rendered mirror manifest source count mismatch")
    if manifest["output"]["file_count_including_manifest"] != len(mirror.files):
        raise MirrorError("rendered mirror manifest output count mismatch")
    non_manifest = [
        item
        for item in mirror.files
        if item.destination != PurePosixPath("mirror-manifest.json")
    ]
    front_matter_by_source: dict[PurePosixPath, bytes] = {}
    for entry in frozen_map.entries:
        if entry.kind == "page":
            source = snapshot.by_path[entry.source]
            text = source.data.decode("utf-8", errors="strict")
            front_matter, _body = _split_front_matter(text, source.path)
            front_matter_by_source[source.path] = front_matter.encode("utf-8")
    expected_manifest = _build_manifest(
        snapshot, frozen_map, non_manifest, front_matter_by_source
    )
    if by_destination[PurePosixPath("mirror-manifest.json")].data != expected_manifest:
        raise MirrorError("rendered mirror manifest does not exactly bind the output")


def _git_administration_roots(repo_root: Path) -> tuple[Path, ...]:
    values = {
        (repo_root / ".git").absolute(),
        Path(
            _run_git(repo_root, "rev-parse", "--absolute-git-dir")
            .decode("utf-8", errors="strict")
            .strip()
        ).resolve(),
        Path(
            _run_git(
                repo_root,
                "rev-parse",
                "--path-format=absolute",
                "--git-common-dir",
            )
            .decode("utf-8", errors="strict")
            .strip()
        ).resolve(),
    }
    if any(not value.is_absolute() for value in values):
        raise MirrorError(f"Git returned a non-absolute administration path: {values}")
    return tuple(sorted(values, key=str))


def _refuse_protected_output(output: Path, repo_root: Path, role: str) -> Path:
    lexical = _lexical_absolute(output)
    _reject_reparse_chain(lexical)
    resolved = lexical.resolve()
    repository = repo_root.resolve()
    repository_paths = {repository, _lexical_absolute(repo_root)}
    blocked_roots = {
        (repository / CANONICAL_ROOT).resolve(),
        _lexical_absolute(repository / CANONICAL_ROOT),
        *_git_administration_roots(repository),
    }
    candidates = {lexical, resolved}
    if (
        any(candidate.parent == candidate for candidate in candidates)
        or candidates & repository_paths
        or any(candidate.is_relative_to(root) for candidate in candidates for root in blocked_roots)
    ):
        raise MirrorError(f"refusing unsafe {role}: {lexical}")
    return resolved


def _is_reparse_path(path: Path) -> bool:
    if path.is_symlink():
        return True
    is_junction = getattr(path, "is_junction", None)
    if is_junction is not None and is_junction():
        return True
    try:
        attributes = getattr(path.lstat(), "st_file_attributes", 0)
    except FileNotFoundError:
        return False
    except OSError as error:
        raise MirrorError(f"cannot inspect staged path without following links: {path}: {error}") from error
    return bool(attributes & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0))


def _lexical_absolute(path: Path) -> Path:
    return Path(os.path.abspath(os.fspath(path)))


def _reject_reparse_chain(path: Path) -> None:
    current = _lexical_absolute(path)
    while True:
        if _is_reparse_path(current):
            raise MirrorError(f"staged path or ancestor is a symlink/reparse point: {current}")
        if current.parent == current:
            break
        current = current.parent


def _closed_tree_paths(output: Path) -> tuple[dict[PurePosixPath, Path], set[PurePosixPath]]:
    """Enumerate a staging tree without following symlinks, junctions, or reparse points."""

    output = _lexical_absolute(output)
    _reject_reparse_chain(output)
    if not output.is_dir():
        raise MirrorError(f"mirror output directory does not exist: {output}")
    files: dict[PurePosixPath, Path] = {}
    directories: set[PurePosixPath] = set()
    pending = [output]
    while pending:
        directory = pending.pop()
        try:
            entries = list(os.scandir(directory))
        except OSError as error:
            raise MirrorError(f"cannot enumerate mirror output directory {directory}: {error}") from error
        for entry in entries:
            path = Path(entry.path)
            if _is_reparse_path(path):
                raise MirrorError(f"mirror output contains a symlink/reparse point: {path}")
            relative = PurePosixPath(path.relative_to(output).as_posix())
            if entry.is_dir(follow_symlinks=False):
                directories.add(relative)
                pending.append(path)
            elif entry.is_file(follow_symlinks=False):
                files[relative] = path
            else:
                raise MirrorError(f"mirror output contains a non-regular entry: {path}")
    return files, directories


def _expected_directories(paths: Iterable[PurePosixPath]) -> set[PurePosixPath]:
    directories: set[PurePosixPath] = set()
    for path in paths:
        parent = path.parent
        while parent != PurePosixPath("."):
            directories.add(parent)
            parent = parent.parent
    return directories


def _write_new_file(path: Path, data: bytes, role: str) -> None:
    """Create one file without following or replacing an intervening path."""

    try:
        with path.open("xb") as stream:
            stream.write(data)
    except FileExistsError as error:
        raise MirrorError(f"{role} path already exists; choose a new path: {path}") from error
    except OSError as error:
        raise MirrorError(f"cannot create {role} path {path}: {error}") from error


def _publish_no_replace(staging: Path, output: Path, role: str) -> None:
    """Atomically publish one staged file or directory only if output is absent."""

    try:
        if os.name == "nt":
            os.rename(staging, output)
            return
        if sys.platform.startswith("linux"):
            library = ctypes.CDLL(None, use_errno=True)
            try:
                renameat2 = library.renameat2
            except AttributeError as error:
                raise MirrorError("Linux runtime lacks atomic renameat2 no-replace support") from error
            renameat2.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_uint]
            renameat2.restype = ctypes.c_int
            at_fdcwd = -100
            rename_noreplace = 1
            result = renameat2(
                at_fdcwd,
                os.fsencode(staging),
                at_fdcwd,
                os.fsencode(output),
                rename_noreplace,
            )
            if result == 0:
                return
            error_number = ctypes.get_errno()
            raise OSError(error_number, os.strerror(error_number), output)
        raise MirrorError(f"atomic no-replace publication is unsupported on {sys.platform}")
    except (FileExistsError, IsADirectoryError, NotADirectoryError) as error:
        raise MirrorError(f"{role} path already exists; choose a new path: {output}") from error
    except OSError as error:
        if error.errno in {errno.EEXIST, errno.ENOTEMPTY}:
            raise MirrorError(f"{role} path already exists; choose a new path: {output}") from error
        raise MirrorError(f"cannot atomically publish {role} path {output}: {error}") from error


def _require_safe_existing_parent(output: Path, role: str) -> Path:
    parent = output.parent
    _reject_reparse_chain(parent)
    if not parent.is_dir():
        raise MirrorError(f"{role} parent directory must already exist: {parent}")
    return parent


def write_mirror(mirror: RenderedMirror, output: Path, repo_root: Path) -> None:
    output = _refuse_protected_output(output, repo_root, "output directory")
    parent = _require_safe_existing_parent(output, "output")
    staging = Path(tempfile.mkdtemp(prefix=".halofpx-wiki-stage-", dir=parent))
    _reject_reparse_chain(staging)
    for item in mirror.files:
        _reject_reparse_chain(staging)
        destination = staging / Path(*item.destination.parts)
        destination.parent.mkdir(parents=True, exist_ok=True)
        _reject_reparse_chain(destination.parent)
        _write_new_file(destination, item.data, "mirror file")
    compare_output(mirror, staging)
    _reject_reparse_chain(parent)
    _publish_no_replace(staging, output, "output")
    compare_output(mirror, output)


def compare_output(mirror: RenderedMirror, output: Path) -> None:
    actual_files, actual_directories = _closed_tree_paths(output)
    actual_paths = set(actual_files)
    expected_paths = set(mirror.by_destination)
    if actual_paths != expected_paths:
        raise MirrorError(
            f"output file-set mismatch; missing={sorted(map(str, expected_paths-actual_paths))}, extra={sorted(map(str, actual_paths-expected_paths))}"
        )
    expected_directories = _expected_directories(expected_paths)
    if actual_directories != expected_directories:
        raise MirrorError(
            "output directory-set mismatch; "
            f"missing={sorted(map(str, expected_directories-actual_directories))}, "
            f"extra={sorted(map(str, actual_directories-expected_directories))}"
        )
    for destination, expected in mirror.by_destination.items():
        path = actual_files[destination]
        actual = path.read_bytes()
        if actual != expected.data:
            raise MirrorError(
                f"output content mismatch: {destination}; expected={expected.sha256}, actual={hashlib.sha256(actual).hexdigest()}"
            )


def _require_mapping_keys(value: object, keys: set[str], role: str) -> dict[str, object]:
    if not isinstance(value, dict) or set(value) != keys:
        raise MirrorError(f"historical mirror {role} has invalid fields")
    return value


def _require_hash(value: object, length: int, role: str) -> str:
    if not isinstance(value, str) or re.fullmatch(rf"[0-9a-f]{{{length}}}", value) is None:
        raise MirrorError(f"historical mirror {role} is not a {length}-character lowercase hash")
    return value


def _require_count(value: object, role: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        raise MirrorError(f"historical mirror {role} is not a non-negative integer")
    return value


def _validate_historical_manifest_shape(manifest: object, manifest_bytes: bytes) -> dict[str, object]:
    top = _require_mapping_keys(
        manifest,
        {"authority", "files", "generator", "links", "mapping", "output", "schema", "source"},
        "manifest",
    )
    if manifest_bytes != _canonical_json_bytes(top):
        raise MirrorError("historical mirror manifest is not in its one canonical JSON encoding")
    if top["schema"] != SCHEMA:
        raise MirrorError("historical mirror manifest has an unsupported schema")

    source = _require_mapping_keys(
        top["source"],
        {
            "canonical_root",
            "commit",
            "git_blob_bytes",
            "markdown_page_count",
            "non_markdown_asset_count",
            "repository",
            "source_file_count",
            "source_set_sha256",
            "wiki_tree",
        },
        "source",
    )
    source_commit = _require_hash(source["commit"], 40, "source commit")
    _require_hash(source["wiki_tree"], 40, "source Wiki tree")
    _require_hash(source["source_set_sha256"], 64, "source-set SHA-256")
    for name in (
        "git_blob_bytes",
        "markdown_page_count",
        "non_markdown_asset_count",
        "source_file_count",
    ):
        _require_count(source[name], f"source {name}")
    if source["canonical_root"] != CANONICAL_ROOT.as_posix() or source["repository"] != REPOSITORY:
        raise MirrorError("historical mirror source authority is for a different repository/root")
    if source["markdown_page_count"] + source["non_markdown_asset_count"] != source["source_file_count"]:
        raise MirrorError("historical mirror source counts do not reconcile")

    authority = _require_mapping_keys(
        top["authority"],
        {"canonical", "license_and_provenance", "mirror_role", "web_edits_allowed"},
        "authority",
    )
    expected_canonical = f"{REPOSITORY_URL}/tree/{source_commit}/{CANONICAL_ROOT.as_posix()}"
    expected_license = f"{REPOSITORY_URL}/blob/{source_commit}/{LICENSE_PATH.as_posix()}"
    if authority != {
        "canonical": expected_canonical,
        "license_and_provenance": expected_license,
        "mirror_role": "generated-convenience-copy",
        "web_edits_allowed": False,
    }:
        raise MirrorError("historical mirror authority fields do not bind the source commit")

    generator = _require_mapping_keys(
        top["generator"],
        {"commonmark_parser", "name", "sha256_lf_normalized", "url_dependency", "version"},
        "generator",
    )
    if generator["name"] != "github_wiki_mirror.py" or not isinstance(generator["version"], str) or not generator["version"]:
        raise MirrorError("historical mirror generator identity is invalid")
    _require_hash(generator["sha256_lf_normalized"], 64, "generator SHA-256")
    for role in ("commonmark_parser", "url_dependency"):
        dependency = _require_mapping_keys(
            generator[role],
            {"distribution", "installed_record_set_sha256", "version", "wheel_sha256"},
            f"generator {role}",
        )
        if not isinstance(dependency["distribution"], str) or not dependency["distribution"]:
            raise MirrorError(f"historical mirror generator {role} distribution is invalid")
        if not isinstance(dependency["version"], str) or not dependency["version"]:
            raise MirrorError(f"historical mirror generator {role} version is invalid")
        _require_hash(dependency["installed_record_set_sha256"], 64, f"generator {role} RECORD root")
        _require_hash(dependency["wheel_sha256"], 64, f"generator {role} wheel SHA-256")

    mapping = _require_mapping_keys(
        top["mapping"], {"schema", "sha256", "source_entry_count"}, "mapping"
    )
    if mapping["schema"] != MAP_SCHEMA:
        raise MirrorError("historical mirror mapping schema is invalid")
    _require_hash(mapping["sha256"], 64, "mapping SHA-256")
    if _require_count(mapping["source_entry_count"], "mapping source count") != source["source_file_count"]:
        raise MirrorError("historical mirror mapping/source counts differ")

    links = _require_mapping_keys(
        top["links"],
        {
            "explicit_fragment_aliases_applied",
            "generated_external_occurrence_count",
            "generated_link_occurrence_count",
            "generated_local_page_occurrence_count",
            "generated_pinned_repository_occurrence_count",
            "source_link_occurrence_count",
            "source_unique_destination_count",
        },
        "links",
    )
    for name in links:
        if name != "explicit_fragment_aliases_applied":
            _require_count(links[name], f"links {name}")
    aliases = links["explicit_fragment_aliases_applied"]
    if not isinstance(aliases, list):
        raise MirrorError("historical mirror fragment aliases must be a list")
    for index, alias in enumerate(aliases):
        item = _require_mapping_keys(
            alias, {"occurrences", "original", "rewritten", "source_path"}, f"fragment alias {index}"
        )
        if _require_count(item["occurrences"], f"fragment alias {index} occurrences") == 0:
            raise MirrorError("historical mirror fragment alias has zero occurrences")
        if any(not isinstance(item[name], str) or not item[name] for name in ("original", "rewritten", "source_path")):
            raise MirrorError(f"historical mirror fragment alias {index} is invalid")
    generated_count = links["generated_link_occurrence_count"]
    if generated_count != sum(
        links[name]
        for name in (
            "generated_external_occurrence_count",
            "generated_local_page_occurrence_count",
            "generated_pinned_repository_occurrence_count",
        )
    ):
        raise MirrorError("historical mirror generated link counts do not reconcile")
    output = _require_mapping_keys(
        top["output"],
        {
            "file_count_including_manifest",
            "generated_control_page_count",
            "output_set_sha256_excluding_manifest",
            "rendered_markdown_file_count",
            "source_asset_count",
            "source_page_count",
        },
        "output",
    )
    for name in output:
        if name != "output_set_sha256_excluding_manifest":
            _require_count(output[name], f"output {name}")
    _require_hash(output["output_set_sha256_excluding_manifest"], 64, "output-set SHA-256")
    if output["source_page_count"] != source["markdown_page_count"] or output["source_asset_count"] != source["non_markdown_asset_count"]:
        raise MirrorError("historical mirror source/output class counts differ")
    if output["rendered_markdown_file_count"] != output["source_page_count"] + output["generated_control_page_count"]:
        raise MirrorError("historical mirror rendered Markdown count does not reconcile")

    if not isinstance(top["files"], list):
        raise MirrorError("historical mirror manifest files must be a list")
    return top


def audit_manifest_output(output: Path) -> dict[str, object]:
    """Audit a historical mirror against only the manifest committed beside it."""

    actual_files, actual_directories = _closed_tree_paths(output)
    if len(actual_files) != len(
        {path.as_posix().casefold() for path in actual_files}
    ):
        raise MirrorError("historical mirror contains case-insensitive path collisions")
    manifest_path = PurePosixPath("mirror-manifest.json")
    if manifest_path not in actual_files:
        raise MirrorError("historical mirror lacks mirror-manifest.json")
    try:
        manifest_bytes = actual_files[manifest_path].read_bytes()
        manifest = json.loads(manifest_bytes.decode("utf-8", errors="strict"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise MirrorError(f"cannot parse historical mirror manifest: {error}") from error
    manifest = _validate_historical_manifest_shape(manifest, manifest_bytes)
    records = manifest.get("files")
    if not isinstance(records, list):
        raise MirrorError("historical mirror manifest files must be a list")
    if manifest["output"]["file_count_including_manifest"] != len(records) + 1:
        raise MirrorError("historical mirror output file count does not match its records")
    expected_files = {manifest_path}
    aggregate_records: list[tuple[str, int, str]] = []
    source_records: list[tuple[str, int, str]] = []
    source_paths: set[PurePosixPath] = set()
    source_casefolds: set[str] = set()
    source_git_blob_bytes = 0
    kind_counts: Counter[str] = Counter()
    for index, record in enumerate(records):
        base_keys = {
            "destination_path",
            "kind",
            "output_sha256",
            "output_size",
            "source_git_blob",
            "source_path",
            "source_sha256",
            "source_size",
        }
        if not isinstance(record, dict) or record.get("kind") not in {
            "generated-control",
            "source-asset",
            "source-page",
        }:
            raise MirrorError(f"historical mirror manifest file record {index} is invalid")
        expected_record_keys = base_keys | (
            {"source_front_matter_sha256", "source_front_matter_size"}
            if record["kind"] == "source-page"
            else set()
        )
        if set(record) != expected_record_keys:
            raise MirrorError(f"historical mirror manifest file record {index} has invalid fields")
        kind_counts[record["kind"]] += 1
        destination_text = record.get("destination_path")
        expected_size = record.get("output_size")
        expected_sha256 = record.get("output_sha256")
        if (
            not isinstance(destination_text, str)
            or not isinstance(expected_sha256, str)
            or re.fullmatch(r"[0-9a-f]{64}", expected_sha256) is None
        ):
            raise MirrorError(f"historical mirror manifest file record {index} is invalid")
        expected_size = _require_count(expected_size, f"file record {index} output size")
        if record["kind"] == "generated-control":
            if any(record[name] is not None for name in ("source_git_blob", "source_path", "source_sha256", "source_size")):
                raise MirrorError(f"historical mirror generated record {index} claims source authority")
        else:
            _require_hash(record["source_git_blob"], 40, f"file record {index} source Git blob")
            _require_hash(record["source_sha256"], 64, f"file record {index} source SHA-256")
            _require_count(record["source_size"], f"file record {index} source size")
            if not isinstance(record["source_path"], str):
                raise MirrorError(f"historical mirror file record {index} source path is invalid")
            source_path = PurePosixPath(record["source_path"])
            _validate_portable_path(source_path, record["source_path"], f"historical source record {index}")
            source_casefold = source_path.as_posix().casefold()
            if source_path in source_paths or source_casefold in source_casefolds:
                raise MirrorError(f"duplicate/colliding historical source path: {source_path}")
            source_paths.add(source_path)
            source_casefolds.add(source_casefold)
            source_size = record["source_size"]
            source_sha256 = record["source_sha256"]
            source_git_blob_bytes += source_size
            source_records.append((source_path.as_posix(), source_size, source_sha256))
            if (record["kind"] == "source-page") != (source_path.suffix.lower() == ".md"):
                raise MirrorError(f"historical source kind/extension mismatch: {source_path}")
            if record["kind"] == "source-page":
                _require_hash(
                    record["source_front_matter_sha256"],
                    64,
                    f"file record {index} front-matter SHA-256",
                )
                _require_count(
                    record["source_front_matter_size"],
                    f"file record {index} front-matter size",
                )
        destination = PurePosixPath(destination_text)
        _validate_portable_path(
            destination, destination_text, f"historical manifest destination {index}"
        )
        if destination == manifest_path:
            raise MirrorError(f"unsafe historical manifest destination: {destination_text}")
        if destination in expected_files:
            raise MirrorError(f"duplicate historical manifest destination: {destination_text}")
        expected_files.add(destination)
        path = actual_files.get(destination)
        if path is None:
            raise MirrorError(f"historical mirror is missing {destination_text}")
        data = path.read_bytes()
        actual_sha256 = hashlib.sha256(data).hexdigest()
        if len(data) != expected_size or actual_sha256 != expected_sha256:
            raise MirrorError(
                f"historical mirror content mismatch: {destination_text}; "
                f"expected={expected_size}/{expected_sha256}, actual={len(data)}/{actual_sha256}"
            )
        aggregate_records.append((destination_text, expected_size, expected_sha256))
    if kind_counts != Counter(
        {
            "generated-control": manifest["output"]["generated_control_page_count"],
            "source-asset": manifest["output"]["source_asset_count"],
            "source-page": manifest["output"]["source_page_count"],
        }
    ):
        raise MirrorError("historical mirror file-record classes do not match output counts")
    if len(source_records) != manifest["source"]["source_file_count"]:
        raise MirrorError("historical mirror source-record count does not match source authority")
    if source_git_blob_bytes != manifest["source"]["git_blob_bytes"]:
        raise MirrorError("historical mirror source byte count does not match source records")
    if _records_root(source_records) != manifest["source"]["source_set_sha256"]:
        raise MirrorError("historical mirror source-set hash does not match source records")
    if set(actual_files) != expected_files:
        raise MirrorError(
            "historical mirror file-set mismatch; "
            f"missing={sorted(map(str, expected_files-set(actual_files)))}, "
            f"extra={sorted(map(str, set(actual_files)-expected_files))}"
        )
    expected_directories = _expected_directories(expected_files)
    if actual_directories != expected_directories:
        raise MirrorError(
            "historical mirror directory-set mismatch; "
            f"missing={sorted(map(str, expected_directories-actual_directories))}, "
            f"extra={sorted(map(str, actual_directories-expected_directories))}"
        )
    if len(expected_files) != manifest.get("output", {}).get("file_count_including_manifest"):
        raise MirrorError("historical mirror manifest file count mismatch")
    aggregate = _records_root(aggregate_records)
    if aggregate != manifest.get("output", {}).get("output_set_sha256_excluding_manifest"):
        raise MirrorError("historical mirror aggregate hash mismatch")
    return {
        "file_count": len(expected_files),
        "manifest_sha256": hashlib.sha256(manifest_bytes).hexdigest(),
        "output_set_sha256_excluding_manifest": aggregate,
        "source_commit": manifest.get("source", {}).get("commit"),
        "source_wiki_tree": manifest.get("source", {}).get("wiki_tree"),
        "status": "OK",
    }


def _summary(snapshot: SourceSnapshot, frozen_map: FrozenMap, mirror: RenderedMirror) -> dict[str, object]:
    manifest_file = mirror.by_destination[PurePosixPath("mirror-manifest.json")]
    return {
        "asset_count": frozen_map.expected_asset_count,
        "manifest_sha256": manifest_file.sha256,
        "mirror_file_count": len(mirror.files),
        "page_count": frozen_map.expected_markdown_page_count,
        "source_commit": snapshot.commit,
        "source_file_count": len(snapshot.files),
        "source_git_blob_bytes": sum(item.size for item in snapshot.files),
        "source_wiki_tree": snapshot.wiki_tree,
        "status": "OK",
    }


def _default_map_path(repo_root: Path) -> Path:
    return repo_root / "project/project-management/documentation/github-wiki-page-map.json"


def _create_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=Path.cwd(), help="repository or path inside it")
    subparsers = parser.add_subparsers(dest="command", required=True)

    freeze = subparsers.add_parser("freeze-map", help="create a new deterministic mapping registry")
    freeze.add_argument("--source-ref", default="HEAD")
    freeze.add_argument("--output", type=Path, required=True)

    audit = subparsers.add_parser(
        "audit-manifest", help="audit an existing historical mirror from its own manifest"
    )
    audit.add_argument("--output", type=Path, required=True)

    for command in ("generate", "validate", "verify"):
        subparser = subparsers.add_parser(command)
        subparser.add_argument("--source-ref", default="HEAD")
        subparser.add_argument("--page-map", type=Path)
        if command in {"generate", "validate"}:
            subparser.add_argument("--output", type=Path, required=True)
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = _create_parser()
    args = parser.parse_args(argv)
    try:
        repo_root = find_repo_root(args.repo)
        if args.command == "audit-manifest":
            print(json.dumps(audit_manifest_output(args.output), indent=2, sort_keys=True))
            return 0
        snapshot = load_snapshot(repo_root, args.source_ref)
        if args.command == "freeze-map":
            output = _refuse_protected_output(args.output, repo_root, "page-map output")
            parent = _require_safe_existing_parent(output, "page-map output")
            data = _canonical_json_bytes(derive_mapping(snapshot))
            descriptor, staging_text = tempfile.mkstemp(prefix=".halofpx-wiki-map-stage-", dir=parent)
            staging = Path(staging_text)
            _reject_reparse_chain(staging)
            with os.fdopen(descriptor, "wb") as stream:
                stream.write(data)
            _reject_reparse_chain(parent)
            _publish_no_replace(staging, output, "page-map output")
            if output.read_bytes() != data:
                raise MirrorError(f"published page-map bytes changed unexpectedly: {output}")
            print(json.dumps({"status": "OK", "output": str(output)}, sort_keys=True))
            return 0
        page_map_path = (args.page_map or _default_map_path(repo_root)).resolve()
        frozen_map = load_frozen_map(page_map_path)
        first = build_mirror(snapshot, frozen_map)
        if args.command == "generate":
            write_mirror(first, args.output, repo_root)
        elif args.command == "validate":
            compare_output(first, args.output)
        elif args.command == "verify":
            second = build_mirror(snapshot, frozen_map)
            if first.by_destination.keys() != second.by_destination.keys():
                raise MirrorError("deterministic regeneration changed the output file set")
            for destination, first_file in first.by_destination.items():
                if first_file.data != second.by_destination[destination].data:
                    raise MirrorError(f"deterministic regeneration changed {destination}")
        print(json.dumps(_summary(snapshot, frozen_map, first), indent=2, sort_keys=True))
        return 0
    except MirrorError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
