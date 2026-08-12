#!/usr/bin/env python3
"""Validate HaloFPX documentation navigation and preservation rules."""

from __future__ import annotations

import hashlib
import json
import re
import subprocess
import sys
from collections import Counter, defaultdict
from pathlib import Path
from urllib.parse import unquote


ROOT = Path(__file__).resolve().parents[2]
WIKI = ROOT / "wiki" / "HaloFPX_Wiki"
INVENTORY = Path(__file__).with_name("document-inventory.json")

CLAIM_LABELS = (
    "[MEASURED]",
    "[VERIFIED]",
    "[INFERENCE]",
    "[ASSUMPTION]",
    "[RECOMMENDATION]",
    "[OPEN]",
)
MANIFEST_FIELDS = (
    "Purpose",
    "Authoritative files",
    "Current owner",
    "Status",
    "Last verified date",
    "Source commits",
    "Related decisions",
    "Related evidence",
    "Open work",
    "Next safe action",
)
ROOT_ROUTES = (
    "Start Here",
    "Current Project State",
    "Goals and Non-Negotiable Rules",
    "System Architecture",
    "Production Operations",
    "Cache Design",
    "Distributed Execution",
    "Performance Results",
    "Milestones and Decisions",
    "Evidence Index",
    "Worker Guide",
    "Glossary",
    "Archive",
)
ENTRY_POINTS = {
    "README.md",
    "WORKER_START_HERE.md",
    "CURRENT_STATE.md",
    "PROJECT_GOAL.md",
    "AGENTS.md",
    "wiki/HaloFPX_Wiki/README.md",
    "project-management/lead/README.md",
    "project-management/lead/OBJECTIVES.md",
    "project-management/lead/CURRENT_STATUS.md",
    "project-management/lead/MONITORING.md",
    "project-management/lead/DECISIONS.md",
    "project-management/lead/worker-specs/DOCUMENTATION_STE_ORGANIZATION_TASK.md",
    "project-management/lead/worker-specs/L111_VISIBLE_IMPLEMENTATION_TASK.md",
}
LINK_RE = re.compile(r"(?<!!)\[[^\]]*\]\(([^)]+)\)")
SENTENCE_RE = re.compile(r"(?<=[.!?])\s+")
ABBREVIATION_RE = re.compile(r"\b[A-Z][A-Z0-9]{1,7}\b")
PASSIVE_RE = re.compile(
    r"\b(?:is|are|was|were|be|been|being)\s+"
    r"(?:accepted|approved|bound|built|called|classified|closed|committed|"
    r"completed|defined|derived|generated|linked|measured|opened|preserved|"
    r"recorded|required|retained|selected|stated|used|validated|verified)\b",
    re.IGNORECASE,
)
AMBIGUOUS_RE = re.compile(r"\b(?:it|this|that|they)\b", re.IGNORECASE)
VAGUE_RE = re.compile(r"\b(?:latest|better|works|recent|normal|fast|large|safe)\b", re.IGNORECASE)
IDENTIFIER_RE = re.compile(r"\b(?:[0-9a-f]{40}|[0-9a-f]{64})\b", re.IGNORECASE)
MEASUREMENT_RE = re.compile(
    r"\b\d+(?:\.\d+)?\s*(?:tokens?/s|ms|GiB|MiB|GB|MB|W|%)\b",
    re.IGNORECASE,
)
VERSION_RE = re.compile(r"\bv?\d+\.\d+(?:\.\d+)?(?:-[A-Za-z0-9.]+)?\b")
QUALIFIER_RE = re.compile(
    r"\b(?:not accepted|not approved|not promoted|no accepted|remain open|"
    r"remains open|unapproved|pending|failed|needs-machine-validation)\b",
    re.IGNORECASE,
)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def run_git(*args: str) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=ROOT,
        text=True,
        encoding="utf-8",
        errors="replace",
        capture_output=True,
        check=True,
    )
    return result.stdout


