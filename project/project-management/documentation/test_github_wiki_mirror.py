#!/usr/bin/env python3
"""Closed contract tests for the generated GitHub Wiki convenience mirror."""

from __future__ import annotations

import hashlib
import json
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import unittest
import uuid
from pathlib import Path, PurePosixPath
from unittest import mock


TOOL_DIRECTORY = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOL_DIRECTORY))

import github_wiki_mirror as mirror  # noqa: E402


AUDITED_WIKI_TREE = "ce025293b00bc80a65005714165fcc4c80e42fa5"
AUDITED_MAP_SHA256 = "f8d21d17d2937738d3ef413c2b487f8e425847b076aaa520f81214247e4e179c"


def _create_directory_redirect(link: Path, target: Path) -> None:
    if os.name == "nt":
        subprocess.run(
            ["cmd.exe", "/d", "/c", "mklink", "/J", str(link), str(target)],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
    else:
        link.symlink_to(target, target_is_directory=True)


def _remove_directory_redirect(link: Path) -> None:
    if os.name == "nt":
        os.rmdir(link)
    else:
        link.unlink()


class GitHubWikiMirrorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.repo = mirror.find_repo_root(TOOL_DIRECTORY)
        cls.page_map_path = cls.repo / (
            "project/project-management/documentation/github-wiki-page-map.json"
        )
        cls.snapshot = mirror.load_snapshot(cls.repo, "HEAD")
        cls.page_map = mirror.load_frozen_map(cls.page_map_path)
        cls.rendered = mirror.build_mirror(cls.snapshot, cls.page_map)

    def test_exact_audited_source_and_mapping_coverage(self) -> None:
        self.assertEqual(self.snapshot.wiki_tree, AUDITED_WIKI_TREE)
        self.assertEqual(len(self.snapshot.files), 642)
        self.assertEqual(sum(item.path.suffix.lower() == ".md" for item in self.snapshot.files), 534)
        self.assertEqual(sum(item.path.suffix.lower() != ".md" for item in self.snapshot.files), 108)
        self.assertEqual(sum(item.size for item in self.snapshot.files), 2_266_891)
        self.assertEqual(len(self.page_map.entries), 642)
        self.assertEqual(
            hashlib.sha256(self.page_map_path.read_bytes()).hexdigest(),
            AUDITED_MAP_SHA256,
        )
        self.assertEqual(self.page_map.sha256, AUDITED_MAP_SHA256)
        self.assertEqual(
            mirror._canonical_json_bytes(mirror.derive_mapping(self.snapshot)),
            self.page_map_path.read_bytes(),
        )

    def test_destination_registry_is_flat_for_pages_and_collision_free(self) -> None:
        destinations = [entry.destination.as_posix() for entry in self.page_map.entries]
        self.assertEqual(len(destinations), len({item.casefold() for item in destinations}))
        pages = [entry for entry in self.page_map.entries if entry.kind == "page"]
        assets = [entry for entry in self.page_map.entries if entry.kind == "asset"]
        self.assertTrue(all(len(entry.destination.parts) == 1 for entry in pages))
        self.assertTrue(all(entry.destination.parts[0] == "assets" for entry in assets))
        self.assertEqual(max(len(entry.destination.name) for entry in pages), 107)

    def test_rendered_coverage_assets_and_authority_banners(self) -> None:
        self.assertEqual(len(self.rendered.files), 645)
        self.assertEqual(
            set(self.rendered.by_destination),
            {entry.destination for entry in self.page_map.entries}
            | {PurePosixPath(name) for name in mirror.SPECIAL_OUTPUTS},
        )
        for entry in self.page_map.entries:
            source = self.snapshot.by_path[entry.source]
            output = self.rendered.by_destination[entry.destination]
            if entry.kind == "asset":
                self.assertEqual(output.data, source.data, entry.source.as_posix())
                self.assertEqual(output.sha256, source.sha256)
            else:
                self.assertTrue(
                    output.data.startswith(
                        mirror._authority_banner(self.snapshot, entry.source).encode("utf-8")
                    ),
                    entry.destination.as_posix(),
                )

    def test_manifest_binds_exact_source_output_and_link_census(self) -> None:
        manifest_file = self.rendered.by_destination[PurePosixPath("mirror-manifest.json")]
        manifest = json.loads(manifest_file.data)
        self.assertEqual(manifest["source"]["wiki_tree"], AUDITED_WIKI_TREE)
        self.assertEqual(manifest["source"]["source_file_count"], 642)
        self.assertEqual(manifest["source"]["markdown_page_count"], 534)
        self.assertEqual(manifest["source"]["non_markdown_asset_count"], 108)
        self.assertEqual(manifest["output"]["file_count_including_manifest"], 645)
        self.assertEqual(len(manifest["files"]), 644)
        self.assertEqual(manifest["mapping"]["sha256"], AUDITED_MAP_SHA256)
        self.assertEqual(manifest["links"]["source_link_occurrence_count"], 1_522)
        self.assertEqual(manifest["links"]["source_unique_destination_count"], 826)
        self.assertEqual(manifest["links"]["generated_link_occurrence_count"], 2_696)
        self.assertEqual(manifest["links"]["generated_local_page_occurrence_count"], 810)
        self.assertEqual(
            manifest["links"]["generated_pinned_repository_occurrence_count"], 1_192
        )
        self.assertEqual(manifest["links"]["generated_external_occurrence_count"], 694)
        self.assertEqual(
            len(manifest["links"]["explicit_fragment_aliases_applied"]), 3
        )

    def test_commonmark_rewrite_preserves_literal_code_and_handles_references(self) -> None:
        text = """[inline](architecture-overview.md)

[reference][decision]

[decision]: decision-map.md

`[literal](architecture-overview.md)`

```text
[fenced](architecture-overview.md)
```
"""
        rewritten = mirror.rewrite_markdown_body(
            text, PurePosixPath("README.md"), self.snapshot, self.page_map
        )
        self.assertIn("[inline](R-architecture-overview)", rewritten)
        self.assertIn("[decision]: R-decision-map", rewritten)
        self.assertIn("`[literal](architecture-overview.md)`", rewritten)
        self.assertIn("[fenced](architecture-overview.md)", rewritten)
        self.assertEqual(len(mirror._extract_links(rewritten)), 2)

    def test_nonlink_lookalikes_are_byte_preserved(self) -> None:
        text = """[real](architecture-overview.md)

\\[escaped](architecture-overview.md)

<span title="[attribute](architecture-overview.md)">unchanged</span>

<!-- [comment](architecture-overview.md) -->
"""
        rewritten = mirror.rewrite_markdown_body(
            text, PurePosixPath("README.md"), self.snapshot, self.page_map
        )
        self.assertIn("[real](R-architecture-overview)", rewritten)
        self.assertIn("\\[escaped](architecture-overview.md)", rewritten)
        self.assertIn('title="[attribute](architecture-overview.md)"', rewritten)
        self.assertIn("<!-- [comment](architecture-overview.md) -->", rewritten)
        self.assertEqual(mirror._extract_links(rewritten), ["R-architecture-overview"])

    def test_semantically_unchanged_entity_destination_preserves_source_bytes(self) -> None:
        text = "[external](https://example.invalid/?a=1&amp;b=2)\n"
        rewritten = mirror.rewrite_markdown_body(
            text, PurePosixPath("README.md"), self.snapshot, self.page_map
        )
        self.assertEqual(rewritten, text)

    def test_raw_html_href_and_src_fail_closed(self) -> None:
        for text in (
            '<a href="architecture-overview.md">x</a>\n',
            '<img src="architecture-overview.md">\n',
        ):
            with self.subTest(text=text):
                with self.assertRaises(mirror.MirrorError):
                    mirror.rewrite_markdown_body(
                        text,
                        PurePosixPath("README.md"),
                        self.snapshot,
                        self.page_map,
                    )

    def test_unadmitted_reference_definition_shapes_fail_closed(self) -> None:
        cases = (
            "[x][a]\n\n[a]:\n  architecture-overview.md\n",
            "> [a]: architecture-overview.md\n>\n> [x][a]\n",
        )
        for text in cases:
            with self.subTest(text=text):
                with self.assertRaises(mirror.MirrorError):
                    mirror.rewrite_markdown_body(
                        text,
                        PurePosixPath("README.md"),
                        self.snapshot,
                        self.page_map,
                    )

    def test_front_matter_is_removed_from_rendered_body_but_bound_in_manifest(self) -> None:
        entry = next(
            entry
            for entry in self.page_map.entries
            if entry.kind == "page"
            and self.snapshot.by_path[entry.source].data.startswith(b"---\n")
        )
        source = self.snapshot.by_path[entry.source]
        output = self.rendered.by_destination[entry.destination]
        self.assertTrue(output.data.startswith(b"<!-- GENERATED BY"))
        self.assertNotIn(b"\n---\nsection_id:", output.data)
        front_matter, _body = mirror._split_front_matter(
            source.data.decode("utf-8"), source.path
        )
        manifest = json.loads(
            self.rendered.by_destination[PurePosixPath("mirror-manifest.json")].data
        )
        record = next(
            record
            for record in manifest["files"]
            if record["source_path"] == entry.source.as_posix()
        )
        self.assertEqual(record["source_front_matter_size"], len(front_matter.encode()))
        self.assertEqual(
            record["source_front_matter_sha256"],
            hashlib.sha256(front_matter.encode()).hexdigest(),
        )

    def test_front_matter_supports_crlf_and_rejects_bom(self) -> None:
        front_matter, body = mirror._split_front_matter(
            "---\r\nsection_id: 01\r\n---\r\n\r\n# Body\r\n",
            PurePosixPath("crlf.md"),
        )
        self.assertEqual(front_matter, "---\r\nsection_id: 01\r\n---\r\n")
        self.assertEqual(body, "\r\n# Body\r\n")
        with self.assertRaises(mirror.MirrorError):
            mirror._split_front_matter(
                "\ufeff---\nsection_id: 01\n---\n\n# Body\n",
                PurePosixPath("bom.md"),
            )

    def test_missing_unsafe_and_ambiguous_destinations_fail_closed(self) -> None:
        for destination in ("missing-page.md", "/root.md", "javascript:alert(1)"):
            with self.subTest(destination=destination):
                with self.assertRaises(mirror.MirrorError):
                    mirror.rewrite_destination(
                        destination,
                        PurePosixPath("README.md"),
                        self.snapshot,
                        self.page_map,
                    )

    def test_unambiguous_legacy_fragment_repair_is_explicit(self) -> None:
        source = PurePosixPath(
            "04_Hardware_and_OS_Platform/"
            "23_Linux_Kernel_amdgpu_Firmware_ROCm_and_Mesa_Compatibility_Matrix/README.md"
        )
        rewritten = mirror.rewrite_destination(
            "sources.md#s23-l03", source, self.snapshot, self.page_map
        )
        self.assertEqual(
            rewritten,
            "S23-Linux-Kernel-amdgpu-Firmware-ROCm-and-Mesa-Compatibility-Matrix--sources"
            "#s23-l03--post-incident-recovered-service-authority",
        )

    def test_noncanonical_and_colliding_maps_are_rejected(self) -> None:
        value = json.loads(self.page_map_path.read_text(encoding="utf-8"))
        with tempfile.TemporaryDirectory(prefix="halofpx-wiki-map-test-") as temporary:
            noncanonical = Path(temporary) / "noncanonical.json"
            noncanonical.write_bytes(self.page_map_path.read_bytes() + b" ")
            with self.assertRaises(mirror.MirrorError):
                mirror.load_frozen_map(noncanonical)

            value["entries"][1]["destination"] = value["entries"][0][
                "destination"
            ].swapcase()
            collision = Path(temporary) / "collision.json"
            collision.write_bytes(mirror._canonical_json_bytes(value))
            with self.assertRaises(mirror.MirrorError):
                mirror.load_frozen_map(collision)

    def test_nonportable_map_paths_fail_before_filesystem_use(self) -> None:
        values = (
            "assets/x\\..\\..\\escape.txt",
            "assets/control\x00.txt",
            "assets/tab\tname.txt",
            "assets/newline\nname.txt",
            "assets/delete\x7fname.txt",
            "assets/con/file.txt",
            "assets/question?.txt",
            "assets/pipe|name.txt",
        )
        for raw in values:
            with self.subTest(raw=raw):
                with self.assertRaises(mirror.MirrorError):
                    mirror._validate_portable_path(
                        PurePosixPath(raw), raw, "adversarial test"
                    )

    def test_existing_output_validation_detects_tamper_and_needs_no_network(self) -> None:
        with tempfile.TemporaryDirectory(prefix="halofpx-wiki-output-test-") as temporary:
            output = Path(temporary) / "mirror"
            mirror.write_mirror(self.rendered, output, self.repo)
            with mock.patch.object(
                socket, "create_connection", side_effect=AssertionError("network used")
            ):
                mirror.compare_output(self.rendered, output)
            home = output / "Home.md"
            home.write_bytes(home.read_bytes() + b"tamper\n")
            with self.assertRaises(mirror.MirrorError):
                mirror.compare_output(self.rendered, output)

    def test_historical_manifest_audit_detects_tamper_and_extras(self) -> None:
        with tempfile.TemporaryDirectory(prefix="halofpx-wiki-audit-test-") as temporary:
            output = Path(temporary) / "mirror"
            mirror.write_mirror(self.rendered, output, self.repo)
            result = mirror.audit_manifest_output(output)
            self.assertEqual(result["file_count"], 645)
            extra = output / "unexpected.md"
            extra.write_text("unexpected\n", encoding="utf-8")
            with self.assertRaises(mirror.MirrorError):
                mirror.audit_manifest_output(output)
            extra.unlink()
            footer = output / "_Footer.md"
            footer.write_bytes(footer.read_bytes() + b"tamper\n")
            with self.assertRaises(mirror.MirrorError):
                mirror.audit_manifest_output(output)

    def test_historical_manifest_audit_rejects_malformed_v1_shape(self) -> None:
        with tempfile.TemporaryDirectory(prefix="halofpx-wiki-audit-shape-test-") as temporary:
            output = Path(temporary) / "mirror"
            mirror.write_mirror(self.rendered, output, self.repo)
            manifest_path = output / "mirror-manifest.json"
            manifest = json.loads(manifest_path.read_bytes())
            del manifest["authority"]
            manifest_path.write_bytes(mirror._canonical_json_bytes(manifest))
            with self.assertRaises(mirror.MirrorError):
                mirror.audit_manifest_output(output)

    def test_historical_manifest_audit_recomputes_source_ledger(self) -> None:
        with tempfile.TemporaryDirectory(prefix="halofpx-wiki-audit-ledger-test-") as temporary:
            output = Path(temporary) / "mirror"
            mirror.write_mirror(self.rendered, output, self.repo)
            manifest_path = output / "mirror-manifest.json"
            original = json.loads(manifest_path.read_bytes())
            mutations = (
                lambda value: value["source"].__setitem__("git_blob_bytes", 0),
                lambda value: value["source"].__setitem__("source_set_sha256", "0" * 64),
                lambda value: next(
                    record
                    for record in value["files"]
                    if record["kind"] == "source-page"
                ).__setitem__(
                    "source_path",
                    next(
                        record["source_path"]
                        for record in value["files"]
                        if record["kind"] == "source-asset"
                    ),
                ),
            )
            for mutate in mutations:
                manifest = json.loads(json.dumps(original))
                mutate(manifest)
                manifest_path.write_bytes(mirror._canonical_json_bytes(manifest))
                with self.assertRaises(mirror.MirrorError):
                    mirror.audit_manifest_output(output)

    def test_deterministic_regeneration_needs_no_network(self) -> None:
        with mock.patch.object(
            socket, "create_connection", side_effect=AssertionError("network used")
        ):
            second = mirror.build_mirror(self.snapshot, self.page_map)
        self.assertEqual(
            {path: item.data for path, item in self.rendered.by_destination.items()},
            {path: item.data for path, item in second.by_destination.items()},
        )

    def test_output_writer_refuses_canonical_and_git_paths(self) -> None:
        nonce = uuid.uuid4().hex
        unsafe_paths = (
            self.repo / mirror.CANONICAL_ROOT / f"generated-{nonce}",
            self.repo / ".git" / f"generated-{nonce}",
        )
        for output in unsafe_paths:
            with self.subTest(output=output):
                self.assertFalse(output.exists())
                with self.assertRaises(mirror.MirrorError):
                    mirror.write_mirror(self.rendered, output, self.repo)
                self.assertFalse(output.exists())

        git_roots = mirror._git_administration_roots(self.repo)
        self.assertGreaterEqual(len(git_roots), 2)
        for root in git_roots:
            output = root / f"generated-{nonce}"
            with self.subTest(git_administration_root=root):
                self.assertFalse(output.exists())
                with self.assertRaises(mirror.MirrorError):
                    mirror.write_mirror(self.rendered, output, self.repo)
                self.assertFalse(output.exists())

    def test_new_outputs_are_claimed_without_replacement(self) -> None:
        with tempfile.TemporaryDirectory(prefix="halofpx-wiki-claim-test-") as temporary:
            root = Path(temporary)
            output = root / "mirror"
            output.mkdir()
            sentinel = output / "sentinel.txt"
            sentinel.write_text("keep\n", encoding="utf-8")
            with self.assertRaises(mirror.MirrorError):
                mirror.write_mirror(self.rendered, output, self.repo)
            self.assertEqual(sentinel.read_text(encoding="utf-8"), "keep\n")

            page_map = root / "page-map.json"
            page_map.write_text("keep\n", encoding="utf-8")
            with mock.patch.object(mirror, "load_snapshot", return_value=self.snapshot):
                self.assertEqual(
                    mirror.main(
                        [
                            "--repo",
                            str(self.repo),
                            "freeze-map",
                            "--output",
                            str(page_map),
                        ]
                    ),
                    1,
                )
            self.assertEqual(page_map.read_text(encoding="utf-8"), "keep\n")

    def test_intervening_mirror_files_are_not_deleted_on_failure(self) -> None:
        with tempfile.TemporaryDirectory(prefix="halofpx-wiki-file-race-test-") as temporary:
            output = Path(temporary) / "mirror"
            original_write = mirror._write_new_file
            calls = 0
            first_path: Path | None = None
            second_path: Path | None = None

            def race(path: Path, data: bytes, role: str) -> None:
                nonlocal calls, first_path, second_path
                calls += 1
                if calls == 1:
                    first_path = path
                    original_write(path, data, role)
                    path.unlink()
                    path.write_text("replacement\n", encoding="utf-8")
                    return
                if calls == 2:
                    second_path = path
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_text("intervening\n", encoding="utf-8")
                original_write(path, data, role)

            with mock.patch.object(mirror, "_write_new_file", side_effect=race):
                with self.assertRaises(mirror.MirrorError):
                    mirror.write_mirror(self.rendered, output, self.repo)
            self.assertIsNotNone(first_path)
            self.assertIsNotNone(second_path)
            self.assertEqual(first_path.read_text(encoding="utf-8"), "replacement\n")
            self.assertEqual(second_path.read_text(encoding="utf-8"), "intervening\n")
            self.assertFalse(output.exists())

    def test_atomic_publish_rejects_intervening_output_redirect(self) -> None:
        with tempfile.TemporaryDirectory(prefix="halofpx-wiki-root-race-test-") as temporary:
            root = Path(temporary)
            output = root / "mirror"
            external = root / "external"
            external.mkdir()
            original_publish = mirror._publish_no_replace

            def race(staging: Path, destination: Path, role: str) -> None:
                self.assertEqual(destination, output)
                _create_directory_redirect(destination, external)
                original_publish(staging, destination, role)

            try:
                with mock.patch.object(mirror, "_publish_no_replace", side_effect=race):
                    with self.assertRaises(mirror.MirrorError):
                        mirror.write_mirror(self.rendered, output, self.repo)
                self.assertEqual(list(external.iterdir()), [])
                self.assertTrue(mirror._is_reparse_path(output))
            finally:
                if mirror._is_reparse_path(output):
                    _remove_directory_redirect(output)

    def test_freeze_map_rejects_intervening_parent_redirect(self) -> None:
        with tempfile.TemporaryDirectory(prefix="halofpx-wiki-map-race-test-") as temporary:
            root = Path(temporary)
            parent = root / "map-parent"
            held_parent = root / "held-parent"
            external = root / "external"
            parent.mkdir()
            external.mkdir()
            output = parent / "page-map.json"
            original_publish = mirror._publish_no_replace

            def race(staging: Path, destination: Path, role: str) -> None:
                parent.rename(held_parent)
                _create_directory_redirect(parent, external)
                try:
                    original_publish(staging, destination, role)
                finally:
                    _remove_directory_redirect(parent)
                    held_parent.rename(parent)

            with mock.patch.object(mirror, "load_snapshot", return_value=self.snapshot), mock.patch.object(
                mirror, "_publish_no_replace", side_effect=race
            ):
                self.assertEqual(
                    mirror.main(
                        [
                            "--repo",
                            str(self.repo),
                            "freeze-map",
                            "--output",
                            str(output),
                        ]
                    ),
                    1,
                )
            self.assertEqual(list(external.iterdir()), [])
            self.assertFalse(output.exists())

    @unittest.skipUnless(os.name == "nt", "Windows junction regression")
    def test_existing_output_validation_rejects_windows_junctions(self) -> None:
        with tempfile.TemporaryDirectory(prefix="halofpx-wiki-junction-test-") as temporary:
            temporary_root = Path(temporary)
            output = temporary_root / "mirror"
            mirror.write_mirror(self.rendered, output, self.repo)
            external_assets = temporary_root / "external-assets"
            shutil.move(str(output / "assets"), external_assets)
            junction = output / "assets"
            subprocess.run(
                ["cmd.exe", "/d", "/c", "mklink", "/J", str(junction), str(external_assets)],
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            try:
                self.assertTrue(junction.is_junction())
                with self.assertRaises(mirror.MirrorError):
                    mirror.compare_output(self.rendered, output)
                with self.assertRaises(mirror.MirrorError):
                    mirror.audit_manifest_output(output)
            finally:
                os.rmdir(junction)

            target = temporary_root / "target-mirror"
            mirror.write_mirror(self.rendered, target, self.repo)
            root_junction = temporary_root / "mirror-junction"
            subprocess.run(
                ["cmd.exe", "/d", "/c", "mklink", "/J", str(root_junction), str(target)],
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            try:
                self.assertTrue(root_junction.is_junction())
                with self.assertRaises(mirror.MirrorError):
                    mirror.compare_output(self.rendered, root_junction)
                with self.assertRaises(mirror.MirrorError):
                    mirror.audit_manifest_output(root_junction)
                with self.assertRaises(mirror.MirrorError):
                    mirror.write_mirror(self.rendered, root_junction, self.repo)
                with mock.patch.object(mirror, "load_snapshot", return_value=self.snapshot), mock.patch.object(
                    mirror, "load_frozen_map", return_value=self.page_map
                ), mock.patch.object(mirror, "build_mirror", return_value=self.rendered):
                    self.assertEqual(
                        mirror.main(
                            [
                                "--repo",
                                str(self.repo),
                                "validate",
                                "--output",
                                str(root_junction),
                            ]
                        ),
                        1,
                    )
            finally:
                os.rmdir(root_junction)

    def test_parser_stack_is_exactly_locked(self) -> None:
        mirror.validate_runtime_dependencies()
        with mock.patch.object(
            mirror.importlib.metadata, "version", return_value="999.0"
        ):
            with self.assertRaises(mirror.MirrorError):
                mirror.validate_runtime_dependencies()

    def test_shadow_parser_is_rejected_before_code_execution(self) -> None:
        with tempfile.TemporaryDirectory(prefix="halofpx-wiki-parser-shadow-") as temporary:
            root = Path(temporary)
            sentinel = root / "executed.txt"
            shadow = root / "markdown_it" / "__init__.py"
            shadow.parent.mkdir()
            shadow.write_text(
                f"from pathlib import Path\nPath({str(sentinel)!r}).write_text('executed')\n",
                encoding="utf-8",
            )
            environment = os.environ.copy()
            environment["PYTHONPATH"] = os.pathsep.join(
                [str(root), str(TOOL_DIRECTORY), environment.get("PYTHONPATH", "")]
            )
            completed = subprocess.run(
                [
                    sys.executable,
                    "-c",
                    "import github_wiki_mirror as m; m.validate_runtime_dependencies()",
                ],
                env=environment,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            self.assertNotEqual(completed.returncode, 0)
            self.assertFalse(sentinel.exists(), completed.stderr)

    def test_git_reads_disable_lazy_fetch_and_prompts(self) -> None:
        completed = mock.Mock(returncode=0, stdout=b"ok", stderr=b"")
        with mock.patch.object(mirror.subprocess, "run", return_value=completed) as run:
            self.assertEqual(mirror._run_git(self.repo, "version"), b"ok")
        environment = run.call_args.kwargs["env"]
        self.assertEqual(
            run.call_args.args[0][:2],
            ["git", "--no-replace-objects"],
        )
        self.assertEqual(environment["GIT_NO_LAZY_FETCH"], "1")
        self.assertEqual(environment["GIT_NO_REPLACE_OBJECTS"], "1")
        self.assertEqual(environment["GIT_TERMINAL_PROMPT"], "0")
        for name in mirror.GIT_ROUTING_ENVIRONMENT:
            self.assertNotIn(name, environment)

    def test_git_reads_ignore_local_replace_objects(self) -> None:
        def run(repo: Path, *arguments: str, env: dict[str, str] | None = None) -> bytes:
            return subprocess.run(
                ["git", "-C", str(repo), *arguments],
                check=True,
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            ).stdout

        with tempfile.TemporaryDirectory(prefix="halofpx-wiki-replace-test-") as temporary:
            repo = Path(temporary)
            run(repo, "init", "--quiet")
            run(repo, "config", "user.name", "HaloFPX mirror test")
            run(repo, "config", "user.email", "mirror-test@example.invalid")

            source = repo / "source.txt"
            source.write_bytes(b"canonical\n")
            run(repo, "add", "source.txt")
            run(repo, "commit", "--quiet", "-m", "canonical")
            canonical_commit = run(repo, "rev-parse", "HEAD").decode().strip()
            canonical_blob = run(repo, "rev-parse", f"{canonical_commit}:source.txt")

            source.write_bytes(b"replacement\n")
            run(repo, "commit", "--quiet", "-am", "replacement")
            replacement_commit = run(repo, "rev-parse", "HEAD").decode().strip()
            replacement_blob = run(repo, "rev-parse", f"{replacement_commit}:source.txt")
            self.assertNotEqual(canonical_blob, replacement_blob)

            run(repo, "replace", canonical_commit, replacement_commit)
            self.assertEqual(
                run(repo, "rev-parse", f"{canonical_commit}:source.txt"),
                replacement_blob,
            )
            self.assertEqual(
                mirror._run_git(repo, "rev-parse", f"{canonical_commit}:source.txt"),
                canonical_blob,
            )

    def test_git_reads_ignore_ambient_repository_redirection(self) -> None:
        def run(repo: Path, *arguments: str) -> bytes:
            return subprocess.run(
                ["git", "-C", str(repo), *arguments],
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            ).stdout

        with tempfile.TemporaryDirectory(prefix="halofpx-wiki-git-env-test-") as temporary:
            repositories = [Path(temporary) / "one", Path(temporary) / "two"]
            heads = []
            for index, repo in enumerate(repositories):
                repo.mkdir()
                run(repo, "init", "--quiet")
                run(repo, "config", "user.name", "HaloFPX mirror test")
                run(repo, "config", "user.email", "mirror-test@example.invalid")
                (repo / "source.txt").write_text(f"repository {index}\n", encoding="utf-8")
                run(repo, "add", "source.txt")
                run(repo, "commit", "--quiet", "-m", f"repository {index}")
                heads.append(run(repo, "rev-parse", "HEAD"))
            self.assertNotEqual(*heads)
            with mock.patch.dict(
                os.environ,
                {
                    "GIT_DIR": str(repositories[1] / ".git"),
                    "GIT_WORK_TREE": str(repositories[1]),
                },
            ):
                self.assertEqual(mirror._run_git(repositories[0], "rev-parse", "HEAD"), heads[0])


if __name__ == "__main__":
    unittest.main()
