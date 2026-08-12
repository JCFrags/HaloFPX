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

PUBLICATION_INTEGRATION_COMMIT = "728c3b441fcb38a9eb55272ed673da9d2d18c173"
PUBLICATION_IMPLEMENTATION_COMMIT = "620ef60aa446990335ef46c7d76738f797e62f8f"
PUBLICATION_WIKI_COMMIT = "b1c2d8aef707fb03920fc189ccd26395fa61879d"


def discover_git_root() -> Path:
    result = subprocess.run(
        ["git", "rev-parse", "--show-toplevel"],
        cwd=ROOT,
        text=True,
        encoding="utf-8",
        errors="replace",
        capture_output=True,
        check=True,
    )
    return Path(result.stdout.strip()).resolve()


GIT_ROOT = discover_git_root()
PROJECT_PREFIX = "" if ROOT == GIT_ROOT else ROOT.relative_to(GIT_ROOT).as_posix()
PUBLICATION_MODE = PROJECT_PREFIX == "project"
PUBLICATION_MANIFEST = GIT_ROOT / "docs" / "publication" / "manifest.json"

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
        cwd=GIT_ROOT,
        text=True,
        encoding="utf-8",
        errors="replace",
        capture_output=True,
        check=True,
    )
    return result.stdout


def run_git_result(*args: str, text: bool = True) -> subprocess.CompletedProcess:
    return subprocess.run(
        ["git", *args],
        cwd=GIT_ROOT,
        text=text,
        encoding="utf-8" if text else None,
        errors="replace" if text else None,
        capture_output=True,
        check=False,
    )


def project_git_path(path: str) -> str:
    path = path.strip("/")
    if not PROJECT_PREFIX:
        return path
    return f"{PROJECT_PREFIX}/{path}" if path else PROJECT_PREFIX


def project_relative_git_path(path: str) -> str | None:
    path = path.replace("\\", "/").strip("/")
    if not PROJECT_PREFIX:
        return path
    prefix = f"{PROJECT_PREFIX}/"
    return path[len(prefix):] if path.startswith(prefix) else None


def normalized_text_sha256(path: Path) -> str:
    text = path.read_text(encoding="utf-8").replace("\r\n", "\n").replace("\r", "\n")
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


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


def check_legacy_protected(inventory: dict[str, object]) -> tuple[list[str], dict[str, object]]:
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
            line
            for line in run_git(
                "status",
                "--short",
                "--",
                project_git_path("sources"),
                project_git_path("experiments"),
            ).splitlines()
            if line
        )
        if current_status != baseline_status:
            errors.append("protected Git status changed from the inventory baseline")
    return errors, {
        "mode": "legacy_worktree_snapshot",
        "files_checked": count,
        "recorded_file_count": snapshot["file_count"],
        "recorded_content_digest_sha256": snapshot["content_digest_sha256"],
        "computed_content_digest_sha256": content_digest,
        "worktree_clean_at_committed_head": None,
        "publication_receipt_verified": None,
    }


def publication_manifest_receipt_errors(
    inventory: dict[str, object], manifest: dict[str, object]
) -> list[str]:
    errors: list[str] = []
    snapshot = inventory["protected_repository_snapshot"]
    receipt = manifest.get("documentation_inventory_preservation")
    if not isinstance(receipt, dict):
        return ["publication manifest lacks documentation_inventory_preservation"]

    expected_scalars = {
        "inventory_path": "project/project-management/documentation/document-inventory.json",
        "inventory_repository_head": inventory["repository_head"],
        "engineering_wiki_source_commit": PUBLICATION_WIKI_COMMIT,
        "integration_commit": PUBLICATION_INTEGRATION_COMMIT,
    }
    for key, expected in expected_scalars.items():
        if receipt.get(key) != expected:
            errors.append(f"publication inventory receipt has invalid {key}")

    recorded = receipt.get("protected_repository_snapshot")
    if not isinstance(recorded, dict):
        errors.append("publication inventory receipt lacks protected_repository_snapshot")
        return errors
    for key in (
        "file_count",
        "metadata_digest_sha256",
        "content_digest_sha256",
        "content_digest_recorded_on",
    ):
        if recorded.get(key) != snapshot.get(key):
            errors.append(f"publication inventory receipt changed protected snapshot {key}")
    return errors