def resolve_link(source: Path, raw_target: str) -> tuple[str, bool]:
    target = raw_target.strip().strip("<>")
    if not target or target.startswith(("#", "http://", "https://", "mailto:", "data:", "app://")):
        return target, True
    target = unquote(target.split("#", 1)[0].split("?", 1)[0])
    if not target:
        return target, True
    if re.match(r"^[A-Za-z]:[\\/]", target):
        path = Path(target)
        return str(path), path.exists()
    path = (source.parent / target).resolve()
    return str(path), path.exists()


def markdown_files() -> list[Path]:
    files = [p for p in WIKI.rglob("*.md") if p.is_file()]
    files.extend(p for p in ROOT.glob("*.md") if p.is_file())
    files.extend(
        p for p in (ROOT / "project-management").rglob("*.md") if p.is_file()
    )
    return sorted(set(files))


def check_links(files: list[Path]) -> tuple[list[str], dict[str, list[str]]]:
    errors: list[str] = []
    inbound: dict[str, list[str]] = defaultdict(list)
    for source in files:
        text = source.read_text(encoding="utf-8")
        for raw in LINK_RE.findall(text):
            target_text, exists = resolve_link(source, raw)
            if not exists:
                errors.append(f"{rel(source)} -> {raw}")
                continue
            try:
                target_rel = Path(target_text).resolve().relative_to(ROOT).as_posix()
            except (ValueError, OSError):
                continue
            inbound[target_rel].append(rel(source))
    return errors, inbound


def check_orphans(files: list[Path], inbound: dict[str, list[str]]) -> list[str]:
    errors: list[str] = []
    for path in files:
        rp = rel(path)
        authoritative = (
            rp in ENTRY_POINTS
            or rp.startswith("wiki/HaloFPX_Wiki/") and path.name == "README.md"
            or rp in {
                "wiki/HaloFPX_Wiki/architecture-overview.md",
                "wiki/HaloFPX_Wiki/evidence-map.md",
                "wiki/HaloFPX_Wiki/decision-map.md",
                "wiki/HaloFPX_Wiki/glossary.md",
                "wiki/HaloFPX_Wiki/archive-index.md",
            }
        )
        if authoritative and rp not in ENTRY_POINTS and not inbound.get(rp):
            errors.append(rp)
    return errors


def check_manifests() -> list[str]:
    errors: list[str] = []
    manifests = sorted(path / "README.md" for path in WIKI.iterdir() if path.is_dir())
    if len(manifests) != 12:
        errors.append(f"expected 12 category manifests, found {len(manifests)}")
    for manifest in manifests:
        text = manifest.read_text(encoding="utf-8")
        for field in MANIFEST_FIELDS:
            if f"**{field}:**" not in text:
                errors.append(f"{rel(manifest)}: missing {field}")
    return errors


def check_root_routes() -> list[str]:
    text = (ROOT / "README.md").read_text(encoding="utf-8")
    return [route for route in ROOT_ROUTES if f"**{route}:**" not in text]


def check_worker_start() -> list[str]:
    errors: list[str] = []
    worker = (ROOT / "WORKER_START_HERE.md").read_text(encoding="utf-8")
    agents = (ROOT / "AGENTS.md").read_text(encoding="utf-8")
    required = (
        "AGENTS.md",
        "PROJECT_GOAL.md",
        "CURRENT_STATE.md",
        "CURRENT_STATUS.md",
        "DECISIONS.md",
        "category manifest",
        "feature-off",
        "cleanup",
    )
    for term in required:
        if term.lower() not in worker.lower():
            errors.append(f"WORKER_START_HERE.md: missing {term}")
    if "Start with `WORKER_START_HERE.md`" not in agents:
        errors.append("AGENTS.md does not require WORKER_START_HERE.md")
    return errors


def protected_digests() -> tuple[int, str, str]:
    metadata_digest = hashlib.sha256()
    content_digest = hashlib.sha256()
    count = 0
    paths: list[Path] = []
    for top in ("sources", "experiments"):
        base = ROOT / top
        if not base.exists():
            continue
        paths.extend(p for p in base.rglob("*") if p.is_file())
    for path in sorted(paths, key=lambda p: rel(p).lower()):
        stat = path.stat()
        row = f"{rel(path)}\0{stat.st_size}\0{stat.st_mtime_ns}\n"
        metadata_digest.update(row.encode("utf-8"))
        content_digest.update(rel(path).encode("utf-8"))
        content_digest.update(b"\0")
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(8 * 1024 * 1024), b""):
                content_digest.update(chunk)
        content_digest.update(b"\n")
        count += 1
    return count, metadata_digest.hexdigest(), content_digest.hexdigest()


