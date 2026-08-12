#!/usr/bin/env python3
"""Generate the local, read-only G0A candidate source-lock evidence package."""

from __future__ import annotations

import hashlib
import json
import os
import platform
import re
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path


LOCK_ROOT = Path(__file__).resolve().parent
WORKSPACE = LOCK_ROOT.parents[3]
REPO_ROOT = WORKSPACE / "sources" / "repositories"
BUNDLE_ROOT = LOCK_ROOT / "bundles"
RECORD_ROOT = LOCK_ROOT / "repository-records"
PATCH_ROOT = LOCK_ROOT / "patch-ids"

REPOSITORIES = [
    {
        "slug": "charlie12345__rocmfpx",
        "name": "charlie12345/ROCmFPX",
        "path": REPO_ROOT / "charlie12345__rocmfpx",
        "locked_revisions": [
            ("implementation-candidate", "61f2f2d7bc4955e9bca821095ef69125837133b5"),
            ("research-control", "a5605a72768c6562241b248e268e33dc92787394"),
        ],
    },
    {
        "slug": "fewtarius__llama-ai",
        "name": "fewtarius/llama-ai",
        "path": REPO_ROOT / "fewtarius__llama-ai",
        "locked_revisions": [
            ("operational-requirements-donor", "1017f3dfdce3ca2b06aa9007b23295db3bb35722"),
        ],
    },
    {
        "slug": "fewtarius__cachyllama",
        "name": "fewtarius/CachyLLama",
        "path": REPO_ROOT / "fewtarius__cachyllama",
        "locked_revisions": [
            ("mit-engine-donor", "6be745998f568e379ea197fcf827baec73ff9940"),
            ("donor-comparison-parent", "92366df30d4eaa4b85139b5fd694360237731b19"),
        ],
    },
    {
        "slug": "ggml-org__llama.cpp",
        "name": "ggml-org/llama.cpp",
        "path": REPO_ROOT / "ggml-org__llama.cpp",
        "locked_revisions": [
            ("captured-head", "6bdd77f13cf11b264b4231d320afc404f48d576e"),
            ("wiki-upstream-control", "788e07dc91d266ad3162a1ce9037665656269689"),
            ("donor-comparison-parent", "92366df30d4eaa4b85139b5fd694360237731b19"),
        ],
    },
]

DELTAS = [
    ("charlie12345__rocmfpx", "research-control_to_implementation-candidate",
     "a5605a72768c6562241b248e268e33dc92787394", "61f2f2d7bc4955e9bca821095ef69125837133b5"),
    ("fewtarius__cachyllama", "upstream-comparison-parent_to_donor-head",
     "92366df30d4eaa4b85139b5fd694360237731b19", "6be745998f568e379ea197fcf827baec73ff9940"),
    ("fewtarius__cachyllama", "custom-first-parent_to_merge-head",
     "c8ead677a7fe42fb0a67e6e866fb254cc338e9fd", "6be745998f568e379ea197fcf827baec73ff9940"),
    ("ggml-org__llama.cpp", "donor-comparison-parent_to_wiki-control",
     "92366df30d4eaa4b85139b5fd694360237731b19", "788e07dc91d266ad3162a1ce9037665656269689"),
    ("ggml-org__llama.cpp", "wiki-control_to_captured-head",
     "788e07dc91d266ad3162a1ce9037665656269689", "6bdd77f13cf11b264b4231d320afc404f48d576e"),
]