def check_publication_protected(
    inventory: dict[str, object]
) -> tuple[list[str], dict[str, object]]:
    errors: list[str] = []
    snapshot = inventory["protected_repository_snapshot"]

    if not PUBLICATION_MANIFEST.is_file():
        return [f"missing publication manifest: {PUBLICATION_MANIFEST}"], {
            "mode": "monorepo_publication",
            "files_checked": 0,
            "recorded_file_count": snapshot["file_count"],
            "recorded_content_digest_sha256": snapshot["content_digest_sha256"],
            "worktree_clean_at_committed_head": False,
            "publication_receipt_verified": False,
        }

    try:
        manifest = json.loads(PUBLICATION_MANIFEST.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return [f"invalid publication manifest: {exc}"], {
            "mode": "monorepo_publication",
            "files_checked": 0,
            "recorded_file_count": snapshot["file_count"],
            "recorded_content_digest_sha256": snapshot["content_digest_sha256"],
            "worktree_clean_at_committed_head": False,
            "publication_receipt_verified": False,
        }

    receipt_errors = publication_manifest_receipt_errors(inventory, manifest)
    errors.extend(receipt_errors)

    history = manifest.get("history", {})
    if not isinstance(history, dict) or history.get("integration_commit") != PUBLICATION_INTEGRATION_COMMIT:
        errors.append("publication manifest changed the integration commit")

    for commit, role in (
        (PUBLICATION_IMPLEMENTATION_COMMIT, "implementation"),
        (PUBLICATION_WIKI_COMMIT, "engineering wiki"),
        (PUBLICATION_INTEGRATION_COMMIT, "integration"),
    ):
        if run_git_result("cat-file", "-e", f"{commit}^{{commit}}").returncode != 0:
            errors.append(f"missing {role} commit {commit}")
        elif run_git_result("merge-base", "--is-ancestor", commit, "HEAD").returncode != 0:
            errors.append(f"{role} commit is not an ancestor of HEAD: {commit}")

    parent_line = run_git_result(
        "rev-list", "--parents", "-n", "1", PUBLICATION_INTEGRATION_COMMIT
    )
    expected_parents = [
        PUBLICATION_INTEGRATION_COMMIT,
        PUBLICATION_IMPLEMENTATION_COMMIT,
        PUBLICATION_WIKI_COMMIT,
    ]
    if parent_line.returncode != 0 or parent_line.stdout.split() != expected_parents:
        errors.append("publication integration commit parents changed")

    imported_tree = run_git_result(
        "rev-parse", f"{PUBLICATION_INTEGRATION_COMMIT}:project"
    )
    source_tree = run_git_result("rev-parse", f"{PUBLICATION_WIKI_COMMIT}^{{tree}}")
    if (
        imported_tree.returncode != 0
        or source_tree.returncode != 0
        or imported_tree.stdout.strip() != source_tree.stdout.strip()
    ):
        errors.append("integration project tree does not equal the engineering-wiki source tree")

    source_inventory = run_git_result(
        "show",
        f"{PUBLICATION_WIKI_COMMIT}:project-management/documentation/document-inventory.json",
        text=False,
    )
    source_inventory_bytes = source_inventory.stdout.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    current_inventory_bytes = INVENTORY.read_bytes().replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    if source_inventory.returncode != 0 or source_inventory_bytes != current_inventory_bytes:
        errors.append("documentation inventory differs from the imported source commit")

    inventory_head = str(inventory["repository_head"])
    if run_git_result(
        "merge-base", "--is-ancestor", inventory_head, PUBLICATION_WIKI_COMMIT
    ).returncode != 0:
        errors.append("inventory baseline is not an ancestor of the imported engineering wiki")

    protected_pathspecs = (
        project_git_path("sources"),
        project_git_path("experiments"),
    )
    protected_commit_diff = run_git_result(
        "diff",
        "--quiet",
        PUBLICATION_INTEGRATION_COMMIT,
        "HEAD",
        "--",
        *protected_pathspecs,
    )
    if protected_commit_diff.returncode != 0:
        errors.append("committed protected content changed after the publication boundary")

    current_status = [
        line
        for line in run_git(
            "status",
            "--porcelain=v1",
            "--untracked-files=all",
            "--",
            *protected_pathspecs,
        ).splitlines()
        if line
    ]
    if current_status:
        errors.append(f"protected worktree differs from committed HEAD: {current_status!r}")

    tracked_result = run_git_result(
        "ls-tree",
        "-r",
        "--name-only",
        PUBLICATION_INTEGRATION_COMMIT,
        "--",
        *protected_pathspecs,
    )
    tracked_count = len([line for line in tracked_result.stdout.splitlines() if line])
    if tracked_result.returncode != 0 or tracked_count == 0:
        errors.append("could not enumerate imported protected Git files")

    return errors, {
        "mode": "monorepo_publication",
        "files_checked": tracked_count,
        "recorded_file_count": snapshot["file_count"],
        "recorded_content_digest_sha256": snapshot["content_digest_sha256"],
        "imported_project_tree_sha1": imported_tree.stdout.strip() if imported_tree.returncode == 0 else None,
        "worktree_clean_at_committed_head": not current_status,
        "publication_receipt_verified": not receipt_errors,
    }


def check_protected(inventory: dict[str, object]) -> tuple[list[str], dict[str, object]]:
    if PUBLICATION_MODE:
        return check_publication_protected(inventory)
    return check_legacy_protected(inventory)


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


def changed_project_markdown_names(inventory: dict[str, object]) -> list[str]:
    if PUBLICATION_MODE:
        names: set[str] = set()
        for name in run_git(
            "diff",
            "--name-only",
            PUBLICATION_INTEGRATION_COMMIT,
            "--",
            project_git_path(""),
        ).splitlines():
            project_name = project_relative_git_path(name)
            if project_name and project_name.lower().endswith(".md"):
                names.add(project_name)
        for name in run_git(
            "ls-files",
            "--others",
            "--exclude-standard",
            "--",
            project_git_path("*.md"),
            project_git_path("**/*.md"),
        ).splitlines():
            project_name = project_relative_git_path(name)
            if project_name and project_name.lower().endswith(".md"):
                names.add(project_name)
        return sorted(names)

    baseline = str(inventory["repository_head"])
    return sorted(
        name
        for name in run_git("diff", "--name-only", baseline, "--", "*.md").splitlines()
        if name.lower().endswith(".md")
    )


def authorized_semantic_rewrites() -> dict[str, str]:
    if not PUBLICATION_MODE or not PUBLICATION_MANIFEST.is_file():
        return {}
    try:
        manifest = json.loads(PUBLICATION_MANIFEST.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}
    receipt = manifest.get("documentation_inventory_preservation", {})
    rewrites = receipt.get("authorized_semantic_rewrites", []) if isinstance(receipt, dict) else []
    result: dict[str, str] = {}
    if not isinstance(rewrites, list):
        return result
    for item in rewrites:
        if not isinstance(item, dict):
            continue
        path = item.get("project_relative_path")
        digest = item.get("normalized_text_sha256")
        if isinstance(path, str) and isinstance(digest, str):
            result[path] = digest.lower()
    return result


def check_claim_diff(inventory: dict[str, object]) -> list[str]:
    errors: list[str] = []
    baseline = PUBLICATION_WIKI_COMMIT if PUBLICATION_MODE else str(inventory["repository_head"])
    rewrite_receipts = authorized_semantic_rewrites()
    for name in changed_project_markdown_names(inventory):
        path = ROOT / name
        if not path.exists():
            continue
        current = path.read_text(encoding="utf-8")
        base_result = subprocess.run(
            ["git", "show", f"{baseline}:{name}"],
            cwd=GIT_ROOT,
            text=True,
            encoding="utf-8",
            errors="replace",
            capture_output=True,
        )
        if base_result.returncode != 0:
            continue
        base = base_result.stdout
        rewrite_authorized = (
            rewrite_receipts.get(name) == normalized_text_sha256(path)
        )
        base_labels = Counter({label: base.count(label) for label in CLAIM_LABELS})
        current_labels = Counter({label: current.count(label) for label in CLAIM_LABELS})
        for label, count in base_labels.items():
            if current_labels[label] < count and not rewrite_authorized:
                errors.append(f"{name}: claim label count decreased for {label}")
        missing_claim_lines = claim_lines(base) - claim_lines(current)
        if not rewrite_authorized:
            for claim_line in missing_claim_lines:
                errors.append(f"{name}: changed claim-labeled line: {claim_line}")
        missing_tokens = semantic_tokens(base) - semantic_tokens(current)
        if not rewrite_authorized:
            for token, count in missing_tokens.items():
                errors.append(f"{name}: removed exact semantic token {token} ({count})")
        base_qualifiers = Counter(match.lower() for match in QUALIFIER_RE.findall(base))
        current_qualifiers = Counter(match.lower() for match in QUALIFIER_RE.findall(current))
        if not rewrite_authorized:
            for qualifier, count in (base_qualifiers - current_qualifiers).items():
                errors.append(f"{name}: removed limiting qualifier {qualifier} ({count})")
    return errors


def check_unrelated_changes(inventory: dict[str, object]) -> list[str]:
    if PUBLICATION_MODE:
        current = [
            line
            for line in run_git(
                "status",
                "--short",
                "--untracked-files=all",
                "--",
                project_git_path(""),
            ).splitlines()
            if line
        ]
    else:
        current = [line for line in run_git("status", "--short").splitlines() if line]
    allowed_owned_files = {
        "AGENTS.md",
        "README.md",
        "CURRENT_STATE.md",
        "PERFORMANCE_WORKPLAN.md",
        "PROJECT_GOAL.md",
        "TARGET_MACHINES.md",
        "WORKER_START_HERE.md",
        "references/agent-harness.md",
        "research/prompts/tools/generate_wiki_manifest.py",
        "research/prompts/tools/test_generate_wiki_manifest.py",
        "reviews/follow-ups/2026-08-12__documentation-and-target-authority-audit__v01.md",
        "skills/README.md",
    }
    allowed_owned_prefixes = (
        "wiki/HaloFPX_Wiki/",
        "project-management/documentation/",
    )
    if PUBLICATION_MODE:
        allowed_owned_files.update({
            "project-management/lead/CURRENT_STATUS.md",
            "project-management/lead/DECISIONS.md",
            "project-management/lead/monitor-state.json",
        })
    protected_prefixes = ("sources/", "experiments/")
    unrelated: list[str] = []
    for line in current:
        path_text = line[3:]
        if " -> " in path_text:
            path_text = path_text.rsplit(" -> ", 1)[-1]
        project_path = project_relative_git_path(path_text) if PUBLICATION_MODE else path_text
        if project_path is None:
            continue
        if "__pycache__/" in project_path and project_path.endswith(".pyc"):
            continue
        if (
            project_path not in allowed_owned_files
            and not project_path.startswith(allowed_owned_prefixes)
            and not project_path.startswith(protected_prefixes)
        ):
            unrelated.append(f"{line[:3]}{project_path}" if PUBLICATION_MODE else line)
    unrelated.sort()
    # The legacy baseline recorded generated Python bytecode differences from
    # that workstation. They are not publication content and must not be
    # recreated in a clean monorepo checkout.
    baseline = [] if PUBLICATION_MODE else sorted(inventory.get("baseline_unrelated_git_status", []))
    if unrelated != baseline:
        return [f"unrelated Git status changed: {baseline!r} -> {unrelated!r}"]
    return []


def changed_markdown() -> list[Path]:
    inventory = json.loads(INVENTORY.read_text(encoding="utf-8"))
    names = set(changed_project_markdown_names(inventory))
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
    protected_errors, protected_report = check_protected(inventory)
    checks = {
        "broken_internal_links": link_errors,
        "orphan_authoritative_pages": check_orphans(files, inbound),
        "category_manifest_errors": check_manifests(),
        "missing_root_routes": check_root_routes(),
        "worker_start_errors": check_worker_start(),
        "protected_area_errors": protected_errors,
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
        "protected_files_checked": protected_report["files_checked"],
        "protected_content_digest_sha256": protected_report["recorded_content_digest_sha256"],
        "protected_preservation": protected_report,
        "failures": failures,
        "readability_warnings": warnings,
    }
    print(json.dumps(report, indent=2, ensure_ascii=False))
    return 0 if not failures else 1


if __name__ == "__main__":
    sys.exit(main())