def check_protected(inventory: dict[str, object]) -> list[str]:
    errors: list[str] = []
    snapshot = inventory["protected_repository_snapshot"]
    count, metadata_digest, content_digest = protected_digests()
    if count != snapshot["file_count"]:
        errors.append(f"protected file count changed: {snapshot['file_count']} -> {count}")
    if metadata_digest != snapshot["metadata_digest_sha256"]:
        errors.append("protected repository metadata digest changed")
    if content_digest != snapshot["content_digest_sha256"]:
        errors.append("protected repository content digest changed")
    baseline_status = inventory.get("baseline_protected_git_status")
    if baseline_status is not None:
        current_status = sorted(
            line for line in run_git("status", "--short", "--", "sources", "experiments").splitlines() if line
        )
        if current_status != baseline_status:
            errors.append("protected Git status changed from the inventory baseline")
    return errors


def semantic_tokens(text: str) -> Counter[str]:
    tokens: list[str] = []
    tokens.extend(IDENTIFIER_RE.findall(text))
    tokens.extend(MEASUREMENT_RE.findall(text))
    tokens.extend(VERSION_RE.findall(text))
    return Counter(token.lower() for token in tokens)


def claim_lines(text: str) -> Counter[str]:
    return Counter(
        re.sub(r"^\s*\d+\.\s*", "", re.sub(r"\s+", " ", line)).strip()
        for line in text.splitlines()
        if any(label in line for label in CLAIM_LABELS)
    )


def check_claim_diff(inventory: dict[str, object]) -> list[str]:
    errors: list[str] = []
    baseline = str(inventory["repository_head"])
    for name in run_git("diff", "--name-only", baseline, "--", "*.md").splitlines():
        path = ROOT / name
        if not path.exists():
            continue
        current = path.read_text(encoding="utf-8")
        base_result = subprocess.run(
            ["git", "show", f"{baseline}:{name}"],
            cwd=ROOT,
            text=True,
            encoding="utf-8",
            errors="replace",
            capture_output=True,
        )
        if base_result.returncode != 0:
            continue
        base = base_result.stdout
        base_labels = Counter({label: base.count(label) for label in CLAIM_LABELS})
        current_labels = Counter({label: current.count(label) for label in CLAIM_LABELS})
        for label, count in base_labels.items():
            if current_labels[label] < count:
                errors.append(f"{name}: claim label count decreased for {label}")
        missing_claim_lines = claim_lines(base) - claim_lines(current)
        for claim_line in missing_claim_lines:
            errors.append(f"{name}: changed claim-labeled line: {claim_line}")
        missing_tokens = semantic_tokens(base) - semantic_tokens(current)
        for token, count in missing_tokens.items():
            errors.append(f"{name}: removed exact semantic token {token} ({count})")
        base_qualifiers = Counter(match.lower() for match in QUALIFIER_RE.findall(base))
        current_qualifiers = Counter(match.lower() for match in QUALIFIER_RE.findall(current))
        for qualifier, count in (base_qualifiers - current_qualifiers).items():
            errors.append(f"{name}: removed limiting qualifier {qualifier} ({count})")
    return errors


def check_unrelated_changes(inventory: dict[str, object]) -> list[str]:
    current = [line for line in run_git("status", "--short").splitlines() if line]
    allowed_owned_prefixes = (
        "AGENTS.md",
        "README.md",
        "CURRENT_STATE.md",
        "WORKER_START_HERE.md",
        "wiki/HaloFPX_Wiki/",
        "project-management/documentation/",
    )
    protected_prefixes = ("sources/", "experiments/")
    unrelated = sorted(
        line
        for line in current
        if not line[3:].startswith(allowed_owned_prefixes)
        and not line[3:].startswith(protected_prefixes)
    )
    baseline = sorted(inventory.get("baseline_unrelated_git_status", []))
    if unrelated != baseline:
        return [f"unrelated Git status changed: {baseline!r} -> {unrelated!r}"]
    return []