def run(args: list[str], cwd: Path | None = None, binary: bool = False) -> bytes | str:
    result = subprocess.run(
        args,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if result.returncode:
        raise RuntimeError(
            f"command failed ({result.returncode}): {' '.join(args)}\n"
            + result.stderr.decode("utf-8", "replace")
        )
    return result.stdout if binary else result.stdout.decode("utf-8", "surrogateescape").rstrip("\n")


def git(repo: Path, *args: str, binary: bool = False) -> bytes | str:
    return run(["git", "-C", str(repo), *args], binary=binary)


def write_text(path: Path, value: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(value.rstrip("\n") + "\n", encoding="utf-8", newline="\n")


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def input_category(path: str) -> str | None:
    checks = [
        (r"(?i)(^|/)(license|licence|copying|copyright|notice|third[_-]party[_-]notices)(\.|$|_)", "license-or-notice"),
        (r"(?i)(^|/)cmakelists\.txt$|^cmake/|/cmake/|cmakepresets\.json$", "cmake-input"),
        (r"(?i)(^|/)(makefile|meson\.build|meson_options\.txt)$", "build-system-input"),
        (r"(?i)(^|/)(requirements[^/]*\.txt|pyproject\.toml|poetry\.lock|uv\.lock|pdm\.lock|setup\.py|setup\.cfg)$", "python-dependency-input"),
        (r"(?i)(^|/)(package(-lock)?\.json|pnpm-lock\.yaml|yarn\.lock|cargo\.toml|cargo\.lock|go\.mod|go\.sum|vcpkg\.json|conanfile\.(txt|py))$", "dependency-input"),
        (r"(?i)(^|/)(dockerfile[^/]*|containerfile[^/]*)$|^\.github/workflows/.*\.(yml|yaml)$", "ci-container-input"),
        (r"(?i)^\.gitmodules$|^flake\.(nix|lock)$|^nix/|^scripts/(build|configure|install)[^/]*\.(sh|ps1|py)$|^(build|configure|install)[^/]*\.(sh|ps1|py)$", "build-or-source-lock-input"),
        (r"(?i)^scripts/.*\.(sh|ps1|py)$|^src/[^/]+/build\.(sh|ps1|py)$|^[^/]+\.(sh|ps1)$|^systemd/.*\.service$", "runtime-build-orchestration-input"),
    ]
    for pattern, category in checks:
        if re.search(pattern, path):
            return category
    return None


def parse_tree(repo: Path, revision: str) -> list[tuple[str, str, str, str]]:
    raw = git(repo, "ls-tree", "-r", "-z", revision, binary=True)
    rows = []
    for item in raw.split(b"\0"):
        if not item:
            continue
        metadata, raw_path = item.split(b"\t", 1)
        mode, kind, object_id = metadata.decode("ascii").split(" ")
        path = raw_path.decode("utf-8", "surrogateescape")
        rows.append((mode, kind, object_id, path))
    return rows


def patch_id(repo: Path, producer: list[str]) -> str:
    first = subprocess.Popen(
        ["git", "-C", str(repo), *producer],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    second = subprocess.Popen(
        ["git", "patch-id", "--stable"],
        stdin=first.stdout,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    assert first.stdout is not None
    first.stdout.close()
    out, second_err = second.communicate()
    first_err = first.stderr.read() if first.stderr else b""
    first_rc = first.wait()
    if first_rc or second.returncode:
        raise RuntimeError(
            f"patch-id pipeline failed: {' '.join(producer)}\n"
            + (first_err + second_err).decode("utf-8", "replace")
        )
    return out.decode("utf-8", "replace").rstrip("\n")


def main() -> None:
    started = datetime.now(timezone.utc).isoformat()
    for path in (BUNDLE_ROOT, RECORD_ROOT, PATCH_ROOT):
        path.mkdir(parents=True, exist_ok=True)

    records = []
    repo_by_slug = {item["slug"]: item for item in REPOSITORIES}
    for spec in REPOSITORIES:
        repo = spec["path"]
        record_name = f"{spec['slug']}--record"
        out = RECORD_ROOT / record_name
        out.mkdir(parents=True, exist_ok=True)

        before_status = git(repo, "status", "--porcelain=v2", "--branch")
        before_refs = git(
            repo, "for-each-ref",
            "--format=%(refname)%09%(objecttype)%09%(objectname)%09%(upstream:short)",
            "refs/heads", "refs/remotes", "refs/tags",
        )
        remotes = git(repo, "remote", "-v")
        shallow = git(repo, "rev-parse", "--is-shallow-repository")
        object_format = git(repo, "rev-parse", "--show-object-format")
        head = git(repo, "rev-parse", "HEAD")
        branch = git(repo, "symbolic-ref", "--short", "HEAD")
        count_objects = git(repo, "count-objects", "-vH")
        fsck = git(repo, "fsck", "--full", "--strict")

        write_text(out / "status-before.txt", before_status)
        write_text(out / "refs.txt", before_refs)
        write_text(out / "remotes.txt", remotes)
        write_text(out / "count-objects.txt", count_objects)
        write_text(out / "git-fsck.txt", fsck or "[no output; exit 0]")

        revision_records = []
        object_rows = ["role\tobject_role\tobject_id\tobserved_type"]
        input_rows = [
            "role\trevision\tcategory\tpath\tmode\tgit_object\tsha256_raw_blob\tsize_bytes"
        ]
        for role, commit in spec["locked_revisions"]:
            kind = git(repo, "cat-file", "-t", commit)
            if kind != "commit":
                raise RuntimeError(f"{spec['name']} {commit} is not a commit")
            tree = git(repo, "show", "-s", "--format=%T", commit)
            parents = [p for p in git(repo, "show", "-s", "--format=%P", commit).split(" ") if p]
            object_rows.append(f"{role}\tcommit\t{commit}\t{kind}")
            object_rows.append(f"{role}\ttree\t{tree}\t{git(repo, 'cat-file', '-t', tree)}")
            for parent in parents:
                object_rows.append(f"{role}\tparent\t{parent}\t{git(repo, 'cat-file', '-t', parent)}")

            gitlinks = []
            for mode, entry_kind, object_id, path in parse_tree(repo, commit):
                if mode == "160000":
                    gitlinks.append({"path": path, "commit": object_id})
                    object_rows.append(
                        f"{role}\tgitlink\t{object_id}\tdeclared-not-required-in-superproject-object-db"
                    )
                    continue
                category = input_category(path)
                if category and entry_kind == "blob":
                    raw_blob = git(repo, "cat-file", "blob", object_id, binary=True)
                    input_rows.append(
                        "\t".join([
                            role, commit, category, path, mode, object_id,
                            sha256_bytes(raw_blob), str(len(raw_blob)),
                        ])
                    )

            revision_records.append({
                "role": role,
                "commit": commit,
                "tree": tree,
                "parents": parents,
                "commit_date": git(repo, "show", "-s", "--format=%cI", commit),
                "author": git(repo, "show", "-s", "--format=%an <%ae>", commit),
                "subject": git(repo, "show", "-s", "--format=%s", commit),
                "gitlinks": gitlinks,
            })

        write_text(out / "object-availability.tsv", "\n".join(object_rows))
        write_text(out / "license-build-inputs.tsv", "\n".join(input_rows))

        bundle = BUNDLE_ROOT / f"{spec['slug']}--all-refs.bundle"
        if bundle.exists():
            bundle.unlink()
        run(["git", "-C", str(repo), "bundle", "create", str(bundle), "--all"])
        verification = run(["git", "-C", str(repo), "bundle", "verify", str(bundle)])
        write_text(out / "bundle-verify.txt", verification)

        after_status = git(repo, "status", "--porcelain=v2", "--branch")
        after_refs = git(
            repo, "for-each-ref",
            "--format=%(refname)%09%(objecttype)%09%(objectname)%09%(upstream:short)",
            "refs/heads", "refs/remotes", "refs/tags",
        )
        write_text(out / "status-after.txt", after_status)
        write_text(out / "refs-after.txt", after_refs)
        if before_status != after_status or before_refs != after_refs:
            raise RuntimeError(f"reference clone changed while locking {spec['name']}")

        records.append({
            "repository": spec["name"],
            "slug": spec["slug"],
            "canonical_path": str(repo.resolve()),
            "head": head,
            "branch": branch,
            "shallow": shallow == "true",
            "object_format": object_format,
            "remotes_file": f"repository-records/{record_name}/remotes.txt",
            "refs_file": f"repository-records/{record_name}/refs.txt",
            "dirty_state_file": f"repository-records/{record_name}/status-before.txt",
            "worktree_and_refs_unchanged": True,
            "fsck_strict_passed": True,
            "locked_revisions": revision_records,
            "bundle": {
                "path": f"bundles/{bundle.name}",
                "bytes": bundle.stat().st_size,
                "sha256": sha256_file(bundle),
                "verify_file": f"repository-records/{record_name}/bundle-verify.txt",
                "verified": True,
            },
        })

    delta_records = []
    for slug, label, start, end in DELTAS:
        repo = repo_by_slug[slug]["path"]
        for endpoint in (start, end):
            if git(repo, "cat-file", "-t", endpoint) != "commit":
                raise RuntimeError(f"unavailable delta endpoint: {endpoint}")
        base = f"{slug}--{label}"
        summary = git(repo, "diff", "--stat", "--summary", start, end)
        names = git(repo, "diff", "--name-status", start, end)
        commits = git(
            repo, "log", "--reverse",
            "--format=%H%x09%P%x09%aI%x09%an <%ae>%x09%s",
            f"{start}..{end}",
        )
        aggregate = patch_id(repo, ["diff", "--binary", start, end])
        series = patch_id(repo, ["log", "--reverse", "--no-merges", "-p", "--binary", f"{start}..{end}"])
        write_text(PATCH_ROOT / f"{base}--summary.txt", summary)
        write_text(PATCH_ROOT / f"{base}--name-status.tsv", names)
        write_text(
            PATCH_ROOT / f"{base}--commits.tsv",
            "commit\tparents\tauthor_date\tauthor\tsubject\n" + commits,
        )
        write_text(PATCH_ROOT / f"{base}--aggregate.patch-id", aggregate or "[empty tree delta]")
        write_text(PATCH_ROOT / f"{base}--series.patch-ids", series or "[no non-merge commits]")
        delta_records.append({
            "repository_slug": slug,
            "label": label,
            "from": start,
            "to": end,
            "aggregate_patch_id_file": f"patch-ids/{base}--aggregate.patch-id",
            "series_patch_ids_file": f"patch-ids/{base}--series.patch-ids",
        })

    manifest = {
        "schema_version": 1,
        "artifact_type": "g0a-candidate-source-lock",
        "status": "candidate",
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "generation_started_at_utc": started,
        "implementation_pin_decision": {
            "state": "OPEN",
            "item": "OPEN-PIN-01",
            "nominated_candidate": "charlie12345/ROCmFPX@61f2f2d7bc4955e9bca821095ef69125837133b5",
            "research_control": "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394",
            "note": "This source lock preserves both revisions and does not approve either as the implementation pin.",
        },
        "generation_environment": {
            "git": run(["git", "--version"]),
            "python": sys.version.replace("\n", " "),
            "platform": platform.platform(),
            "workspace_root": str(WORKSPACE),
        },
        "policies": {
            "clone_mutation": "forbidden; status and refs compared before and after",
            "network_fetch": "not performed",
            "checkout": "not performed",
            "donor_or_imported_code_execution": "not performed",
            "bundle_scope": "all refs present in each complete, non-shallow local clone at capture time",
            "license_build_input_inventory": "deterministic path-policy inventory at each locked revision with raw Git blob SHA-256",
        },
        "repositories": records,
        "deltas": delta_records,
    }
    write_text(LOCK_ROOT / "source-lock-manifest.json", json.dumps(manifest, indent=2))

    excluded = {"SHA256SUMS.txt"}
    hash_rows = []
    for path in sorted(p for p in LOCK_ROOT.rglob("*") if p.is_file()):
        if path.name in excluded:
            continue
        relative = path.relative_to(LOCK_ROOT).as_posix()
        hash_rows.append(f"{sha256_file(path)}  {relative}")
    write_text(LOCK_ROOT / "SHA256SUMS.txt", "\n".join(hash_rows))
    print(f"Generated source lock at {LOCK_ROOT}")


if __name__ == "__main__":
    main()