def changed_markdown() -> list[Path]:
    inventory = json.loads(INVENTORY.read_text(encoding="utf-8"))
    baseline = str(inventory["repository_head"])
    names = set(run_git("diff", "--name-only", baseline, "--", "*.md").splitlines())
    names.update(run_git("ls-files", "--others", "--exclude-standard", "--", "*.md").splitlines())
    return sorted(
        ROOT / name
        for name in names
        if (ROOT / name).exists()
        and not name.startswith(("sources/", "experiments/"))
    )


def readability_warnings(files: list[Path]) -> list[dict[str, object]]:
    warnings: list[dict[str, object]] = []
    known_abbreviations = {
        "ABI", "ADR", "AMD", "API", "CI", "CLI", "DAG", "DRAM", "GPU",
        "HIP", "HMAC", "HTTP", "KV", "LLM", "MoE", "MTP", "OOM", "PDT",
        "PID", "RDMA", "RNG", "ROCm", "RPC", "SHA", "SoC", "SSD", "STE",
        "TTFT", "USB4",
    }
    for path in files:
        text = path.read_text(encoding="utf-8")
        text = re.sub(r"```.*?```", "", text, flags=re.DOTALL)
        prose_lines = [
            line.strip()
            for line in text.splitlines()
            if line.strip()
            and not line.lstrip().startswith(("#", "-", "*", "|", "```", ">", "1.", "2.", "3.", "4.", "5.", "6.", "7.", "8.", "9."))
        ]
        prose = " ".join(prose_lines)
        long_sentences = []
        for prose_line in prose_lines:
            for sentence in SENTENCE_RE.split(prose_line):
                words = re.findall(r"\b[\w'-]+\b", sentence)
                if len(words) > 20:
                    long_sentences.append({"words": len(words), "text": sentence[:180]})
        undefined = sorted(
            abbreviation
            for abbreviation in set(ABBREVIATION_RE.findall(text))
            if abbreviation not in known_abbreviations
            and f"{abbreviation})" not in text
        )
        item = {
            "path": rel(path),
            "sentences_over_20_words": long_sentences,
            "passive_voice_candidates": len(PASSIVE_RE.findall(prose)),
            "ambiguous_pronoun_candidates": len(AMBIGUOUS_RE.findall(prose)),
            "undefined_abbreviation_candidates": undefined,
            "vague_word_candidates": sorted(set(VAGUE_RE.findall(prose))),
        }
        if (
            long_sentences
            or item["passive_voice_candidates"]
            or item["ambiguous_pronoun_candidates"]
            or undefined
            or item["vague_word_candidates"]
        ):
            warnings.append(item)
    return warnings


def main() -> int:
    failures: dict[str, list[str]] = {}
    inventory = json.loads(INVENTORY.read_text(encoding="utf-8"))
    files = markdown_files()
    link_errors, inbound = check_links(files)
    checks = {
        "broken_internal_links": link_errors,
        "orphan_authoritative_pages": check_orphans(files, inbound),
        "category_manifest_errors": check_manifests(),
        "missing_root_routes": check_root_routes(),
        "worker_start_errors": check_worker_start(),
        "protected_area_errors": check_protected(inventory),
        "claim_or_identifier_regressions": check_claim_diff(inventory),
        "unrelated_change_errors": check_unrelated_changes(inventory),
    }
    failures = {name: values for name, values in checks.items() if values}
    warnings = readability_warnings(changed_markdown())
    report = {
        "status": "PASS" if not failures else "FAIL",
        "repository_head": run_git("rev-parse", "HEAD").strip(),
        "markdown_files_checked": len(files),
        "category_manifests_checked": 12,
        "authoritative_orphans": len(checks["orphan_authoritative_pages"]),
        "broken_internal_links": len(link_errors),
        "protected_files_checked": inventory["protected_repository_snapshot"]["file_count"],
        "protected_content_digest_sha256": inventory["protected_repository_snapshot"]["content_digest_sha256"],
        "failures": failures,
        "readability_warnings": warnings,
    }
    print(json.dumps(report, indent=2, ensure_ascii=False))
    return 0 if not failures else 1


if __name__ == "__main__":
    sys.exit(main())
